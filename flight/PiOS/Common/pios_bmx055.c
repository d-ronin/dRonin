/**
 ******************************************************************************
 * @file       pios_bmx055.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMX055
 * @{
 * @brief Driver for Bosch BMX055 IMU Sensor
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

/* The PIOS_BMX055 contains several sensors integrated into one package.
 * The accelerometer and gyro are supported here.  The magnetomter is supported
 * by the BMM150 driver, due to its different characteristics and the fac that
 * you might want to skip it if you're using an external mag.
 */

#include "openpilot.h"
#include "pios.h"

#if defined(PIOS_INCLUDE_BMX055)

#include "pios_semaphore.h"
#include "pios_thread.h"
#include "pios_queue.h"
#include "physical_constants.h"
#include "taskmonitor.h"

#include "pios_bmx055_priv.h"

#define PIOS_BMX_TASK_PRIORITY   PIOS_THREAD_PRIO_HIGHEST
#define PIOS_BMX_TASK_STACK      640

#define PIOS_BMX_QUEUE_LEN       2

#define PIOS_BMX_SPI_SPEED       10000000


/**
 * Magic byte sequence used to validate the device state struct.
 * Should be unique amongst all PiOS drivers!
 */
enum pios_bmx055_dev_magic {
	PIOS_BMX_DEV_MAGIC = 0x58786d42 /**< Unique byte sequence 'BmxX' */
};

/**
 * @brief The device state struct
 */
struct pios_bmx055_dev {
	enum pios_bmx055_dev_magic magic;              /**< Magic bytes to validate the struct contents */
	const struct pios_bmx055_cfg *cfg;             /**< Device configuration structure */
	uint32_t spi_id;                            /**< Handle to the communication driver */
	uint32_t spi_slave_gyro;                    /**< The slave number (SPI) */
	uint32_t spi_slave_accel;

	struct pios_queue *gyro_queue;
	struct pios_queue *accel_queue;

	struct pios_thread *task_handle;
	struct pios_semaphore *data_ready_sema;
	volatile uint32_t interrupt_count;
};

//! Global structure for this device device
static struct pios_bmx055_dev *bmx_dev;

//! Private functions
/**
 * @brief Allocate a new device
 */
static struct pios_bmx055_dev *PIOS_BMX_Alloc(const struct pios_bmx055_cfg *cfg);

/**
 * @brief Validate the handle to the device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_BMX_Validate(struct pios_bmx055_dev *dev);

static void PIOS_BMX_Task(void *parameters);

/**
 * @brief Claim the SPI bus for the communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_BMX_ClaimBus(int slave);

/**
 * @brief Release the SPI bus for the communications and end the transaction
 * @return 0 if successful
 */
static int32_t PIOS_BMX_ReleaseBus(int slave);

static int32_t PIOS_BMX_ReadReg(int slave, uint8_t address, uint8_t *buffer);
static int32_t PIOS_BMX_WriteReg(int slave, uint8_t address, uint8_t buffer);

static struct pios_bmx055_dev *PIOS_BMX_Alloc(const struct pios_bmx055_cfg *cfg)
{
	struct pios_bmx055_dev *dev;

	dev = (struct pios_bmx055_dev *)PIOS_malloc(sizeof(*bmx_dev));
	if (!dev)
		return NULL;

	dev->magic = PIOS_BMX_DEV_MAGIC;

	dev->accel_queue = PIOS_Queue_Create(PIOS_BMX_QUEUE_LEN, sizeof(struct pios_sensor_accel_data));
	if (dev->accel_queue == NULL) {
		PIOS_free(dev);
		return NULL;
	}

	dev->gyro_queue = PIOS_Queue_Create(PIOS_BMX_QUEUE_LEN, sizeof(struct pios_sensor_gyro_data));
	if (dev->gyro_queue == NULL) {
		PIOS_Queue_Delete(dev->accel_queue);
		PIOS_free(dev);
		return NULL;
	}

