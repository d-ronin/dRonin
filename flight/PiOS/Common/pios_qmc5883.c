/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_QMC5883 QMC5883 Functions
 * @brief Deals with the hardware interface to the magnetometers
 * @{
 * @file       pios_qmc5883.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @brief      QMC5883 Magnetic Sensor Functions from AHRS
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************
 */
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
 */

/* Project Includes */
#include "pios.h"
#include "pios_qmc5883_priv.h"

#if defined(PIOS_INCLUDE_QMC5883)

#include "pios_semaphore.h"
#include "pios_thread.h"
#include "pios_queue.h"

/* Private constants */
#define QMC5883_TASK_PRIORITY        PIOS_THREAD_PRIO_HIGHEST
#define QMC5883_TASK_STACK_BYTES     512
#define PIOS_QMC5883_MAX_DOWNSAMPLE  1

/* Global Variables */

/* Local Types */
enum pios_qmc5883_dev_magic {
	PIOS_QMC5883_DEV_MAGIC = 0x514d4335,  // QMC5
};

struct qmc5883_dev {
	pios_i2c_t i2c_id;
	const struct pios_qmc5883_cfg *cfg;
	struct pios_queue *queue;
	struct pios_thread *task;
	struct pios_semaphore *data_ready_sema;
	enum pios_qmc5883_dev_magic magic;
	enum pios_qmc5883_orientation orientation;
};

/* Local Variables */
static int32_t PIOS_QMC5883_Config(const struct pios_qmc5883_cfg * cfg);
static int32_t PIOS_QMC5883_Read(uint8_t address, uint8_t * buffer, uint8_t len);
static int32_t PIOS_QMC5883_Write(uint8_t address, uint8_t buffer);
static void PIOS_QMC5883_Task(void *parameters);

static struct qmc5883_dev *dev;

/**
 * @brief Allocate a new device
 */
static struct qmc5883_dev * PIOS_QMC5883_alloc(void)
{
	struct qmc5883_dev *qmc5883_dev;
	
	qmc5883_dev = (struct qmc5883_dev *)PIOS_malloc(sizeof(*qmc5883_dev));
	if (!qmc5883_dev) return (NULL);
	
	qmc5883_dev->magic = PIOS_QMC5883_DEV_MAGIC;
	
	qmc5883_dev->queue = PIOS_Queue_Create(PIOS_QMC5883_MAX_DOWNSAMPLE, sizeof(struct pios_sensor_mag_data));
	if (qmc5883_dev->queue == NULL) {
		PIOS_free(qmc5883_dev);
		return NULL;
	}

	return qmc5883_dev;
}

/**
 * @brief Validate the handle to the i2c device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_QMC5883_Validate(struct qmc5883_dev *dev)
{
	if (dev == NULL) 
		return -1;
	if (dev->magic != PIOS_QMC5883_DEV_MAGIC)
		return -2;
	if (dev->i2c_id == 0)
		return -3;
	return 0;
}

/**
 * @brief Initialize the QMC5883 magnetometer sensor.
 * @return 0 on success
 */
int32_t PIOS_QMC5883_Init(pios_i2c_t i2c_id, const struct pios_qmc5883_cfg *cfg)
{
	dev = (struct qmc5883_dev *) PIOS_QMC5883_alloc();
	if (dev == NULL)
		return -1;

	dev->cfg = cfg;
	dev->i2c_id = i2c_id;
	dev->orientation = cfg->Default_Orientation;

	/* check if we are using an irq line */
#ifndef PIOS_QMC5883_NO_EXTI
	if (cfg->exti_cfg != NULL) {
		PIOS_EXTI_Init(cfg->exti_cfg);

		dev->data_ready_sema = PIOS_Semaphore_Create();
		PIOS_Assert(dev->data_ready_sema != NULL);
	}
	else {
		dev->data_ready_sema = NULL;
	}
#endif

	if (PIOS_QMC5883_Config(cfg) != 0)
		return -2;

	PIOS_SENSORS_Register(PIOS_SENSOR_MAG, dev->queue);

	dev->task = PIOS_Thread_Create(PIOS_QMC5883_Task, "pios_qmc5883", QMC5883_TASK_STACK_BYTES, NULL, QMC5883_TASK_PRIORITY);

	PIOS_Assert(dev->task != NULL);

	return 0;
}

/**
 * @brief Updates the QMC5883 chip orientation.
 * @returns 0 for success or -1 for failure
 */
int32_t PIOS_QMC5883_SetOrientation(enum pios_qmc5883_orientation orientation)
{
	if (PIOS_QMC5883_Validate(dev) != 0)
		return -1;

	dev->orientation = orientation;

	return 0;
}

/**
 * @brief Initialize the QMC5883 magnetometer sensor
 * \return none
 * \param[in] PIOS_QMC5883_ConfigTypeDef struct to be used to configure sensor.
 */
static int32_t PIOS_QMC5883_Config(const struct pios_qmc5883_cfg * cfg)
{
	// Set/Reset Period Register
	if (PIOS_QMC5883_Write(PIOS_QMC5883_SET_RESET_PERIOD_REG, PIOS_QMC5883_SET_RESET_PERIOD) != 0)
		return -1;
	
	// Control Register 1
	if (PIOS_QMC5883_Write(PIOS_QMC5883_CONTROL_REG_1, PIOS_QMC5883_MODE_CONTINUOUS | 
	                                                   cfg->ODR | 
	                                                   PIOS_QMC5883_OSR_512 | 
	                                                   PIOS_QMC5883_RNG_8G) != 0)
		return -1;
	
	return 0;
}

/**
 * @brief Read current X, Z, Y values (in that order)
 * \param[out] int16_t array of size 3 to store X, Z, and Y magnetometer readings
 * \return 0 for success or -1 for failure
 */
static int32_t PIOS_QMC5883_ReadMag(struct pios_sensor_mag_data *mag_data)
{
	if (PIOS_QMC5883_Validate(dev) != 0)
		return -1;

	/* don't use PIOS_QMC5883_Read and PIOS_QMC5883_Write here because the task could be
	 * switched out of context in between which would give the sensor less time to capture
	 * the next sample.
	 */
	uint8_t addr_read = PIOS_QMC5883_DATAOUT_XLSB_REG;
	uint8_t buffer_read[6];

	const struct pios_i2c_txn txn_list[] = {
		{
			.info = __func__,
			.addr = PIOS_QMC5883_I2C_ADDR,
			.rw = PIOS_I2C_TXN_WRITE,
			.len = sizeof(addr_read),
			.buf = &addr_read,
		},
		{
			.info = __func__,
			.addr = PIOS_QMC5883_I2C_ADDR,
			.rw = PIOS_I2C_TXN_READ,
			.len = sizeof(buffer_read),
			.buf = buffer_read,
		},
	};

	if (PIOS_I2C_Transfer(dev->i2c_id, txn_list, NELEMENTS(txn_list)) != 0)
		return -1;

	int16_t mag_x, mag_y, mag_z;
	mag_x = ((int16_t) ((uint16_t)buffer_read[1] << 8) + buffer_read[0]);
	mag_y = ((int16_t) ((uint16_t)buffer_read[3] << 8) + buffer_read[2]);
	mag_z = ((int16_t) ((uint16_t)buffer_read[5] << 8) + buffer_read[4]);
	
	// Define "0" when the fiducial is in the front left of the board
	switch (dev->orientation) {
		case PIOS_QMC5883_TOP_0DEG:
			mag_data->x = -mag_x;
			mag_data->y = mag_y;
			mag_data->z = -mag_z;
			break;
		case PIOS_QMC5883_TOP_90DEG:
			mag_data->x = -mag_y;
			mag_data->y = -mag_x;
			mag_data->z = -mag_z;
			break;
		case PIOS_QMC5883_TOP_180DEG:
			mag_data->x = mag_x;
			mag_data->y = -mag_y;
			mag_data->z = -mag_z;
			break;
		case PIOS_QMC5883_TOP_270DEG:
			mag_data->x = mag_y;
			mag_data->y = mag_x;
			mag_data->z = -mag_z;
			break;
		case PIOS_QMC5883_BOTTOM_0DEG:
			mag_data->x = -mag_x;
			mag_data->y = -mag_y;
			mag_data->z = mag_z;
			break;
		case PIOS_QMC5883_BOTTOM_90DEG:
			mag_data->x = mag_y;
			mag_data->y = -mag_x;
			mag_data->z = mag_z;
			break;
		case PIOS_QMC5883_BOTTOM_180DEG:
			mag_data->x = mag_x;
			mag_data->y = mag_y;
			mag_data->z = mag_z;
			break;
		case PIOS_QMC5883_BOTTOM_270DEG:
			mag_data->x = -mag_y;
			mag_data->y = mag_x;
			mag_data->z = mag_z;
			break;
	}
	
	return 0;
}


/**
 * @brief Read the identification bytes from the QMC5883 sensor
 * \param[out] uint8_t array of size 4 to store QMC5883 ID.
 * \return 0 if successful, -1 if not
 */
static uint8_t PIOS_QMC5883_ReadID(uint8_t * out)
{
	uint8_t retval = PIOS_QMC5883_Read(PIOS_QMC5883_ID_REG, out, 1);
	return retval;
}


/**
 * @brief Reads one or more bytes into a buffer
 * \param[in] address QMC5883 register address (depends on size)
 * \param[out] buffer destination buffer
 * \param[in] len number of bytes which should be read
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 * \return -2 if unable to claim i2c device
 */
static int32_t PIOS_QMC5883_Read(uint8_t address, uint8_t * buffer, uint8_t len)
{
	if(PIOS_QMC5883_Validate(dev) != 0)
		return -1;

	uint8_t addr_buffer[] = {
		address,
	};
	
	const struct pios_i2c_txn txn_list[] = {
		{
			.info = __func__,
			.addr = PIOS_QMC5883_I2C_ADDR,
			.rw = PIOS_I2C_TXN_WRITE,
			.len = sizeof(addr_buffer),
			.buf = addr_buffer,
		}
		,
		{
			.info = __func__,
			.addr = PIOS_QMC5883_I2C_ADDR,
			.rw = PIOS_I2C_TXN_READ,
			.len = len,
			.buf = buffer,
		}
	};
	
	return PIOS_I2C_Transfer(dev->i2c_id, txn_list, NELEMENTS(txn_list));
}

/**
 * @brief Writes one or more bytes to the QMC5883
 * \param[in] address Register address
 * \param[in] buffer source buffer
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 * \return -2 if unable to claim i2c device
 */
static int32_t PIOS_QMC5883_Write(uint8_t address, uint8_t buffer)
{
	if(PIOS_QMC5883_Validate(dev) != 0)
		return -1;

	uint8_t data[] = {
		address,
		buffer,
	};
	
	const struct pios_i2c_txn txn_list[] = {
		{
			.info = __func__,
			.addr = PIOS_QMC5883_I2C_ADDR,
			.rw = PIOS_I2C_TXN_WRITE,
			.len = sizeof(data),
			.buf = data,
		}
		,
	};

	return PIOS_I2C_Transfer(dev->i2c_id, txn_list, NELEMENTS(txn_list));
}

/**
 * @brief Run self-test operation.  Do not call this during operational use!!
 * \return 0 if success, -1 if test failed
 */
int32_t PIOS_QMC5883_Test(void)
{
	/* Verify that ID matches */
	uint8_t id;
	PIOS_QMC5883_ReadID(&id);
	if (id != PIOS_QMC5883_ID) 
		return -1;

	return 0;
}

/**
 * @brief IRQ Handler
 */
#ifndef PIOS_QMC5883_NO_EXTI
bool PIOS_QMC5883_IRQHandler(void)
{
	if (PIOS_QMC5883_Validate(dev) != 0)
		return false;

	bool woken = false;
	PIOS_Semaphore_Give_FromISR(dev->data_ready_sema, &woken);

	return woken;
}
#endif

/**
 * The QMC5883 task
 */
static void PIOS_QMC5883_Task(void *parameters)
{
	while (PIOS_QMC5883_Validate(dev) != 0) {
		PIOS_Thread_Sleep(100);
	}

	uint32_t sample_delay;

	switch (dev->cfg->ODR) {
	case PIOS_QMC5883_ODR_10HZ:
		sample_delay = 100;
		break;
	case PIOS_QMC5883_ODR_50HZ:
		sample_delay = 20;
		break;
	case PIOS_QMC5883_ODR_200HZ:
		sample_delay = 5;
		break;
	case PIOS_QMC5883_ODR_100HZ:
	default:
		sample_delay = 10;
		break;
	}

	uint32_t now = PIOS_Thread_Systime();

	while (1) {
		if ((dev->data_ready_sema != NULL) && (dev->cfg->Mode == PIOS_QMC5883_MODE_CONTINUOUS)) {
			if (PIOS_Semaphore_Take(dev->data_ready_sema, PIOS_SEMAPHORE_TIMEOUT_MAX) != true) {
				PIOS_Thread_Sleep(100);
				continue;
			}
		} else {
			PIOS_Thread_Sleep_Until(&now, sample_delay);
		}

		struct pios_sensor_mag_data mag_data;
		if (PIOS_QMC5883_ReadMag(&mag_data) == 0)
			PIOS_Queue_Send(dev->queue, &mag_data, 0);
	}
}

#endif /* PIOS_INCLUDE_QMC5883 */

/**
 * @}
 * @}
 */
