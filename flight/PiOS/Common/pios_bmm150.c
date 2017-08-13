/**
 ******************************************************************************
 * @file       pios_bmm150.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMM150
 * @{
 * @brief Driver for Bosch BMM150 IMU Sensor (part of BMX055)
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "openpilot.h"
#include "pios.h"

#if defined(PIOS_INCLUDE_BMM150)

#include "pios_semaphore.h"
#include "pios_thread.h"
#include "pios_queue.h"
#include "physical_constants.h"
#include "taskmonitor.h"

#include "pios_bmm150_priv.h"

#define PIOS_BMM_TASK_PRIORITY   PIOS_THREAD_PRIO_HIGHEST

#define PIOS_BMM_TASK_STACK      640

#define PIOS_BMM_QUEUE_LEN       2

#define PIOS_BMM_SPI_SPEED       10000000


/**
 * Magic byte sequence used to validate the device state struct.
 * Should be unique amongst all PiOS drivers!
 */
enum pios_bmm150_dev_magic {
	PIOS_BMM_DEV_MAGIC = 0x586e6f42 /**< Unique byte sequence 'BnoX' */
};

/**
 * @brief The device state struct
 */
struct pios_bmm150_dev {
	enum pios_bmm150_dev_magic magic;       /**< Magic bytes to validate the struct contents */
	const struct pios_bmm150_cfg *cfg;      /**< Device configuration structure */
	pios_spi_t spi_id;                      /**< Handle to the communication driver */
	uint32_t spi_slave_mag;                 /**< The slave number (SPI) */

	struct pios_queue *mag_queue;

	struct pios_thread *task_handle;

	int8_t dig_x1;     /**< trim x1 data */
	int8_t dig_y1;     /**< trim y1 data */

	int8_t dig_x2;     /**< trim x2 data */
	int8_t dig_y2;     /**< trim y2 data */

	uint16_t dig_z1;   /**< trim z1 data */
	int16_t dig_z2;    /**< trim z2 data */
	int16_t dig_z3;    /**< trim z3 data */
	int16_t dig_z4;    /**< trim z4 data */

	uint8_t dig_xy1;   /**< trim xy1 data */
	int8_t dig_xy2;    /**< trim xy2 data */

	uint16_t dig_xyz1; /**< trim xyz1 data */
};

//! Global structure for this device device
static struct pios_bmm150_dev *bmm_dev;

//! Private functions
/**
 * @brief Allocate a new device
 */
static struct pios_bmm150_dev *PIOS_BMM_Alloc(const struct pios_bmm150_cfg *cfg);

/**
 * @brief Validate the handle to the device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_BMM_Validate(struct pios_bmm150_dev *dev);

static void PIOS_BMM_Task(void *parameters);

static inline float bmm050_compensate_X_float(pios_bmm150_dev_t p_bmm050,
		int16_t mag_data_x, uint16_t data_r);
static inline float bmm050_compensate_Y_float(pios_bmm150_dev_t p_bmm050,
		int16_t mag_data_y, uint16_t data_r);
static inline float bmm050_compensate_Z_float(pios_bmm150_dev_t p_bmm050,
		int16_t mag_data_z, uint16_t data_r);
/**
 * @brief Claim the SPI bus for the communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_BMM_ClaimBus();

/**
 * @brief Release the SPI bus for the communications and end the transaction
 * @return 0 if successful
 */
static int32_t PIOS_BMM_ReleaseBus();

static int32_t PIOS_BMM_ReadReg(uint8_t address, uint8_t *buffer);
static int32_t PIOS_BMM_WriteReg(uint8_t address, uint8_t buffer);

static struct pios_bmm150_dev *PIOS_BMM_Alloc(const struct pios_bmm150_cfg *cfg)
{
	struct pios_bmm150_dev *dev;

	dev = (struct pios_bmm150_dev *)PIOS_malloc(sizeof(*bmm_dev));
	if (!dev)
		return NULL;

	dev->magic = PIOS_BMM_DEV_MAGIC;

	dev->mag_queue = PIOS_Queue_Create(PIOS_BMM_QUEUE_LEN, sizeof(struct pios_sensor_mag_data));
	if (dev->mag_queue == NULL) {
		PIOS_free(dev);
		return NULL;
	}

