/**
 ******************************************************************************
 * @file       pios_mpu.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MPU Invensense MPU Functions
 * @{
 * @brief Common driver for Invensense MPU-xxx0 6/9-axis IMUs
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "openpilot.h"
#include "pios.h"

#if defined(PIOS_INCLUDE_MPU)

#include "pios_semaphore.h"
#include "pios_thread.h"
#include "pios_queue.h"
#include "physical_constants.h"
#include "taskmonitor.h"

#include "pios_mpu_priv.h"

#define PIOS_MPU_TASK_PRIORITY   PIOS_THREAD_PRIO_HIGHEST
#ifdef PIOS_INCLUDE_MPU_MAG
#define PIOS_MPU_TASK_STACK      600
#else
#define PIOS_MPU_TASK_STACK      512
#endif // PIOS_INCLUDE_MPU_MAG

#define PIOS_MPU_QUEUE_LEN       2

#ifndef PIOS_MPU_SPI_HIGH_SPEED
#define PIOS_MPU_SPI_HIGH_SPEED              20000000	// should result in 10.5MHz clock on F4 targets like Sparky2
#endif // PIOS_MPU_SPI_HIGH_SPEED
#define PIOS_MPU_SPI_LOW_SPEED               300000


/**
 * WHOAMI ids of each device, must be same length as pios_mpu_type
 */
static const uint8_t pios_mpu_whoami[PIOS_MPU_NUM] = {
	[PIOS_MPU60X0] = 0x68,
	[PIOS_MPU6500] = 0x70,
	[PIOS_MPU9150] = 0x68,
	[PIOS_MPU9250] = 0x71,
	[PIOS_ICM20608G] = 0xAF,
};

/**
 * I2C addresses to probe for device
 */
static const uint8_t pios_mpu_i2c_addr[] = {
	0x68,
	0x69,
};

/**
 * The available underlying communication drivers
 */
enum pios_mpu_com_driver {
	PIOS_MPU_COM_I2C, /**< I2C driver */
	PIOS_MPU_COM_SPI,  /**< SPI driver */
};

/**
 * Magic byte sequence used to validate the device state struct.
 * Should be unique amongst all PiOS drivers!
 */
enum pios_mpu_dev_magic {
	PIOS_MPU_DEV_MAGIC = 0xa9dd94b4 /**< Unique byte sequence */
};

/**
 * @brief The device state struct
 */
struct pios_mpu_dev {
	const struct pios_mpu_cfg *cfg;      /**< Device configuration structure */
	enum pios_mpu_type mpu_type;              /**< The IMU device type */
	enum pios_mpu_com_driver com_driver_type;   /**< Communication driver type */
	uint32_t com_driver_id;              /**< Handle to the communication driver */
	uint32_t com_slave_addr;             /**< The slave address (I2C) or number (SPI) */
#ifdef PIOS_INCLUDE_MPU_MAG
	bool use_mag;
	struct pios_queue *mag_queue;
#endif // PIOS_INCLUDE_MPU_MAG
	struct pios_queue *gyro_queue;
	struct pios_queue *accel_queue;
	struct pios_thread *task_handle;
	struct pios_semaphore *data_ready_sema;
	enum pios_mpu_gyro_range gyro_range;
	enum pios_mpu_accel_range accel_range;
	enum pios_mpu_dev_magic magic;       /**< Magic bytes to validate the struct contents */
};


//! Global structure for this device device
static struct pios_mpu_dev *mpu_dev;

//! Private functions
/**
 * @brief Allocate a new device
 */
static struct pios_mpu_dev *PIOS_MPU_Alloc(const struct pios_mpu_cfg *cfg);

#ifdef PIOS_INCLUDE_MPU_MAG
/**
 * @brief Allocate a a magnetometer queue
 */
static bool PIOS_MPU_Mag_Alloc(struct pios_mpu_dev *mpu_dev);
static int32_t PIOS_MPU_Mag_Probe(void);
#endif // PIOS_INCLUDE_MPU_MAG
/**
 * @brief Validate the handle to the device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_MPU_Validate(struct pios_mpu_dev *dev);
/**
 * @brief Initialize the MPU gyro & accel registers
 * @param[in] pios_mpu_cfg struct to be used to configure sensor.
 * @return 0 if successful
 */
static int32_t PIOS_MPU_Config(struct pios_mpu_cfg const *cfg);
static void PIOS_MPU_Task(void *parameters);
static int32_t PIOS_MPU_ReadReg(uint8_t reg);
static int32_t PIOS_MPU_WriteReg(uint8_t reg, uint8_t data);

#if defined(PIOS_INCLUDE_SPI) || defined(__DOXYGEN__)
/**
 * @brief Claim the SPI bus for the communications and select this chip
 * \param[in] flag controls if low speed access for control registers should be used
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_MPU_ClaimBus(bool lowspeed);
/**
 * @brief Release the SPI bus for the communications and end the transaction
 * \param[in] must be true when bus was claimed in lowspeed mode
 * @return 0 if successful
 */
static int32_t PIOS_MPU_ReleaseBus(bool lowspeed);
/**
 * @brief Probe the SPI bus for an MPU device
 * @param[out] Detected device type, only valid on success
 * @retval 0 success, -1 failure
 */
static int32_t PIOS_MPU_SPI_Probe(enum pios_mpu_type *detected_device);
#endif // defined(PIOS_INCLUDE_SPI) || defined(__DOXYGEN__)

#if defined(PIOS_INCLUDE_I2C) || defined(__DOXYGEN__)
/**
 * @brief Probe the I2C bus for an MPU device
 * @param[out] Detected device type, only valid on success
 * @retval 0 success, -1 failure
 */
static int32_t PIOS_MPU_I2C_Probe(enum pios_mpu_type *detected_device);
#endif // defined(PIOS_INCLUDE_I2C) || defined(__DOXYGEN__)


static struct pios_mpu_dev *PIOS_MPU_Alloc(const struct pios_mpu_cfg *cfg)
{
	struct pios_mpu_dev *dev;

