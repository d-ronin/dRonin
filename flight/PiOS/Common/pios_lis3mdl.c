/**
 ******************************************************************************
 * @file       lis3mdl.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016-2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_LIS3MDL
 * @{
 * @brief Driver for Bosch LIS3MDL IMU Sensor (part of BMX055)
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

#if defined(PIOS_INCLUDE_LIS3MDL)

#include "pios_semaphore.h"
#include "pios_thread.h"
#include "pios_queue.h"
#include "physical_constants.h"
#include "taskmonitor.h"

#include "pios_lis3mdl_priv.h"

#define PIOS_LIS_SPI_SPEED       9400000	/* 10MHz max per datasheet.
						 * probably results 9.33MHz */

/**
 * Magic byte sequence used to validate the device state struct.
 * Should be unique amongst all PiOS drivers!
 */
enum lis3mdl_dev_magic {
	PIOS_LIS_DEV_MAGIC = 0x3353494c /**< Unique byte sequence 'LIS3' */
};

/**
 * @brief The device state struct
 */
struct lis3mdl_dev {
	enum lis3mdl_dev_magic magic;       /**< Magic bytes to validate the struct contents */
	const struct pios_lis3mdl_cfg *cfg; /**< Device configuration structure */
	pios_spi_t spi_id;                  /**< Handle to the communication driver */
	uint32_t spi_slave_mag;             /**< The slave number (SPI) */
};

//! Private functions
/**
 * @brief Allocate a new device
 */
static struct lis3mdl_dev *PIOS_LIS_Alloc(const struct pios_lis3mdl_cfg *cfg);

/**
 * @brief Validate the handle to the device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_LIS_Validate(struct lis3mdl_dev *dev);

static bool PIOS_LIS_Callback(void *ctx, void *output,
		int ms_to_wait, int *next_call);

/**
 * @brief Claim the SPI bus for the communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_LIS_ClaimBus();

/**
 * @brief Release the SPI bus for the communications and end the transaction
 * @return 0 if successful
 */
static int32_t PIOS_LIS_ReleaseBus();

static int32_t PIOS_LIS_ReadReg(lis3mdl_dev_t lis_dev,
		uint8_t address, uint8_t *buffer);
static int32_t PIOS_LIS_WriteReg(lis3mdl_dev_t lis_dev,
		uint8_t address, uint8_t buffer); 

static struct lis3mdl_dev *PIOS_LIS_Alloc(const struct pios_lis3mdl_cfg *cfg)
{
	lis3mdl_dev_t dev;

	dev = (struct lis3mdl_dev *)PIOS_malloc(sizeof(*dev));
	if (!dev)
		return NULL;

	dev->magic = PIOS_LIS_DEV_MAGIC;

	return dev;
}

static int32_t PIOS_LIS_Validate(struct lis3mdl_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_LIS_DEV_MAGIC)
		return -2;
	if (dev->spi_id == 0)
		return -3;
	return 0;
}

static int32_t AssertReg(lis3mdl_dev_t lis_dev,
		uint8_t address, uint8_t expect) {
	uint8_t c;

	int32_t ret = PIOS_LIS_ReadReg(lis_dev, address, &c);

	if (ret) {
		return ret;
	}

	if (c != expect) {
		DEBUG_PRINTF(2,
			"LIS: Assertion failed: *(%02x) == %02x (expect %02x)\n",
			address, c, expect);
		return -1;
	}

	DEBUG_PRINTF(2, "LIS: Assertion passed: *(%02x) == %02x\n", address,
		expect);

	return 0;
}