	return dev;
}

static int32_t PIOS_BMM_Validate(struct pios_bmm150_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_BMM_DEV_MAGIC)
		return -2;
	if (dev->spi_id == 0)
		return -3;
	return 0;
}

static int32_t AssertReg(uint8_t address, uint8_t expect) {
	uint8_t c;

	int32_t ret = PIOS_BMM_ReadReg(address, &c);

	if (ret) {
		return ret;
	}

	if (c != expect) {
		DEBUG_PRINTF(2, "BMM: Assertion failed: *(%02x) == %02x (expect %02x)\n",
		address, c, expect);
		return -1;
	}

	DEBUG_PRINTF(2, "BMM: Assertion passed: *(%02x) == %02x\n", address,
		expect);

	return 0;
}

static int32_t PIOS_BMM150_ReadTrims(pios_bmm150_dev_t dev)
{
	int ret;
	
	ret = PIOS_BMM_ReadReg(BMM150_DIG_X1, (uint8_t *) &dev->dig_x1);
	if (ret) return ret;

	ret = PIOS_BMM_ReadReg(BMM150_DIG_X2, (uint8_t *) &dev->dig_x2);
	if (ret) return ret;

	ret = PIOS_BMM_ReadReg(BMM150_DIG_Y1, (uint8_t *) &dev->dig_y1);
	if (ret) return ret;

	ret = PIOS_BMM_ReadReg(BMM150_DIG_Y2, (uint8_t *) &dev->dig_y2);
	if (ret) return ret;

	ret = PIOS_BMM_ReadReg(BMM150_DIG_X1, &dev->dig_xy1);
	if (ret) return ret;

	ret = PIOS_BMM_ReadReg(BMM150_DIG_X2, (uint8_t *) &dev->dig_xy2);
	if (ret) return ret;

	/* Sensor is little endian; all supported platforms are little endian
	 * presently, but try to do the right thing anyways */

	uint8_t tmp[2];

	ret = PIOS_BMM_ReadReg(BMM150_DIG_Z1_LSB, tmp);
	if (ret) return ret;
	ret = PIOS_BMM_ReadReg(BMM150_DIG_Z1_MSB, tmp+1);
	if (ret) return ret;
	dev->dig_z1 = (tmp[1] << 8) | tmp[0];

	ret = PIOS_BMM_ReadReg(BMM150_DIG_Z2_LSB, tmp);
	if (ret) return ret;
	ret = PIOS_BMM_ReadReg(BMM150_DIG_Z2_MSB, tmp+1);
	if (ret) return ret;
	dev->dig_z2 = (tmp[1] << 8) | tmp[0];

	ret = PIOS_BMM_ReadReg(BMM150_DIG_Z3_LSB, tmp);
	if (ret) return ret;
	ret = PIOS_BMM_ReadReg(BMM150_DIG_Z3_MSB, tmp+1);
	if (ret) return ret;
	dev->dig_z3 = (tmp[1] << 8) | tmp[0];

	ret = PIOS_BMM_ReadReg(BMM150_DIG_Z4_LSB, tmp);
	if (ret) return ret;
	ret = PIOS_BMM_ReadReg(BMM150_DIG_Z4_MSB, tmp+1);
	if (ret) return ret;
	dev->dig_z4 = (tmp[1] << 8) | tmp[0];

	ret = PIOS_BMM_ReadReg(BMM150_DIG_XYZ1_LSB, tmp);
	if (ret) return ret;
	ret = PIOS_BMM_ReadReg(BMM150_DIG_XYZ1_MSB, tmp+1);
	if (ret) return ret;
	dev->dig_xyz1 = (tmp[1] << 8) | tmp[0];

	DEBUG_PRINTF(2, "BMM: x1=%d y1=%d x2=%d y2=%d xy1=%d xy2=%d\n",
			dev->dig_x1, dev->dig_y1, dev->dig_x2, dev->dig_y2,
			dev->dig_xy1, dev->dig_xy2);

	DEBUG_PRINTF(2, "BMM: z1=%d z2=%d z3=%d z4=%d xyz1=%d\n",
			dev->dig_z1, dev->dig_z2, dev->dig_z3, dev->dig_z4,
			dev->dig_xyz1);

	return 0;
}