	dev = (struct pios_mpu_dev *)PIOS_malloc(sizeof(*mpu_dev));
	if (!dev)
		return NULL;

	dev->magic = PIOS_MPU_DEV_MAGIC;

	dev->accel_queue = PIOS_Queue_Create(PIOS_MPU_QUEUE_LEN, sizeof(struct pios_sensor_accel_data));
	if (dev->accel_queue == NULL) {
		PIOS_free(dev);
		return NULL;
	}

	dev->gyro_queue = PIOS_Queue_Create(PIOS_MPU_QUEUE_LEN, sizeof(struct pios_sensor_gyro_data));
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

static int32_t PIOS_MPU_Validate(struct pios_mpu_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_MPU_DEV_MAGIC)
		return -2;
	if (dev->com_driver_id == 0)
		return -3;
	return 0;
}

static int32_t PIOS_MPU_CheckWhoAmI(enum pios_mpu_type *detected_device)
{
	// mask the reserved bit
	int32_t retval = PIOS_MPU_ReadReg(PIOS_MPU_WHOAMI);
	if (retval < 0)
		return -PIOS_MPU_ERROR_READFAILED;
	for (int i = 0; i < PIOS_MPU_NUM; i++) {
		if ((uint8_t)(retval & 0x7E) == pios_mpu_whoami[i]) {
			*detected_device = i;
			return 0;
		}
	}

	return -PIOS_MPU_ERROR_WHOAMI;
}

static int32_t PIOS_MPU_Config(struct pios_mpu_cfg const *cfg)
{
	// reset chip
	if (PIOS_MPU_WriteReg(PIOS_MPU_PWR_MGMT_REG, PIOS_MPU_PWRMGMT_IMU_RST) != 0)
		return -PIOS_MPU_ERROR_WRITEFAILED;

	// give chip some time to initialize
	PIOS_DELAY_WaitmS(50);

	// power management config
	if (PIOS_MPU_WriteReg(PIOS_MPU_PWR_MGMT_REG, PIOS_MPU_PWRMGMT_PLL_X_CLK) != 0)
		return -PIOS_MPU_ERROR_WRITEFAILED;

	// user control
	if (mpu_dev->com_driver_type == PIOS_MPU_COM_SPI) {
		if (PIOS_MPU_WriteReg(PIOS_MPU_USER_CTRL_REG, PIOS_MPU_USERCTL_DIS_I2C | PIOS_MPU_USERCTL_I2C_MST_EN) != 0)
			return -PIOS_MPU_ERROR_WRITEFAILED;
	} else {
		if (PIOS_MPU_WriteReg(PIOS_MPU_USER_CTRL_REG, PIOS_MPU_USERCTL_I2C_MST_EN) != 0)
			return -PIOS_MPU_ERROR_WRITEFAILED;
	}

	// Digital low-pass filter and scale
	// set this before sample rate else sample rate calculation will fail
	PIOS_MPU_SetGyroBandwidth(184);
	PIOS_MPU_SetAccelBandwidth(184);

	// Sample rate
	if (PIOS_MPU_SetSampleRate(cfg->default_samplerate) != 0)
		return -PIOS_MPU_ERROR_SAMPLERATE;

	// Set the gyro scale
	PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_1000_DEG);

	// Set the accel scale
	PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_8G);

	// Interrupt configuration
	PIOS_MPU_WriteReg(PIOS_MPU_INT_CFG_REG, PIOS_MPU_INT_CLR_ANYRD);

	// Interrupt enable
	PIOS_MPU_WriteReg(PIOS_MPU_INT_EN_REG, PIOS_MPU_INTEN_DATA_RDY);

	return 0;
}

#ifdef PIOS_INCLUDE_MPU_MAG
/**
 * @brief Writes one byte to the AK8xxx register using MPU I2C master
 * \param[in] reg Register address
 * \param[in] data Byte to write
 * @returns 0 when success
 */
static int32_t PIOS_MPU_Mag_WriteReg(uint8_t reg, uint8_t data)
{
	// we will use I2C SLV4 to manipulate with AK8963 control registers
	if (PIOS_MPU_WriteReg(PIOS_MPU_SLV4_REG_REG, reg) != 0)
		return -1;
	PIOS_MPU_WriteReg(PIOS_MPU_SLV4_ADDR_REG, PIOS_MPU_AK89XX_ADDR);
	PIOS_MPU_WriteReg(PIOS_MPU_SLV4_DO_REG, data);
	PIOS_MPU_WriteReg(PIOS_MPU_SLV4_CTRL_REG, PIOS_MPU_I2CSLV_EN);

	// wait for I2C transaction done, use simple safety
	// escape counter to prevent endless loop in case
	// MPU is broken
	uint8_t status = 0;
	uint32_t timeout = 0;
	do {
		if (timeout++ > 50)
			return -2;

		status = PIOS_MPU_ReadReg(PIOS_MPU_I2C_MST_STATUS_REG);
	} while ((status & PIOS_MPU_I2C_MST_SLV4_DONE) == 0);

	if (status & PIOS_MPU_I2C_MST_SLV4_NACK)
		return -3;

	return 0;
}

/**
 * @brief Reads one byte from the AK8xxx register using MPU I2C master
 * \param[in] reg Register address
 * \param[in] data Byte to write
 */
static uint8_t PIOS_MPU_Mag_ReadReg(uint8_t reg)
{
	// we will use I2C SLV4 to manipulate with AK8xxx control registers
	PIOS_MPU_WriteReg(PIOS_MPU_SLV4_REG_REG, reg);
	PIOS_MPU_WriteReg(PIOS_MPU_SLV4_ADDR_REG, PIOS_MPU_AK89XX_ADDR | 0x80);
	PIOS_MPU_WriteReg(PIOS_MPU_SLV4_CTRL_REG, PIOS_MPU_I2CSLV_EN);

	// wait for I2C transaction done, use simple safety
	// escape counter to prevent endless loop in case
	// MPU is broken
	uint8_t status = 0;
	uint32_t timeout = 0;
	do {
		if (timeout++ > 50)
			return 0;

		status = PIOS_MPU_ReadReg(PIOS_MPU_I2C_MST_STATUS_REG);
	} while ((status & PIOS_MPU_I2C_MST_SLV4_DONE) == 0);

	return PIOS_MPU_ReadReg(PIOS_MPU_SLV4_DI_REG);
}