int32_t PIOS_LIS3MDL_SPI_Init(lis3mdl_dev_t *dev, pios_spi_t spi_id, 
		uint32_t slave_mag, const struct pios_lis3mdl_cfg *cfg)
{
	lis3mdl_dev_t lis_dev = PIOS_LIS_Alloc(cfg);
	if (lis_dev == NULL)
		return -1;
	*dev = lis_dev;

	lis_dev->spi_id = spi_id;
	lis_dev->spi_slave_mag = slave_mag;
	lis_dev->cfg = cfg;

	int32_t ret;

	/* Unfortunately can't check mag right away, so proceed with
	 * init
	 */

	/* Verify expected chip id. */	
	ret = AssertReg(lis_dev, LIS_REG_MAG_WHO_AM_I,
			LIS_WHO_AM_I_VAL);

	if (ret) {
		return ret;
	}

	/* Startup sequence, per ST AN4602 */

	/* First, write CTRL_REG2 to set scale */
	ret = PIOS_LIS_WriteReg(lis_dev, LIS_REG_MAG_CTRL2,
			LIS_CTRL2_FS_12GAU);

	if (ret) {
		return ret;
	}

	/* Next, set UHP on X/Y and ODR=80.  Enable temp sensor */
	ret = PIOS_LIS_WriteReg(lis_dev, LIS_REG_MAG_CTRL1,
			LIS_CTRL1_OPMODE_UHP | 
			LIS_CTRL1_ODR_80 |
			LIS_CTRL1_TEMPEN);

	if (ret) {
		return ret;
	}

	/* Next, enable UHP on Z axis */
	ret = PIOS_LIS_WriteReg(lis_dev, LIS_REG_MAG_CTRL4,
			LIS_CTRL4_OPMODEZ_UHP);

	if (ret) {
		return ret;
	}

	/* Enable "Block data update" mode which prevents skew between
	 * LSB / MSB bytes of sensor value */
	ret = PIOS_LIS_WriteReg(lis_dev, LIS_REG_MAG_CTRL5,
			LIS_CTRL5_BDU);

	if (ret) {
		return ret;
	}

	/* Finally, enable continuous measurement mode with these parameters */
	ret = PIOS_LIS_WriteReg(lis_dev, LIS_REG_MAG_CTRL3,
			LIS_CTRL3_MODE_CONTINUOUS);

	if (ret) {
		return ret;
	}

	PIOS_SENSORS_RegisterCallback(PIOS_SENSOR_MAG, 
			PIOS_LIS_Callback, lis_dev);

	return 0;
}

static int32_t PIOS_LIS_ClaimBus(lis3mdl_dev_t lis_dev)
{
	PIOS_Assert(!PIOS_LIS_Validate(lis_dev));

	if (PIOS_SPI_ClaimBus(lis_dev->spi_id) != 0)
		return -2;

	PIOS_SPI_RC_PinSet(lis_dev->spi_id, lis_dev->spi_slave_mag, false);

	PIOS_SPI_SetClockSpeed(lis_dev->spi_id, PIOS_LIS_SPI_SPEED);

	return 0;
}

static int32_t PIOS_LIS_ReleaseBus(lis3mdl_dev_t lis_dev)
{
	PIOS_Assert(!PIOS_LIS_Validate(lis_dev));

	PIOS_SPI_RC_PinSet(lis_dev->spi_id, lis_dev->spi_slave_mag, true);

	PIOS_SPI_ReleaseBus(lis_dev->spi_id);

	return 0;
}

static int32_t PIOS_LIS_ReadReg(lis3mdl_dev_t lis_dev,
		uint8_t address, uint8_t *buffer)
{
	PIOS_Assert(!(address & LIS_ADDRESS_READ));
	PIOS_Assert(!(address & LIS_ADDRESS_AUTOINCREMENT));

	if (PIOS_LIS_ClaimBus(lis_dev) != 0)
		return -1;

	PIOS_SPI_TransferByte(lis_dev->spi_id, LIS_ADDRESS_READ |
		address); // request byte
	*buffer = PIOS_SPI_TransferByte(lis_dev->spi_id, 0);   // receive response

	PIOS_LIS_ReleaseBus(lis_dev);

	return 0;
}

static int32_t PIOS_LIS_WriteReg(lis3mdl_dev_t lis_dev,
		uint8_t address, uint8_t buffer)
{
	PIOS_Assert(!PIOS_LIS_Validate(lis_dev));

	PIOS_Assert(!(address & LIS_ADDRESS_READ));
	PIOS_Assert(!(address & LIS_ADDRESS_AUTOINCREMENT));

	if (PIOS_LIS_ClaimBus(lis_dev) != 0)
		return -1;

	PIOS_SPI_TransferByte(lis_dev->spi_id, 0x7f & address);
	PIOS_SPI_TransferByte(lis_dev->spi_id, buffer);

	PIOS_LIS_ReleaseBus(lis_dev);

	return 0;
}