int32_t PIOS_BMM150_SPI_Init(pios_bmm150_dev_t *dev, pios_spi_t spi_id, 
		uint32_t slave_mag, const struct pios_bmm150_cfg *cfg)
{
	bmm_dev = PIOS_BMM_Alloc(cfg);
	if (bmm_dev == NULL)
		return -1;
	*dev = bmm_dev;

	bmm_dev->spi_id = spi_id;
	bmm_dev->spi_slave_mag = slave_mag;
	bmm_dev->cfg = cfg;

	int32_t ret;

	/* Unfortunately can't check mag right away, so proceed with
	 * init
	 */

	DEBUG_PRINTF(2, "BMM: Resetting sensor\n");

	ret = PIOS_BMM_WriteReg(BMM150_REG_MAG_POWER_CONTROL,
			BMM150_VAL_MAG_POWER_CONTROL_POWEROFF);

	if (ret) return ret;

	PIOS_DELAY_WaitmS(20);

	ret = PIOS_BMM_WriteReg(BMM150_REG_MAG_POWER_CONTROL,
			BMM150_VAL_MAG_POWER_CONTROL_POWERON);

	if (ret) return ret;

	PIOS_DELAY_WaitmS(30);

	/* Verify expected chip id. */	
	ret = AssertReg(BMM150_REG_MAG_CHIPID, BMM150_VAL_MAG_CHIPID);

	if (ret) return ret;

	ret = PIOS_BMM150_ReadTrims(bmm_dev);

	if (ret) {
		DEBUG_PRINTF(2, "BMM: Unable to read trims!\n");
		return ret;
	}

	/* We choose a point between "Enhanced Regular preset"
	 * nXy=15, nZ=27, max ODR=60  (calc by data sheet formula 60.04)
	 * and "High accuracy preset"
	 * nXY=47, nZ=83, max ODR=20. (calc by data sheet formula 20.29)
	 *
	 * nXy=29, nZ=54.  yields ODR>31.   Which makes ODR 30 safe.
	 * A little more data in exchange for a little more RMS noise,
	 * which we can smooth out somewhat anyways.
	 */

	/* 14 * 2 + 1 = 29, per above */
	ret = PIOS_BMM_WriteReg(BMM150_REG_MAG_X_Y_AXIS_REP, 14);

	if (ret) {
		return ret;
	}

	/* 53 + 1 = 54, per above */
	ret = PIOS_BMM_WriteReg(BMM150_REG_MAG_Z_AXIS_REP, 53);

	if (ret) {
		return ret;
	}

	/* Start the sensor in normal mode, ODR=30 */
	ret = PIOS_BMM_WriteReg(BMM150_REG_MAG_OPERATION_MODE,
			BMM150_VAL_MAG_OPERATION_MODE_ODR30 |
			BMM150_VAL_MAG_OPERATION_MODE_OPMODE_NORMAL);

	if (ret) {
		return ret;
	}

	bmm_dev->task_handle = PIOS_Thread_Create(
			PIOS_BMM_Task, "pios_bmm", PIOS_BMM_TASK_STACK,
			NULL, PIOS_BMM_TASK_PRIORITY);

	PIOS_SENSORS_Register(PIOS_SENSOR_MAG, bmm_dev->mag_queue);

	return 0;
}

static int32_t PIOS_BMM_ClaimBus()
{
	if (PIOS_BMM_Validate(bmm_dev) != 0)
		return -1;

	if (PIOS_SPI_ClaimBus(bmm_dev->spi_id) != 0)
		return -2;

	PIOS_SPI_RC_PinSet(bmm_dev->spi_id, bmm_dev->spi_slave_mag, false);

	PIOS_SPI_SetClockSpeed(bmm_dev->spi_id, 10000000);

	return 0;
}

static int32_t PIOS_BMM_ReleaseBus()
{
	if (PIOS_BMM_Validate(bmm_dev) != 0)
		return -1;

	PIOS_SPI_RC_PinSet(bmm_dev->spi_id, bmm_dev->spi_slave_mag, true);

	PIOS_SPI_ReleaseBus(bmm_dev->spi_id);

	return 0;
}