/**
 * @brief Initialize the AK89xx magnetometer inside MPU9x50
 * \return 0 if success
 *
 */
static int32_t PIOS_MPU_Mag_Config(void)
{
	if (mpu_dev->mpu_type == PIOS_MPU9250) {
		// reset AK8963
		if (PIOS_MPU_Mag_WriteReg(PIOS_MPU_AK8963_CNTL2_REG, PIOS_MPU_AK8963_CNTL2_SRST) != 0)
			return -3;
	}
	// AK8975 (MPU-9150) doesn't have soft reset function

	// give chip some time to initialize
	PIOS_DELAY_WaitmS(2);

	// set magnetometer sampling rate to 100Hz and 16-bit resolution
	if (mpu_dev->mpu_type == PIOS_MPU9250)
		PIOS_MPU_Mag_WriteReg(PIOS_MPU_AK89XX_CNTL1_REG, PIOS_MPU_AK8963_MODE_CONTINUOUS_FAST_16B);
	else
		PIOS_MPU_Mag_WriteReg(PIOS_MPU_AK89XX_CNTL1_REG, PIOS_MPU_AK8975_MODE_SINGLE_12B);

	// configure mpu to read ak8963 data range from STATUS1 to STATUS2 at ODR
	PIOS_MPU_WriteReg(PIOS_MPU_SLV0_REG_REG, PIOS_MPU_AK89XX_ST1_REG);
	PIOS_MPU_WriteReg(PIOS_MPU_SLV0_ADDR_REG, PIOS_MPU_AK89XX_ADDR | 0x80);
	PIOS_MPU_WriteReg(PIOS_MPU_SLV0_CTRL_REG, PIOS_MPU_I2CSLV_EN | 8);

	return 0;
}

static bool PIOS_MPU_Mag_Alloc(struct pios_mpu_dev *dev)
{
	dev->mag_queue = PIOS_Queue_Create(PIOS_MPU_QUEUE_LEN, sizeof(struct pios_sensor_mag_data));
	if (dev->mag_queue == NULL) {
		dev->use_mag = false;
		return false;
	}
	dev->use_mag = true;
	return true;
}

static int32_t PIOS_MPU_Mag_Probe(void)
{
	/* probe for mag whomai */
	uint8_t id = PIOS_MPU_Mag_ReadReg(PIOS_MPU_AK89XX_WHOAMI_REG);
	if (id != PIOS_MPU_AK89XX_WHOAMI_ID)
		return -2;

	return 0;
}
#endif // PIOS_INCLUDE_MPU_MAG

static int32_t PIOS_MPU_Common_Init(void)
{
	/* Configure the MPU Sensor */
	if (PIOS_MPU_Config(mpu_dev->cfg) != 0)
		return -PIOS_MPU_ERROR_NOCONFIG;

#ifdef PIOS_INCLUDE_MPU_MAG
	/* Probe for mag */
	if (mpu_dev->cfg->use_internal_mag && PIOS_MPU_Mag_Probe() == 0) {
		PIOS_MPU_Mag_Alloc(mpu_dev);
		if (PIOS_MPU_Mag_Config() != 0)
			return -PIOS_MPU_ERROR_MAGFAILED;
		if (mpu_dev->mpu_type == PIOS_MPU60X0)
			mpu_dev->mpu_type = PIOS_MPU9150;
	} else {
		mpu_dev->use_mag = false;
		if (mpu_dev->mpu_type == PIOS_MPU9250)
			return -PIOS_MPU_ERROR_MAGFAILED;
	}
#endif // PIOS_INCLUDE_MPU_MAG

	/* Set up EXTI line */
	PIOS_EXTI_Init(mpu_dev->cfg->exti_cfg);

	/* Wait 20 ms for data ready interrupt and make sure it happens twice */
	if ((PIOS_Semaphore_Take(mpu_dev->data_ready_sema, 20) != true) ||
			(PIOS_Semaphore_Take(mpu_dev->data_ready_sema, 20) != true)) {
		PIOS_EXTI_DeInit(mpu_dev->cfg->exti_cfg);
		return -PIOS_MPU_ERROR_NOIRQ;
	}

	mpu_dev->task_handle = PIOS_Thread_Create(
			PIOS_MPU_Task, "pios_mpu", PIOS_MPU_TASK_STACK, NULL, PIOS_MPU_TASK_PRIORITY);
	PIOS_Assert(mpu_dev->task_handle != NULL);
	TaskMonitorAdd(TASKINFO_RUNNING_IMU, mpu_dev->task_handle);

	PIOS_SENSORS_Register(PIOS_SENSOR_ACCEL, mpu_dev->accel_queue);
	PIOS_SENSORS_Register(PIOS_SENSOR_GYRO, mpu_dev->gyro_queue);
#ifdef PIOS_INCLUDE_MPU_MAG
	if (mpu_dev->use_mag)
		PIOS_SENSORS_Register(PIOS_SENSOR_MAG, mpu_dev->mag_queue);
#endif // PIOS_INCLUDE_MPU_MAG

	mpu_dev->accel_range = PIOS_MPU_SCALE_8G;
	mpu_dev->gyro_range = PIOS_MPU_SCALE_1000_DEG;

	return 0;
}

/**
 * @brief Set gyroscope range
 * @returns 0 if successful
 * @param range[in] gyroscope range
 */