	dev->data_ready_sema = PIOS_Semaphore_Create();
	if (dev->data_ready_sema == NULL) {
		PIOS_Queue_Delete(dev->accel_queue);
		PIOS_Queue_Delete(dev->gyro_queue);
		PIOS_free(dev);
		return NULL;
	}

	return dev;
}

static int32_t PIOS_BMX_Validate(struct pios_bmx055_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_BMX_DEV_MAGIC)
		return -2;
	if (dev->spi_id == 0)
		return -3;
	return 0;
}

static int32_t AssertReg(int slave, uint8_t address, uint8_t expect) {
	uint8_t c;

	int32_t ret = PIOS_BMX_ReadReg(slave, address, &c);

	if (ret) {
		return ret;
	}

	if (c != expect) {
		DEBUG_PRINTF(2, "BMX: Assertion failed: *(%d:%02x) == %02x (expect %02x)\n",
		slave, address, c, expect);
		return -1;
	}

	DEBUG_PRINTF(2, "BMX: Assertion passed: *(%d:%02x) == %02x\n", slave, address,
		expect);

	return 0;
}

int32_t PIOS_BMX055_SPI_Init(pios_bmx055_dev_t *dev, uint32_t spi_id, uint32_t slave_gyro, uint32_t slave_accel, const struct pios_bmx055_cfg *cfg)
{
	bmx_dev = PIOS_BMX_Alloc(cfg);
	if (bmx_dev == NULL)
		return -1;
	*dev = bmx_dev;

	bmx_dev->spi_id = spi_id;
	bmx_dev->spi_slave_gyro = slave_gyro;
	bmx_dev->spi_slave_accel = slave_accel;
	bmx_dev->cfg = cfg;

	int32_t ret;

	/* Verify presence/chip selects */
	ret = AssertReg(bmx_dev->spi_slave_gyro, BMX055_REG_GYRO_CHIPID,
		BMX055_VAL_GYRO_CHIPID);

	if (ret) return ret;

	ret = AssertReg(bmx_dev->spi_slave_accel, BMX055_REG_ACC_CHIPID,
		BMX055_VAL_ACC_CHIPID);

	if (ret) return ret;

	DEBUG_PRINTF(2, "BMX: Resetting sensors\n");

	ret = PIOS_BMX_WriteReg(bmx_dev->spi_slave_gyro,
			BMX055_REG_GYRO_BGW_SOFTRESET,
			BMX055_VAL_GYRO_BGW_SOFTRESET_REQ);

	if (ret) return ret;

	ret = PIOS_BMX_WriteReg(bmx_dev->spi_slave_accel,
			BMX055_REG_ACC_BGW_SOFTRESET,
			BMX055_VAL_ACC_BGW_SOFTRESET_REQ);

	if (ret) return ret;

	/* Gyro is very slow to wake up, so a big total delay here */
	PIOS_DELAY_WaitmS(50);

	/* Verify presence/chip selects again */
	ret = AssertReg(bmx_dev->spi_slave_gyro,
			BMX055_REG_GYRO_CHIPID,
			BMX055_VAL_GYRO_CHIPID);

	if (ret) return ret;

	ret = AssertReg(bmx_dev->spi_slave_accel,
			BMX055_REG_ACC_CHIPID,
			BMX055_VAL_ACC_CHIPID);

	if (ret) return ret;

	/* Program ODR, range, and bandwidth of accel */
	ret = PIOS_BMX_WriteReg(bmx_dev->spi_slave_accel,
			BMX055_REG_ACC_PMU_RANGE,
			BMX055_VAL_ACC_PMU_RANGE_16G);

	if (ret) return ret;

	ret = PIOS_BMX_WriteReg(bmx_dev->spi_slave_accel,
			BMX055_REG_ACC_PMU_BW,
			BMX055_VAL_ACC_PMU_BW_500HZ);

	if (ret) return ret;

	/* Program ODR, range, and bandwidth of gyro */
	ret = PIOS_BMX_WriteReg(bmx_dev->spi_slave_gyro,
			BMX055_REG_GYRO_RANGE,
			BMX055_VAL_GYRO_RANGE_2000DPS);

	if (ret) return ret;

	ret = PIOS_BMX_WriteReg(bmx_dev->spi_slave_gyro,
			BMX055_REG_GYRO_BW,
			BMX055_VAL_GYRO_BW_116HZ);

	if (ret) return ret;

#if 0
	/* Set up EXTI line */
	PIOS_EXTI_Init(bmx_dev->cfg->exti_cfg);

	/* Wait 20 ms for data ready interrupt and make sure it happens twice */
	if (!bmx_dev->cfg->skip_startup_irq_check) {
		for (int i=0; i<2; i++) {
			uint32_t ref_val = bmx_dev->interrupt_count;
			uint32_t raw_start = PIOS_DELAY_GetRaw();

			while (bmx_dev->interrupt_count == ref_val) {
				if (PIOS_DELAY_DiffuS(raw_start) > 20000) {
					PIOS_EXTI_DeInit(bmx_dev->cfg->exti_cfg);
					return -PIOS_BMX_ERROR_NOIRQ;
				}
			}
		}
	}
#endif

	bmx_dev->task_handle = PIOS_Thread_Create(
			PIOS_BMX_Task, "pios_bmx", PIOS_BMX_TASK_STACK,
			NULL, PIOS_BMX_TASK_PRIORITY);

	PIOS_Assert(bmx_dev->task_handle != NULL);
	TaskMonitorAdd(TASKINFO_RUNNING_IMU, bmx_dev->task_handle);

	PIOS_SENSORS_SetMaxGyro(2000);
	PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_GYRO, 800);
	PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_ACCEL, 800);

	PIOS_SENSORS_Register(PIOS_SENSOR_ACCEL, bmx_dev->accel_queue);
	PIOS_SENSORS_Register(PIOS_SENSOR_GYRO, bmx_dev->gyro_queue);

	return 0;
}