static bool PIOS_LIS_Callback(void *ctx, void *output,
		int ms_to_wait, int *next_call)
{
	lis3mdl_dev_t lis_dev = (lis3mdl_dev_t) ctx;

	PIOS_Assert(!PIOS_LIS_Validate(lis_dev));

	/* Wait 2ms if we don't have the data */
	*next_call = 2;

	uint8_t status;
	if (PIOS_LIS_ReadReg(lis_dev, LIS_REG_MAG_STATUS, &status))
		return false;

	if (!(status & LIS_STATUS_ZYXDA))
		return false;

	if (PIOS_LIS_ClaimBus(lis_dev) != 0)
		return false;

	// TODO: Consider temperature in future
	int16_t sensor_buf[
		(LIS_REG_MAG_OUTZ_H - LIS_REG_MAG_OUTX_L + 1) / 2];

	PIOS_SPI_TransferByte(lis_dev->spi_id,
			LIS_ADDRESS_READ | LIS_ADDRESS_AUTOINCREMENT |
			LIS_REG_MAG_OUTX_L);

	if (PIOS_SPI_TransferBlock(lis_dev->spi_id, NULL,
				(uint8_t *) sensor_buf,
				sizeof(sensor_buf)) < 0) {
		PIOS_LIS_ReleaseBus(lis_dev);
		return false;
	}

	PIOS_LIS_ReleaseBus(lis_dev);

#define GET_SB_FROM_REGNO(x) \
		(sensor_buf[ ((x) - LIS_REG_MAG_OUTX_L) / 2])

	float mag_x = GET_SB_FROM_REGNO(LIS_REG_MAG_OUTX_L);
	float mag_y = GET_SB_FROM_REGNO(LIS_REG_MAG_OUTY_L);
	float mag_z = GET_SB_FROM_REGNO(LIS_REG_MAG_OUTZ_L);

	struct pios_sensor_mag_data *mag_data = output;

	/* Adjust from 2's comp integer value to appropriate
	 * range */
	float mag_scale = LIS_RANGE_12GAU_COUNTS_PER_MGAU;

	/*
	 * Vehicle axes = x front, y right, z down
	 * LIS3MDL axes = x left, y rear, z up
	 * See flight/Doc/imu_orientation.md
	 */
	switch (lis_dev->cfg->orientation) {
		case PIOS_LIS_TOP_0DEG:
			mag_data->y  = -mag_x * mag_scale;
			mag_data->x  = -mag_y * mag_scale;
			mag_data->z  = -mag_z * mag_scale;
			break;
		case PIOS_LIS_TOP_90DEG:
			mag_data->y  = -mag_y * mag_scale;
			mag_data->x  =  mag_x * mag_scale;
			mag_data->z  = -mag_z * mag_scale;
			break;
		case PIOS_LIS_TOP_180DEG:
			mag_data->y  =  mag_x * mag_scale;
			mag_data->x  =  mag_y * mag_scale;
			mag_data->z  = -mag_z * mag_scale;
			break;
		case PIOS_LIS_TOP_270DEG:
			mag_data->y  =  mag_y * mag_scale;
			mag_data->x  = -mag_x * mag_scale;
			mag_data->z  = -mag_z * mag_scale;
			break;
		case PIOS_LIS_BOTTOM_0DEG:
			mag_data->y  =  mag_x * mag_scale;
			mag_data->x  = -mag_y * mag_scale;
			mag_data->z  =  mag_z * mag_scale;
			break;
		case PIOS_LIS_BOTTOM_90DEG:
			mag_data->y  = -mag_y * mag_scale;
			mag_data->x  = -mag_x * mag_scale;
			mag_data->z  =  mag_z * mag_scale;
			break;
		case PIOS_LIS_BOTTOM_180DEG:
			mag_data->y  = -mag_x * mag_scale;
			mag_data->x  =  mag_y * mag_scale;
			mag_data->z  =  mag_z * mag_scale;
			break;
		case PIOS_LIS_BOTTOM_270DEG:
			mag_data->y  =  mag_y * mag_scale;
			mag_data->x  =  mag_x * mag_scale;
			mag_data->z  =  mag_z * mag_scale;
			break;
	}

	/* Output rate of 80 implies 12.5ms between samples.
	 * We sleep 11ms before first try, and then 2ms between subsequent
	 * subsequent tries.  This means we normally get the data on our
	 * second try.
	 */
	*next_call = 11;

	return true;
}

#endif // PIOS_INCLUDE_LIS3MDL

/**
 * @}
 * @}
 */