int32_t PIOS_MPU_SetGyroRange(enum pios_mpu_gyro_range range)
{
	if (PIOS_MPU_WriteReg(PIOS_MPU_GYRO_CFG_REG, range) != 0)
		return -PIOS_MPU_ERROR_WRITEFAILED;

	switch (range) {
	case PIOS_MPU_SCALE_250_DEG:
		PIOS_SENSORS_SetMaxGyro(250);
		break;
	case PIOS_MPU_SCALE_500_DEG:
		PIOS_SENSORS_SetMaxGyro(500);
		break;
	case PIOS_MPU_SCALE_1000_DEG:
		PIOS_SENSORS_SetMaxGyro(1000);
		break;
	case PIOS_MPU_SCALE_2000_DEG:
		PIOS_SENSORS_SetMaxGyro(2000);
		break;
	}

	mpu_dev->gyro_range = range;

	return 0;
}

/**
 * @brief Set accelerometer range
 * @returns 0 if success
 * @param range[in] accelerometer range
 */
int32_t PIOS_MPU_SetAccelRange(enum pios_mpu_accel_range range)
{
	if (PIOS_MPU_WriteReg(PIOS_MPU_ACCEL_CFG_REG, range) != 0)
		return -PIOS_MPU_ERROR_WRITEFAILED;

	mpu_dev->accel_range = range;

	return 0;
}

/**
 * @brief Get current gyro scale for deg/s
 * @returns scale
 */
static float PIOS_MPU_GetGyroScale(void)
{
	switch (mpu_dev->gyro_range) {
	case PIOS_MPU_SCALE_250_DEG:
		return 1.0f / 131.0f;
	case PIOS_MPU_SCALE_500_DEG:
		return 1.0f / 65.5f;
	case PIOS_MPU_SCALE_1000_DEG:
		return 1.0f / 32.8f;
	case PIOS_MPU_SCALE_2000_DEG:
		return 1.0f / 16.4f;
	}
	return 0;
}

/**
 * @brief Get current gyro scale for ms^-2
 * @returns scale
 */
static float PIOS_MPU_GetAccelScale(void)
{
	switch (mpu_dev->accel_range) {
	case PIOS_MPU_SCALE_2G:
		return GRAVITY / 16384.0f;
	case PIOS_MPU_SCALE_4G:
		return GRAVITY / 8192.0f;
	case PIOS_MPU_SCALE_8G:
		return GRAVITY / 4096.0f;
	case PIOS_MPU_SCALE_16G:
		return GRAVITY / 2048.0f;
	}
	return 0;
}

void PIOS_MPU_SetGyroBandwidth(uint16_t bandwidth)
{
	uint8_t filter;
	// TODO: investigate 250/256 Hz (Fs=8 kHz, aliasing?)
	if (mpu_dev->mpu_type == PIOS_MPU6500 || mpu_dev->mpu_type == PIOS_MPU9250) {
		if (bandwidth <= 5)
			filter = PIOS_MPU6500_GYRO_LOWPASS_5_HZ;
		else if (bandwidth <= 10)
			filter = PIOS_MPU6500_GYRO_LOWPASS_10_HZ;
		else if (bandwidth <= 20)
			filter = PIOS_MPU6500_GYRO_LOWPASS_20_HZ;
		else if (bandwidth <= 41)
			filter = PIOS_MPU6500_GYRO_LOWPASS_41_HZ;
		else if (bandwidth <= 92)
			filter = PIOS_MPU6500_GYRO_LOWPASS_92_HZ;
		else
			filter = PIOS_MPU6500_GYRO_LOWPASS_184_HZ;
	} else if (mpu_dev->mpu_type == PIOS_MPU60X0) {
		if (bandwidth <= 5)
			filter = PIOS_MPU60X0_GYRO_LOWPASS_5_HZ;
		else if (bandwidth <= 10)
			filter = PIOS_MPU60X0_GYRO_LOWPASS_10_HZ;
		else if (bandwidth <= 20)
			filter = PIOS_MPU60X0_GYRO_LOWPASS_20_HZ;
		else if (bandwidth <= 42)
			filter = PIOS_MPU60X0_GYRO_LOWPASS_42_HZ;
		else if (bandwidth <= 98)
			filter = PIOS_MPU60X0_GYRO_LOWPASS_98_HZ;
		else
			filter = PIOS_MPU60X0_GYRO_LOWPASS_188_HZ;
	} else {
		if (bandwidth <= 5)
			filter = PIOS_ICM20608G_GYRO_LOWPASS_5_HZ;
		else if (bandwidth <= 10)
			filter = PIOS_ICM20608G_GYRO_LOWPASS_10_HZ;
		else if (bandwidth <= 20)
			filter = PIOS_ICM20608G_GYRO_LOWPASS_20_HZ;
		else if (bandwidth <= 92)
			filter = PIOS_ICM20608G_GYRO_LOWPASS_92_HZ;
		else if (bandwidth <= 176)
			filter = PIOS_ICM20608G_GYRO_LOWPASS_176_HZ;
		else if (bandwidth <= 250)
			filter = PIOS_ICM20608G_GYRO_LOWPASS_250_HZ;
		else
			filter = PIOS_ICM20608G_GYRO_LOWPASS_3281_HZ;
	}

	PIOS_MPU_WriteReg(PIOS_MPU_DLPF_CFG_REG, filter);
}