static int32_t PIOS_BMX_ClaimBus(int slave)
{
	if (PIOS_BMX_Validate(bmx_dev) != 0)
		return -1;

	if (PIOS_SPI_ClaimBus(bmx_dev->spi_id) != 0)
		return -2;

	PIOS_SPI_RC_PinSet(bmx_dev->spi_id, slave, false);

	return 0;
}

static int32_t PIOS_BMX_ReleaseBus(int slave)
{
	if (PIOS_BMX_Validate(bmx_dev) != 0)
		return -1;

	PIOS_SPI_RC_PinSet(bmx_dev->spi_id, slave, true);

	PIOS_SPI_ReleaseBus(bmx_dev->spi_id);

	return 0;
}

static int32_t PIOS_BMX_ReadReg(int slave, uint8_t address, uint8_t *buffer)
{
	if (PIOS_BMX_ClaimBus(slave) != 0)
		return -1;

	PIOS_SPI_TransferByte(bmx_dev->spi_id, 0x80 | address); // request byte
	*buffer = PIOS_SPI_TransferByte(bmx_dev->spi_id, 0);   // receive response

	PIOS_BMX_ReleaseBus(slave);

	return 0;
}

static int32_t PIOS_BMX_WriteReg(int slave, uint8_t address, uint8_t buffer)
{
	if (PIOS_BMX_ClaimBus(slave) != 0)
		return -1;

	PIOS_SPI_TransferByte(bmx_dev->spi_id, 0x7f & address);
	PIOS_SPI_TransferByte(bmx_dev->spi_id, buffer);

	PIOS_BMX_ReleaseBus(slave);

	return 0;
}

#if 0
bool PIOS_BMX_IRQHandler(void)
{
	if (PIOS_BMX_Validate(bmx_dev) != 0)
		return false;

	bool woken = false;

	bmx_dev->interrupt_count++;

	PIOS_Semaphore_Give_FromISR(bmx_dev->data_ready_sema, &woken);

	return woken;
}
#endif