static int32_t PIOS_BMM_ReadReg(uint8_t address, uint8_t *buffer)
{
	if (PIOS_BMM_ClaimBus() != 0)
		return -1;

	PIOS_SPI_TransferByte(bmm_dev->spi_id, 0x80 | address); // request byte
	*buffer = PIOS_SPI_TransferByte(bmm_dev->spi_id, 0);   // receive response

	PIOS_BMM_ReleaseBus();

	return 0;
}

static int32_t PIOS_BMM_WriteReg(uint8_t address, uint8_t buffer)
{
	if (PIOS_BMM_ClaimBus() != 0)
		return -1;

	PIOS_SPI_TransferByte(bmm_dev->spi_id, 0x7f & address);
	PIOS_SPI_TransferByte(bmm_dev->spi_id, buffer);

	PIOS_BMM_ReleaseBus();

	return 0;
}

static void PIOS_BMM_Task(void *parameters)
{
	(void)parameters;

	while (true) {
		PIOS_Thread_Sleep(6);		/* XXX */

		uint8_t drdy;

		if (PIOS_BMM_ReadReg(BMM150_REG_MAG_HALL_RESISTANCE_LSB,
					&drdy))
			continue;

		/* Check if there's any new data.  If not.. wait 6 more ms
		 * (we expect it at 33ms intervals).
		 * At the end of this we sleep 24, which results in a first
		 * drdy check at 30ms [3ms early] and the next check at 36ms
		 * [3ms late].  Should keep us disciplined very close to
		 * measurements so they're not sitting around long.
		 */
		if (!(drdy & BMM150_VAL_MAG_HALL_RESISTANCE_LSB_DRDY))
			continue;

		if (PIOS_BMM_ClaimBus() != 0)
			continue;

		uint8_t sensor_buf[BMM150_REG_MAG_HALL_RESISTANCE_MSB -
			BMM150_REG_MAG_X_LSB + 1];

		PIOS_SPI_TransferByte(bmm_dev->spi_id,
				0x80 | BMM150_REG_MAG_X_LSB);

		if (PIOS_SPI_TransferBlock(bmm_dev->spi_id, NULL, sensor_buf,
					sizeof(sensor_buf)) < 0) {
			PIOS_BMM_ReleaseBus();
			continue;
		}

		PIOS_BMM_ReleaseBus();

		/* The Bosch compensation functions appear to expect this data
		 * right-aligned, signed.  The unpacking functions in the 
		 * library are rather obtuse, but they do sign extension
		 * on the right align operation.
		 *
		 * We form them into a 16 bit left aligned signed value,
		 * then shift them right to right align and sign extend.
		 * Hopefully this is clearer.
		 */

#define PACK_REG13_ADDR_OFFSET(b, reg, off) ((int16_t) ( (b[reg-off] & 0xf8) | (b[reg-off+1] << 8) ))
#define PACK_REG14_ADDR_OFFSET(b, reg, off) ((int16_t) ( (b[reg-off] & 0xfc) | (b[reg-off+1] << 8) ))
#define PACK_REG15_ADDR_OFFSET(b, reg, off) ((int16_t) ( (b[reg-off] & 0xfe) | (b[reg-off+1] << 8) ))

		int16_t raw_x, raw_y, raw_z, raw_r;
		raw_x = PACK_REG13_ADDR_OFFSET(sensor_buf, BMM150_REG_MAG_X_LSB,
				BMM150_REG_MAG_X_LSB);
		raw_x >>= 3;
		raw_y = PACK_REG13_ADDR_OFFSET(sensor_buf, BMM150_REG_MAG_Y_LSB,
				BMM150_REG_MAG_X_LSB);
		raw_y >>= 3;
		raw_z = PACK_REG15_ADDR_OFFSET(sensor_buf, BMM150_REG_MAG_Z_LSB,
				BMM150_REG_MAG_X_LSB);
		raw_z >>= 1;
		raw_r = PACK_REG14_ADDR_OFFSET(sensor_buf,
				BMM150_REG_MAG_HALL_RESISTANCE_LSB,
				BMM150_REG_MAG_X_LSB);
		raw_r >>= 2;

		// Datasheet is typical res 0.3 uT.  X/Y -4096 to 4095,
		// Z -16384 to 16383.  .3*4095 checks out with expected range
		// (+/- 1228 uT vs datasheet +/- 1300uT)
		// .3 * 16384 is off by a factor of 2 though
		// (+/- 4915 uT vs datasheet +/- 2500uT)

		struct pios_sensor_mag_data mag_data;

		float mag_x = bmm050_compensate_X_float(bmm_dev,
				raw_x, raw_r);
		float mag_y = bmm050_compensate_Y_float(bmm_dev,
				raw_y, raw_r);
		float mag_z = bmm050_compensate_Z_float(bmm_dev,
				raw_z, raw_r);

		switch (bmm_dev->cfg->orientation) {
			case PIOS_BMM_TOP_0DEG:
				mag_data.y  =  mag_x;
				mag_data.x  =  mag_y;
				mag_data.z  = -mag_z;
				break;
			case PIOS_BMM_TOP_90DEG:
				mag_data.y  = -mag_y;
				mag_data.x  =  mag_x;
				mag_data.z  = -mag_z;
				break;
			case PIOS_BMM_TOP_180DEG:
				mag_data.y  = -mag_x;
				mag_data.x  = -mag_y;
				mag_data.z  = -mag_z;
				break;
			case PIOS_BMM_TOP_270DEG:
				mag_data.y  =  mag_y;
				mag_data.x  = -mag_x;
				mag_data.z  = -mag_z;
				break;
			case PIOS_BMM_BOTTOM_0DEG:
				mag_data.y  = -mag_x;
				mag_data.x  =  mag_y;
				mag_data.z  =  mag_z;
				break;
			case PIOS_BMM_BOTTOM_90DEG:
				mag_data.y  =  mag_y;
				mag_data.x  =  mag_x;
				mag_data.z  =  mag_z;
				break;
			case PIOS_BMM_BOTTOM_180DEG:
				mag_data.y  =  mag_x;
				mag_data.x  = -mag_y;
				mag_data.z  =  mag_z;
				break;
			case PIOS_BMM_BOTTOM_270DEG:
				mag_data.y  = -mag_y;
				mag_data.x  = -mag_x;
				mag_data.z  =  mag_z;
				break;
		}

		/* scale-- it reads in microtesla.  we want milligauss */
		float mag_scale = 10.0f;

		mag_data.x *= mag_scale;
		mag_data.y *= mag_scale;
		mag_data.z *= mag_scale;

		PIOS_Queue_Send(bmm_dev->mag_queue, &mag_data, 0);

		PIOS_Thread_Sleep(24);
	}
}