void PIOS_MPU_SetAccelBandwidth(uint16_t bandwidth)
{
	if (mpu_dev->mpu_type != PIOS_MPU6500 && mpu_dev->mpu_type != PIOS_MPU9250 && mpu_dev->mpu_type != PIOS_ICM20608G)
		return;

	enum pios_mpu6500_accel_filter filter;
	if (mpu_dev->mpu_type == PIOS_MPU6500 || mpu_dev->mpu_type == PIOS_MPU9250) {
		if (bandwidth <= 5)
			filter = PIOS_MPU6500_ACCEL_LOWPASS_5_HZ;
		else if (bandwidth <= 10)
			filter = PIOS_MPU6500_ACCEL_LOWPASS_10_HZ;
		else if (bandwidth <= 20)
			filter = PIOS_MPU6500_ACCEL_LOWPASS_20_HZ;
		else if (bandwidth <= 41)
			filter = PIOS_MPU6500_ACCEL_LOWPASS_41_HZ;
		else if (bandwidth <= 92)
			filter = PIOS_MPU6500_ACCEL_LOWPASS_92_HZ;
		else if (bandwidth <= 184)
			filter = PIOS_MPU6500_ACCEL_LOWPASS_184_HZ;
		else
			filter = PIOS_MPU6500_ACCEL_LOWPASS_460_HZ;
	} else {
		if (bandwidth <= 5)
			filter = PIOS_ICM20608G_ACCEL_LOWPASS_5_HZ;
		else if (bandwidth <= 10)
			filter = PIOS_ICM20608G_ACCEL_LOWPASS_10_HZ;
		else if (bandwidth <= 21)
			filter = PIOS_ICM20608G_ACCEL_LOWPASS_21_HZ;
		else if (bandwidth <= 45)
			filter = PIOS_ICM20608G_ACCEL_LOWPASS_45_HZ;
		else if (bandwidth <= 99)
			filter = PIOS_ICM20608G_ACCEL_LOWPASS_99_HZ;
		else if (bandwidth <= 218)
			filter = PIOS_ICM20608G_ACCEL_LOWPASS_218_HZ;
		else
			filter = PIOS_ICM20608G_ACCEL_LOWPASS_420_HZ;
	}

	PIOS_MPU_WriteReg(PIOS_MPU_ACCEL_CFG2_REG, filter);
}

int32_t PIOS_MPU_SetSampleRate(uint16_t samplerate_hz)
{
	// TODO: think about supporting >1 khz, aliasing/noise issues though..
	uint16_t internal_rate = 1000;

	// limit samplerate to filter frequency
	if (samplerate_hz > internal_rate)
		samplerate_hz = internal_rate;

	// calculate divisor, round to nearest integer
	int32_t divisor = (int32_t)(((float)internal_rate / samplerate_hz) + 0.5f) - 1;

	// limit resulting divisor to register value range
	if (divisor < 0)
		divisor = 0;

	if (divisor > 0xff)
		divisor = 0xff;

	// calculate true sample rate
	samplerate_hz = internal_rate / (1 + divisor);

	int32_t retval = PIOS_MPU_WriteReg(PIOS_MPU_SMPLRT_DIV_REG, (uint8_t)divisor);

	if (retval == 0) {
		PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_ACCEL, samplerate_hz);
		PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_GYRO, samplerate_hz);
#ifdef PIOS_INCLUDE_MPU_MAG
		if (mpu_dev->use_mag) {
			uint16_t mag_rate = (mpu_dev->mpu_type == PIOS_MPU9250) ? 100 : samplerate_hz;
			PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_MAG, mag_rate);
		}
#endif // PIOS_INCLUDE_MPU_MAG
	}

	return retval;
}

enum pios_mpu_type PIOS_MPU_GetType(void)
{
	return mpu_dev->mpu_type;
}

#if defined(PIOS_INCLUDE_I2C)
static int32_t PIOS_MPU_I2C_Probe(enum pios_mpu_type *detected_device)
{
	int32_t retval = -1;
	for (int i = 0; i < NELEMENTS(pios_mpu_i2c_addr); i++) {
		mpu_dev->com_slave_addr = pios_mpu_i2c_addr[i];
		retval = PIOS_MPU_CheckWhoAmI(detected_device);
		if (retval == 0)
			break;
	}
	return retval;
}

int32_t PIOS_MPU_I2C_Init(pios_mpu_dev_t *dev, uint32_t i2c_id, const struct pios_mpu_cfg *cfg)
{
	if (!*dev) {
		mpu_dev = PIOS_MPU_Alloc(cfg);
		if (mpu_dev == NULL)
			return -1;
		*dev = mpu_dev;
	} else {
		mpu_dev = *dev;
	}

	mpu_dev->com_driver_type = PIOS_MPU_COM_I2C;
	mpu_dev->com_driver_id = i2c_id;
	mpu_dev->cfg = cfg;

	if (PIOS_MPU_I2C_Probe(&mpu_dev->mpu_type) != 0)
		return -PIOS_MPU_ERROR_PROBEFAILED;

	return PIOS_MPU_Common_Init();
}

static int32_t PIOS_MPU_I2C_Read(uint8_t address, uint8_t *buffer, uint8_t len)
{
	uint8_t addr_buffer[] = {
		address,
	};

	const struct pios_i2c_txn txn_list[] = {
		{
			.info = __func__,
			.addr = mpu_dev->com_slave_addr,
			.rw = PIOS_I2C_TXN_WRITE,
			.len = sizeof(addr_buffer),
			.buf = addr_buffer,
		},
		{
			.info = __func__,
			.addr = mpu_dev->com_slave_addr,
			.rw = PIOS_I2C_TXN_READ,
			.len = len,
			.buf = buffer,
		}
	};

	return PIOS_I2C_Transfer(mpu_dev->com_driver_id, txn_list, NELEMENTS(txn_list));
}

static int32_t PIOS_MPU_I2C_Write(uint8_t address, uint8_t buffer)
{
	uint8_t data[] = {
		address,
		buffer,
	};

	const struct pios_i2c_txn txn_list[] = {
		{
			.info = __func__,
			.addr = mpu_dev->com_slave_addr,
			.rw = PIOS_I2C_TXN_WRITE,
			.len = sizeof(data),
			.buf = data,
		},
	};

	return PIOS_I2C_Transfer(mpu_dev->com_driver_id, txn_list, NELEMENTS(txn_list));
}
#endif // defined(PIOS_INCLUDE_I2C)