static void PIOS_BMX_Task(void *parameters)
{
	(void)parameters;

	while (true) {
#if 0
		//Wait for data ready interrupt
		if (PIOS_Semaphore_Take(bmx_dev->data_ready_sema, PIOS_SEMAPHORE_TIMEOUT_MAX) != true)
			continue;
#endif
		PIOS_Thread_Sleep(1);		/* XXX */

		// claim bus in high speed mode
		if (PIOS_SPI_ClaimBus(bmx_dev->spi_id) != 0)
			continue;

		uint8_t acc_buf[BMX055_REG_ACC_TEMP - BMX055_REG_ACC_X_LSB + 1];
		uint8_t gyro_buf[BMX055_REG_GYRO_Z_MSB - BMX055_REG_GYRO_X_LSB + 1];

		PIOS_SPI_SetClockSpeed(bmx_dev->spi_id, 10000000);

		PIOS_SPI_RC_PinSet(bmx_dev->spi_id, bmx_dev->spi_slave_accel, 
			false);

		PIOS_SPI_TransferByte(bmx_dev->spi_id, 0x80 | BMX055_REG_ACC_X_LSB);

		if (PIOS_SPI_TransferBlock(bmx_dev->spi_id, NULL, acc_buf, sizeof(acc_buf)) < 0) {
			PIOS_BMX_ReleaseBus(false);
			continue;
		}

		PIOS_SPI_RC_PinSet(bmx_dev->spi_id, bmx_dev->spi_slave_accel, 
			true);
		PIOS_SPI_RC_PinSet(bmx_dev->spi_id, bmx_dev->spi_slave_gyro, 
			false);

		PIOS_SPI_TransferByte(bmx_dev->spi_id, 0x80 | BMX055_REG_GYRO_X_LSB);
		if (PIOS_SPI_TransferBlock(bmx_dev->spi_id, NULL, gyro_buf, sizeof(gyro_buf)) < 0) {
			PIOS_BMX_ReleaseBus(false);
			continue;
		}
		PIOS_SPI_RC_PinSet(bmx_dev->spi_id, bmx_dev->spi_slave_gyro, 
			true);

		PIOS_SPI_ReleaseBus(bmx_dev->spi_id);

		struct pios_sensor_accel_data accel_data;
		struct pios_sensor_gyro_data gyro_data;

#define PACK_REG12_ADDR_OFFSET(b, reg, off) (int16_t) ( (b[reg-off] & 0xf0) | (b[reg-off+1] << 8) )
#define PACK_REG16_ADDR_OFFSET(b, reg, off) (int16_t) ( (b[reg-off]) | (b[reg-off+1] << 8) )
		float accel_x = PACK_REG12_ADDR_OFFSET(acc_buf, BMX055_REG_ACC_X_LSB, BMX055_REG_ACC_X_LSB);
		float accel_y = PACK_REG12_ADDR_OFFSET(acc_buf, BMX055_REG_ACC_Y_LSB, BMX055_REG_ACC_X_LSB);
		float accel_z = PACK_REG12_ADDR_OFFSET(acc_buf, BMX055_REG_ACC_Z_LSB, BMX055_REG_ACC_X_LSB);
		int accel_temp = ((int8_t) acc_buf[BMX055_REG_ACC_TEMP - BMX055_REG_ACC_X_LSB]) + BMX055_ACC_TEMP_OFFSET;

		float gyro_x = PACK_REG16_ADDR_OFFSET(gyro_buf, BMX055_REG_GYRO_X_LSB, BMX055_REG_GYRO_X_LSB);
		float gyro_y = PACK_REG16_ADDR_OFFSET(gyro_buf, BMX055_REG_GYRO_Y_LSB, BMX055_REG_GYRO_X_LSB);
		float gyro_z = PACK_REG16_ADDR_OFFSET(gyro_buf, BMX055_REG_GYRO_Z_LSB, BMX055_REG_GYRO_X_LSB);

		// Rotate the sensor to our convention.  The datasheet defines X as towards the right
		// and Y as forward. Our convention transposes this.  Also the Z is defined negatively
		// to our convention. This is true for accels and gyros. Magnetometer corresponds our convention.
		switch (bmx_dev->cfg->orientation) {
		case PIOS_BMX_TOP_0DEG:
			accel_data.y =  accel_x;
			accel_data.x =  accel_y;
			accel_data.z = -accel_z;
			gyro_data.y  =  gyro_x;
			gyro_data.x  =  gyro_y;
			gyro_data.z  = -gyro_z;
			break;
		case PIOS_BMX_TOP_90DEG:
			accel_data.y = -accel_y;
			accel_data.x =  accel_x;
			accel_data.z = -accel_z;
			gyro_data.y  = -gyro_y;
			gyro_data.x  =  gyro_x;
			gyro_data.z  = -gyro_z;
			break;
		case PIOS_BMX_TOP_180DEG:
			accel_data.y = -accel_x;
			accel_data.x = -accel_y;
			accel_data.z = -accel_z;
			gyro_data.y  = -gyro_x;
			gyro_data.x  = -gyro_y;
			gyro_data.z  = -gyro_z;
			break;
		case PIOS_BMX_TOP_270DEG:
			accel_data.y =  accel_y;
			accel_data.x = -accel_x;
			accel_data.z = -accel_z;
			gyro_data.y  =  gyro_y;
			gyro_data.x  = -gyro_x;
			gyro_data.z  = -gyro_z;
			break;
		case PIOS_BMX_BOTTOM_0DEG:
			accel_data.y = -accel_x;
			accel_data.x =  accel_y;
			accel_data.z =  accel_z;
			gyro_data.y  = -gyro_x;
			gyro_data.x  =  gyro_y;
			gyro_data.z  =  gyro_z;
			break;
		case PIOS_BMX_BOTTOM_90DEG:
			accel_data.y =  accel_y;
			accel_data.x =  accel_x;
			accel_data.z =  accel_z;
			gyro_data.y  =  gyro_y;
			gyro_data.x  =  gyro_x;
			gyro_data.z  =  gyro_z;
			break;
		case PIOS_BMX_BOTTOM_180DEG:
			accel_data.y =  accel_x;
			accel_data.x = -accel_y;
			accel_data.z =  accel_z;
			gyro_data.y  =  gyro_x;
			gyro_data.x  = -gyro_y;
			gyro_data.z  =  gyro_z;
			break;
		case PIOS_BMX_BOTTOM_270DEG:
			accel_data.y = -accel_y;
			accel_data.x = -accel_x;
			accel_data.z =  accel_z;
			gyro_data.y  = -gyro_y;
			gyro_data.x  = -gyro_x;
			gyro_data.z  =  gyro_z;
			break;
		}

		// Apply sensor scaling
		// At 16G setting, BMX055 is 128 lsb/G.
		// Plus there's 4 bits of left adjustment, so 2048 counts/g.
		// 
		float accel_scale = GRAVITY/2048;
		accel_data.x *= accel_scale;
		accel_data.y *= accel_scale;
		accel_data.z *= accel_scale;
		accel_data.temperature = accel_temp;

		float gyro_scale = 2000.0f / 32768; 
		gyro_data.x *= gyro_scale;
		gyro_data.y *= gyro_scale;
		gyro_data.z *= gyro_scale;
		gyro_data.temperature = accel_temp;

		PIOS_Queue_Send(bmx_dev->accel_queue, &accel_data, 0);
		PIOS_Queue_Send(bmx_dev->gyro_queue, &gyro_data, 0);
	}
}

#endif // PIOS_INCLUDE_BMX055

/**
 * @}
 * @}
 */