/* The following code originally comes from the Bosch SensorTec BMM050 driver:
* https://github.com/BoschSensortec/BMM050_driver/
*
****************************************************************************
* Copyright (C) 2015 - 2016 Bosch Sensortec GmbH
* Copyright (C) 2016 dRonin
*
* bmm050.c
* Date: 2016/03/17
* Revision: 2.0.6 $
*
* Usage: Sensor Driver for  BMM050 and BMM150 sensor
*
****************************************************************************
* License:
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   Redistributions of source code must retain the above copyright
*   notice, this list of conditions and the following disclaimer.
*
*   Redistributions in binary form must reproduce the above copyright
*   notice, this list of conditions and the following disclaimer in the
*   documentation and/or other materials provided with the distribution.
*
*   Neither the name of the copyright holder nor the names of the
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*/

/*!
 *	@brief This API used to get the compensated X data
 *	the out put of X as float
 *
 *
 *
 *  @param  mag_data_x : The value of raw X data
 *	@param  data_r : The value of R data
 *
 *	@return results of compensated X data value output as float
 *
*/
static inline float bmm050_compensate_X_float(pios_bmm150_dev_t p_bmm050,
		int16_t mag_data_x, uint16_t data_r)
{
	float inter_retval;

	if (mag_data_x != BMM050_FLIP_OVERFLOW_ADCVAL	/* no overflow */
	   ) {
		if ((data_r != BMM050_INIT_VALUE)
		&& (p_bmm050->dig_xyz1 != BMM050_INIT_VALUE)) {
			inter_retval = ((((float)p_bmm050->dig_xyz1)
			* 16384.0f / data_r) - 16384.0f);
		} else {
			inter_retval = BMM050_OVERFLOW_OUTPUT_FLOAT;
			return inter_retval;
		}
		inter_retval = (((mag_data_x * ((((((float)p_bmm050->dig_xy2) *
			(inter_retval*inter_retval /
			268435456.0f) +
			inter_retval * ((float)p_bmm050->dig_xy1)
			/ 16384.0f)) + 256.0f) *
			(((float)p_bmm050->dig_x2) + 160.0f)))
			/ 8192.0f)
			+ (((float)p_bmm050->dig_x1) *
			8.0f)) / 16.0f;
	} else {
		inter_retval = BMM050_OVERFLOW_OUTPUT_FLOAT;
	}
	return inter_retval;
}