#if defined(PIOS_INCLUDE_SPI)
int32_t PIOS_MPU_SPI_Init(pios_mpu_dev_t *dev, uint32_t spi_id, uint32_t slave_num, const struct pios_mpu_cfg *cfg)
{
	if (!*dev) {
		mpu_dev = PIOS_MPU_Alloc(cfg);
		if (mpu_dev == NULL)
			return -1;
		*dev = mpu_dev;
	} else {
		mpu_dev = *dev;
	}

	mpu_dev->com_driver_type = PIOS_MPU_COM_SPI;
	mpu_dev->com_driver_id = spi_id;
	mpu_dev->com_slave_addr = slave_num;
	mpu_dev->cfg = cfg;

	// TODO: probe MPU type
	enum pios_mpu_type mpu_type;
	PIOS_MPU_SPI_Probe(&mpu_type);
	// TODO: detect mag?

	return PIOS_MPU_Common_Init();
}

static int32_t PIOS_MPU_SPI_Probe(enum pios_mpu_type *detected_device)
{
	int32_t retval = PIOS_MPU_CheckWhoAmI(detected_device);
	return retval;
}

static int32_t PIOS_MPU_ClaimBus(bool lowspeed)
{
	if (PIOS_MPU_Validate(mpu_dev) != 0)
		return -1;

	if (PIOS_SPI_ClaimBus(mpu_dev->com_driver_id) != 0)
		return -2;

	if (lowspeed)
		PIOS_SPI_SetClockSpeed(mpu_dev->com_driver_id, PIOS_MPU_SPI_LOW_SPEED);

	PIOS_SPI_RC_PinSet(mpu_dev->com_driver_id, mpu_dev->com_slave_addr, 0);

	return 0;
}

static int32_t PIOS_MPU_ReleaseBus(bool lowspeed)
{
	if (PIOS_MPU_Validate(mpu_dev) != 0)
		return -1;

	PIOS_SPI_RC_PinSet(mpu_dev->com_driver_id, mpu_dev->com_slave_addr, 1);

	if (lowspeed)
		PIOS_SPI_SetClockSpeed(mpu_dev->com_driver_id, PIOS_MPU_SPI_HIGH_SPEED);

	PIOS_SPI_ReleaseBus(mpu_dev->com_driver_id);

	return 0;
}

static int32_t PIOS_MPU_SPI_Read(uint8_t address, uint8_t *buffer)
{
	if (PIOS_MPU_ClaimBus(true) != 0)
		return -1;

	PIOS_SPI_TransferByte(mpu_dev->com_driver_id, 0x80 | address); // request byte
	*buffer = PIOS_SPI_TransferByte(mpu_dev->com_driver_id, 0);   // receive response

	PIOS_MPU_ReleaseBus(true);

	return 0;
}

static int32_t PIOS_MPU_SPI_Write(uint8_t address, uint8_t buffer)
{
	if (PIOS_MPU_ClaimBus(true) != 0)
		return -1;

	PIOS_SPI_TransferByte(mpu_dev->com_driver_id, 0x7f & address);
	PIOS_SPI_TransferByte(mpu_dev->com_driver_id, buffer);

	PIOS_MPU_ReleaseBus(true);

	return 0;
}
#endif // defined(PIOS_INCLUDE_SPI)

static int32_t PIOS_MPU_WriteReg(uint8_t reg, uint8_t data)
{
#if defined(PIOS_INCLUDE_I2C)
	if (mpu_dev->com_driver_type == PIOS_MPU_COM_I2C)
		return PIOS_MPU_I2C_Write(reg, data);
#endif // defined(PIOS_INCLUDE_I2C)
#if defined(PIOS_INCLUDE_SPI)
	if (mpu_dev->com_driver_type == PIOS_MPU_COM_SPI)
		return PIOS_MPU_SPI_Write(reg, data);
#endif // defined(PIOS_INCLUDE_SPI)
	
	return -1;
}

static int32_t PIOS_MPU_ReadReg(uint8_t reg)
{
	uint8_t data;
	int32_t retval = -1;

#if defined(PIOS_INCLUDE_I2C)
	if (mpu_dev->com_driver_type == PIOS_MPU_COM_I2C)
		retval = PIOS_MPU_I2C_Read(reg, &data, sizeof(data));
#endif // defined(PIOS_INCLUDE_I2C)
#if defined(PIOS_INCLUDE_SPI)
	if (mpu_dev->com_driver_type == PIOS_MPU_COM_SPI)
		retval = PIOS_MPU_SPI_Read(reg, &data);
#endif // defined(PIOS_INCLUDE_SPI)

	if (retval != 0)
		return retval;
	else
		return data;
}

bool PIOS_MPU_IRQHandler(void)
{
	if (PIOS_MPU_Validate(mpu_dev) != 0)
		return false;

	bool woken = false;

	PIOS_Semaphore_Give_FromISR(mpu_dev->data_ready_sema, &woken);

	return woken;
}

static void PIOS_MPU_Task(void *parameters)
{
	(void)parameters;

	enum {
		IDX_SPI_DUMMY_BYTE = 0,
		IDX_ACCEL_XOUT_H,
		IDX_ACCEL_XOUT_L,
		IDX_ACCEL_YOUT_H,
		IDX_ACCEL_YOUT_L,
		IDX_ACCEL_ZOUT_H,
		IDX_ACCEL_ZOUT_L,
		IDX_TEMP_OUT_H,
		IDX_TEMP_OUT_L,
		IDX_GYRO_XOUT_H,
		IDX_GYRO_XOUT_L,
		IDX_GYRO_YOUT_H,
		IDX_GYRO_YOUT_L,
		IDX_GYRO_ZOUT_H,
		IDX_GYRO_ZOUT_L,
#ifdef PIOS_INCLUDE_MPU_MAG
		IDX_MAG_ST1,
		IDX_MAG_XOUT_L,
		IDX_MAG_XOUT_H,
		IDX_MAG_YOUT_L,
		IDX_MAG_YOUT_H,
		IDX_MAG_ZOUT_L,
		IDX_MAG_ZOUT_H,
		IDX_MAG_ST2,
#endif // PIOS_INCLUDE_MPU_MAG
		BUFFER_SIZE
	};

	uint8_t mpu_rec_buf[BUFFER_SIZE];

#ifdef PIOS_INCLUDE_MPU_MAG
	uint8_t transfer_size = (mpu_dev->use_mag) ? BUFFER_SIZE : BUFFER_SIZE - 8;
#ifdef PIOS_INCLUDE_SPI
	uint8_t mpu_tx_buf[BUFFER_SIZE] = {PIOS_MPU_ACCEL_X_OUT_MSB | 0x80, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif // PIOS_INCLUDE_SPI
#else
	uint8_t transfer_size = BUFFER_SIZE;
#ifdef PIOS_INCLUDE_SPI
	uint8_t mpu_tx_buf[BUFFER_SIZE] = {PIOS_MPU_ACCEL_X_OUT_MSB | 0x80, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif // PIOS_INCLUDE_SPI
#endif // PIOS_INCLUDE_MPU_MAG

	if (mpu_dev->com_driver_type == PIOS_MPU_COM_I2C)
		transfer_size -= 1;

	while (true) {
		//Wait for data ready interrupt
		if (PIOS_Semaphore_Take(mpu_dev->data_ready_sema, PIOS_SEMAPHORE_TIMEOUT_MAX) != true)
			continue;
		
#if defined(PIOS_INCLUDE_SPI)
		if (mpu_dev->com_driver_type == PIOS_MPU_COM_SPI) {
			// claim bus in high speed mode
			if (PIOS_MPU_ClaimBus(false) != 0)
				continue;

			if (PIOS_SPI_TransferBlock(mpu_dev->com_driver_id, mpu_tx_buf, mpu_rec_buf, transfer_size, NULL) < 0) {
				PIOS_MPU_ReleaseBus(false);
				continue;
			}

			PIOS_MPU_ReleaseBus(false);
		}
#endif // defined(PIOS_INCLUDE_SPI)

#if defined(PIOS_INCLUDE_I2C)
		if (mpu_dev->com_driver_type == PIOS_MPU_COM_I2C) {
			// we skip the SPI dummy byte at the beginning of the buffer here
			if (PIOS_MPU_I2C_Read(PIOS_MPU_ACCEL_X_OUT_MSB, &mpu_rec_buf[IDX_ACCEL_XOUT_H], transfer_size) < 0)
				continue;
		}
#endif // defined(PIOS_INCLUDE_I2C)

		struct pios_sensor_accel_data accel_data;
		struct pios_sensor_gyro_data gyro_data;

		float accel_x = (int16_t)(mpu_rec_buf[IDX_ACCEL_XOUT_H] << 8 | mpu_rec_buf[IDX_ACCEL_XOUT_L]);
		float accel_y = (int16_t)(mpu_rec_buf[IDX_ACCEL_YOUT_H] << 8 | mpu_rec_buf[IDX_ACCEL_YOUT_L]);
		float accel_z = (int16_t)(mpu_rec_buf[IDX_ACCEL_ZOUT_H] << 8 | mpu_rec_buf[IDX_ACCEL_ZOUT_L]);
		float gyro_x = (int16_t)(mpu_rec_buf[IDX_GYRO_XOUT_H] << 8 | mpu_rec_buf[IDX_GYRO_XOUT_L]);
		float gyro_y = (int16_t)(mpu_rec_buf[IDX_GYRO_YOUT_H] << 8 | mpu_rec_buf[IDX_GYRO_YOUT_L]);
		float gyro_z = (int16_t)(mpu_rec_buf[IDX_GYRO_ZOUT_H] << 8 | mpu_rec_buf[IDX_GYRO_ZOUT_L]);

#ifdef PIOS_INCLUDE_MPU_MAG
		struct pios_sensor_mag_data mag_data;

		float mag_x = (int16_t)(mpu_rec_buf[IDX_MAG_XOUT_H] << 8 | mpu_rec_buf[IDX_MAG_XOUT_L]);
		float mag_y = (int16_t)(mpu_rec_buf[IDX_MAG_YOUT_H] << 8 | mpu_rec_buf[IDX_MAG_YOUT_L]);
		float mag_z = (int16_t)(mpu_rec_buf[IDX_MAG_ZOUT_H] << 8 | mpu_rec_buf[IDX_MAG_ZOUT_L]);
#endif // PIOS_INCLUDE_MPU_MAG

		// Rotate the sensor to our convention.  The datasheet defines X as towards the right
		// and Y as forward. Our convention transposes this.  Also the Z is defined negatively
		// to our convention. This is true for accels and gyros. Magnetometer corresponds our convention.
		switch (mpu_dev->cfg->orientation) {
		case PIOS_MPU_TOP_0DEG:
			accel_data.y = accel_x;
			accel_data.x = accel_y;
			accel_data.z = -accel_z;
			gyro_data.y  = gyro_x;
			gyro_data.x  = gyro_y;
			gyro_data.z  = -gyro_z;
#ifdef PIOS_INCLUDE_MPU_MAG
			mag_data.x   = mag_x;
			mag_data.y   = mag_y;
			mag_data.z   = mag_z;
#endif // PIOS_INCLUDE_MPU_MAG
			break;
		case PIOS_MPU_TOP_90DEG:
			accel_data.y = -accel_y;
			accel_data.x = accel_x;
			accel_data.z = -accel_z;
			gyro_data.y  = -gyro_y;
			gyro_data.x  = gyro_x;
			gyro_data.z  = -gyro_z;
#ifdef PIOS_INCLUDE_MPU_MAG
			mag_data.x   = -mag_y;
			mag_data.y   = mag_x;
			mag_data.z   = mag_z;
#endif // PIOS_INCLUDE_MPU_MAG
			break;
		case PIOS_MPU_TOP_180DEG:
			accel_data.y = -accel_x;
			accel_data.x = -accel_y;
			accel_data.z = -accel_z;
			gyro_data.y  = -gyro_x;
			gyro_data.x  = -gyro_y;
			gyro_data.z  = -gyro_z;
#ifdef PIOS_INCLUDE_MPU_MAG
			mag_data.x   = -mag_x;
			mag_data.y   = -mag_y;
			mag_data.z   = mag_z;
#endif // PIOS_INCLUDE_MPU_MAG
			break;
		case PIOS_MPU_TOP_270DEG:
			accel_data.y = accel_y;
			accel_data.x = -accel_x;
			accel_data.z = -accel_z;
			gyro_data.y  = gyro_y;
			gyro_data.x  = -gyro_x;
			gyro_data.z  = -gyro_z;
#ifdef PIOS_INCLUDE_MPU_MAG
			mag_data.x   = mag_y;
			mag_data.y   = -mag_x;
			mag_data.z   = mag_z;
#endif // PIOS_INCLUDE_MPU_MAG
			break;
		case PIOS_MPU_BOTTOM_0DEG:
			accel_data.y = -accel_x;
			accel_data.x = accel_y;
			accel_data.z = accel_z;
			gyro_data.y  = -gyro_x;
			gyro_data.x  = gyro_y;
			gyro_data.z  = gyro_z;
#ifdef PIOS_INCLUDE_MPU_MAG
			mag_data.x   = mag_x;
			mag_data.y   = -mag_y;
			mag_data.z   = -mag_z;
#endif // PIOS_INCLUDE_MPU_MAG
			break;

		case PIOS_MPU_BOTTOM_90DEG:
			accel_data.y = -accel_y;
			accel_data.x = -accel_x;
			accel_data.z = accel_z;
			gyro_data.y  = -gyro_y;
			gyro_data.x  = -gyro_x;
			gyro_data.z  = gyro_z;
#ifdef PIOS_INCLUDE_MPU_MAG
			mag_data.x   = -mag_y;
			mag_data.y   = -mag_x;
			mag_data.z   = -mag_z;
#endif // PIOS_INCLUDE_MPU_MAG
			break;

		case PIOS_MPU_BOTTOM_180DEG:
			accel_data.y = accel_x;
			accel_data.x = -accel_y;
			accel_data.z = accel_z;
			gyro_data.y  = gyro_x;
			gyro_data.x  = -gyro_y;
			gyro_data.z  = gyro_z;
#ifdef PIOS_INCLUDE_MPU_MAG
			mag_data.x   = -mag_x;
			mag_data.y   = mag_y;
			mag_data.z   = -mag_z;
#endif // PIOS_INCLUDE_MPU_MAG
			break;

		case PIOS_MPU_BOTTOM_270DEG:
			accel_data.y = accel_y;
			accel_data.x = accel_x;
			gyro_data.y  = gyro_y;
			gyro_data.x  = gyro_x;
			gyro_data.z  = gyro_z;
			accel_data.z = accel_z;
#ifdef PIOS_INCLUDE_MPU_MAG
			mag_data.x   = mag_y;
			mag_data.y   = mag_x;
			mag_data.z   = -mag_z;
#endif // PIOS_INCLUDE_MPU_MAG
			break;
		}

		int16_t raw_temp = (int16_t)(mpu_rec_buf[IDX_TEMP_OUT_H] << 8 | mpu_rec_buf[IDX_TEMP_OUT_L]);
		float temperature;
		if (mpu_dev->mpu_type == PIOS_MPU6500 || mpu_dev->mpu_type == PIOS_MPU9250)
			temperature = 21.0f + ((float)raw_temp) / 333.87f;
		else
			temperature = 35.0f + ((float)raw_temp + 512.0f) / 340.0f;

		// Apply sensor scaling
		float accel_scale = PIOS_MPU_GetAccelScale();
		accel_data.x *= accel_scale;
		accel_data.y *= accel_scale;
		accel_data.z *= accel_scale;
		accel_data.temperature = temperature;

		float gyro_scale = PIOS_MPU_GetGyroScale();
		gyro_data.x *= gyro_scale;
		gyro_data.y *= gyro_scale;
		gyro_data.z *= gyro_scale;
		gyro_data.temperature = temperature;

		PIOS_Queue_Send(mpu_dev->accel_queue, &accel_data, 0);
		PIOS_Queue_Send(mpu_dev->gyro_queue, &gyro_data, 0);

#ifdef PIOS_INCLUDE_MPU_MAG
		if (mpu_dev->use_mag) {
			// check data ready
			bool mag_ok = mpu_rec_buf[IDX_MAG_ST1] & PIOS_MPU_AK89XX_ST1_DRDY;
			// check for overflow
			mag_ok &= !(mpu_rec_buf[IDX_MAG_ST2] & PIOS_MPU_AK89XX_ST2_HOFL);
			// check for data error on mpu-9150
			mag_ok &= (mpu_dev->mpu_type != PIOS_MPU9150 || !(mpu_rec_buf[IDX_MAG_ST2] & PIOS_MPU_AK8975_ST2_DERR));
			if (mag_ok) {
				float mag_scale;
				if (mpu_dev->mpu_type == PIOS_MPU9150)
					mag_scale = 3.0f; // 12-bit sampling
				else if (mpu_rec_buf[IDX_MAG_ST2] & PIOS_MPU_AK8963_ST2_BITM)
					mag_scale = 1.5f; // 16-bit sampling
				else
					mag_scale = 6.0f; // 14-bit sampling
				mag_data.x *= mag_scale;
				mag_data.y *= mag_scale;
				mag_data.z *= mag_scale;
				PIOS_Queue_Send(mpu_dev->mag_queue, &mag_data, 0);

				// trigger another sample
				if (mpu_dev->mpu_type == PIOS_MPU9150)
					PIOS_MPU_Mag_WriteReg(PIOS_MPU_AK89XX_CNTL1_REG, PIOS_MPU_AK8975_MODE_SINGLE_12B);
			}
		}
#endif // PIOS_INCLUDE_MPU_MAG
	}
}

#endif // PIOS_INCLUDE_MPU

/**
 * @}
 * @}
 */