/*!
 *	@brief This API used to get the compensated Y data
 *	the out put of Y as float
 *
 *
 *
 *  @param  mag_data_y : The value of raw Y data
 *	@param  data_r : The value of R data
 *
 *	@return results of compensated Y data value output as float
 *
*/
static inline float bmm050_compensate_Y_float(pios_bmm150_dev_t p_bmm050,
		int16_t mag_data_y, uint16_t data_r)
{
	float inter_retval;

	if (mag_data_y != BMM050_FLIP_OVERFLOW_ADCVAL /* no overflow */
	   ) {
		if ((data_r != BMM050_INIT_VALUE)
		&& (p_bmm050->dig_xyz1 != BMM050_INIT_VALUE)) {
			inter_retval = ((((float)p_bmm050->dig_xyz1)
			* 16384.0f
			/data_r) - 16384.0f);
		} else {
			inter_retval = BMM050_OVERFLOW_OUTPUT_FLOAT;
			return inter_retval;
		}
		inter_retval = (((mag_data_y * ((((((float)p_bmm050->dig_xy2) *
			(inter_retval*inter_retval
			/ 268435456.0f) +
			inter_retval * ((float)p_bmm050->dig_xy1)
			/ 16384.0f)) +
			256.0f) *
			(((float)p_bmm050->dig_y2) + 160.0f)))
			/ 8192.0f) +
			(((float)p_bmm050->dig_y1) * 8.0f))
			/ 16.0f;
	} else {
		/* overflow, set output to 0.0f */
		inter_retval = BMM050_OVERFLOW_OUTPUT_FLOAT;
	}
	return inter_retval;
}

/*!
 *	@brief This API used to get the compensated Z data
 *	the out put of Z as float
 *
 *
 *
 *  @param  mag_data_z : The value of raw Z data
 *	@param  data_r : The value of R data
 *
 *	@return results of compensated Z data value output as float
 *
*/
static inline float bmm050_compensate_Z_float(pios_bmm150_dev_t p_bmm050,
		int16_t mag_data_z, uint16_t data_r)
{
	float inter_retval = BMM050_OVERFLOW_OUTPUT_FLOAT;
	 /* no overflow */
	if (mag_data_z != BMM050_HALL_OVERFLOW_ADCVAL) {
		if ((p_bmm050->dig_z2 != BMM050_INIT_VALUE)
		&& (p_bmm050->dig_z1 != BMM050_INIT_VALUE)
		&& (p_bmm050->dig_xyz1 != BMM050_INIT_VALUE)
		&& (data_r != BMM050_INIT_VALUE)) {
			inter_retval = ((((((float)mag_data_z)-
			((float)p_bmm050->dig_z4)) * 131072.0f)-
			(((float)p_bmm050->dig_z3)*(((float)data_r)
			-((float)p_bmm050->dig_xyz1))))
			/((((float)p_bmm050->dig_z2)+
			((float)p_bmm050->dig_z1)*((float)data_r) /
			32768.0f) * 4.0f)) / 16.0f;
		}
	} 

	return inter_retval;
}

#endif // PIOS_INCLUDE_BMM150

/**
 * @}
 * @}
 */
