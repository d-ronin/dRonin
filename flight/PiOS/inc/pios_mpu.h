/**
 ******************************************************************************
 * @file       pios_mpu_priv.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MPU Invensense MPU Functions
 * @{
 * @brief Common driver for Invensense MPU-xxx0 6/9-axis IMUs.
 * Public Interface
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
 */

#ifndef PIOS_MPU_H
#define PIOS_MPU_H

#include "pios.h"

/**
 * MPU driver error return types
 */
enum pios_mpu_error {
	PIOS_MPU_ERROR_NONE     = 0,
	PIOS_MPU_ERROR_NOCONFIG = 1,
	PIOS_MPU_ERROR_PROBEFAILED,
	PIOS_MPU_ERROR_WRITEFAILED,
	PIOS_MPU_ERROR_READFAILED,
	PIOS_MPU_ERROR_NOIRQ,
	PIOS_MPU_ERROR_SAMPLERATE,
	PIOS_MPU_ERROR_WHOAMI,
	PIOS_MPU_ERROR_MAGFAILED,
};

/**
 * Supported IMU devices
 */
enum pios_mpu_type {
	PIOS_MPU60X0, /**< MPU-6000, SPI/I2C or MPU-6050, I2C */
	PIOS_MPU6500, /**< MPU-6500, SPI/I2C */
	PIOS_MPU9150, /**< MPU-9150, contains MPU-6050+AK8975C */
	PIOS_MPU9250, /**< MPU-9250, contains MPU-6500+AK8963 */
	PIOS_ICM20608G, /**<ICM-20608-G, SPI/I2C */
	PIOS_MPU_NUM  /**< Number of elements */
};

enum pios_mpu60x0_gyro_filter {
	PIOS_MPU60X0_GYRO_LOWPASS_256_HZ = 0x00, // 8KHz sample mode
	PIOS_MPU60X0_GYRO_LOWPASS_188_HZ = 0x01,
	PIOS_MPU60X0_GYRO_LOWPASS_98_HZ  = 0x02,
	PIOS_MPU60X0_GYRO_LOWPASS_42_HZ  = 0x03,
	PIOS_MPU60X0_GYRO_LOWPASS_20_HZ  = 0x04,
	PIOS_MPU60X0_GYRO_LOWPASS_10_HZ  = 0x05,
	PIOS_MPU60X0_GYRO_LOWPASS_5_HZ   = 0x06
};

enum pios_mpu6500_gyro_filter {
	PIOS_MPU6500_GYRO_LOWPASS_250_HZ = 0x00, // 8KHz sample mode
	PIOS_MPU6500_GYRO_LOWPASS_184_HZ = 0x01,
	PIOS_MPU6500_GYRO_LOWPASS_92_HZ  = 0x02,
	PIOS_MPU6500_GYRO_LOWPASS_41_HZ  = 0x03,
	PIOS_MPU6500_GYRO_LOWPASS_20_HZ  = 0x04,
	PIOS_MPU6500_GYRO_LOWPASS_10_HZ  = 0x05,
	PIOS_MPU6500_GYRO_LOWPASS_5_HZ   = 0x06
};

enum pios_icm20608g_gyro_filter {
	PIOS_ICM20608G_GYRO_LOWPASS_250_HZ  = 0x00, // 8KHz sample mode
	PIOS_ICM20608G_GYRO_LOWPASS_176_HZ  = 0x01,
	PIOS_ICM20608G_GYRO_LOWPASS_92_HZ   = 0x02,
	PIOS_ICM20608G_GYRO_LOWPASS_41_HZ   = 0x03,
	PIOS_ICM20608G_GYRO_LOWPASS_20_HZ   = 0x04,
	PIOS_ICM20608G_GYRO_LOWPASS_10_HZ   = 0x05,
	PIOS_ICM20608G_GYRO_LOWPASS_5_HZ    = 0x06,
	PIOS_ICM20608G_GYRO_LOWPASS_3281_HZ = 0x07 // 8KHz sample mode
};

enum pios_mpu6500_accel_filter {
	PIOS_MPU6500_ACCEL_LOWPASS_460_HZ = 0x00,
	PIOS_MPU6500_ACCEL_LOWPASS_184_HZ = 0x01,
	PIOS_MPU6500_ACCEL_LOWPASS_92_HZ  = 0x02,
	PIOS_MPU6500_ACCEL_LOWPASS_41_HZ  = 0x03,
	PIOS_MPU6500_ACCEL_LOWPASS_20_HZ  = 0x04,
	PIOS_MPU6500_ACCEL_LOWPASS_10_HZ  = 0x05,
	PIOS_MPU6500_ACCEL_LOWPASS_5_HZ   = 0x06
};

enum pios_icm20608g_accel_filter {
	PIOS_ICM20608G_ACCEL_LOWPASS_218_HZ = 0x00, // 0x00 and 0x01 are identical.
	PIOS_ICM20608G_ACCEL_LOWPASS_99_HZ  = 0x02,
	PIOS_ICM20608G_ACCEL_LOWPASS_45_HZ  = 0x03,
	PIOS_ICM20608G_ACCEL_LOWPASS_21_HZ  = 0x04,
	PIOS_ICM20608G_ACCEL_LOWPASS_10_HZ  = 0x05,
	PIOS_ICM20608G_ACCEL_LOWPASS_5_HZ   = 0x06,
	PIOS_ICM20608G_ACCEL_LOWPASS_420_HZ = 0x07
};

enum pios_mpu_gyro_range {
	PIOS_MPU_SCALE_250_DEG  = 0x00,
	PIOS_MPU_SCALE_500_DEG  = 0x08,
	PIOS_MPU_SCALE_1000_DEG = 0x10,
	PIOS_MPU_SCALE_2000_DEG = 0x18
};

enum pios_mpu_accel_range {
	PIOS_MPU_SCALE_2G = 0x00,
	PIOS_MPU_SCALE_4G = 0x08,
	PIOS_MPU_SCALE_8G = 0x10,
	PIOS_MPU_SCALE_16G = 0x18
};

enum pios_mpu_orientation { // clockwise rotation from board forward
	PIOS_MPU_TOP_0DEG    = 0x00,
	PIOS_MPU_TOP_90DEG   = 0x01,
	PIOS_MPU_TOP_180DEG  = 0x02,
	PIOS_MPU_TOP_270DEG  = 0x03,
	PIOS_MPU_BOTTOM_0DEG  = 0x04,
	PIOS_MPU_BOTTOM_90DEG  = 0x05,
	PIOS_MPU_BOTTOM_180DEG  = 0x06,
	PIOS_MPU_BOTTOM_270DEG  = 0x07,
};

struct pios_mpu_cfg {
	const struct pios_exti_cfg *exti_cfg; /* Pointer to the EXTI configuration */

	uint16_t default_samplerate;
	enum pios_mpu_orientation orientation;
#ifdef PIOS_INCLUDE_MPU_MAG
	bool use_internal_mag;		/* Flag to indicate whether or not to use the internal mag on MPU9x50 devices */
#endif // PIOS_INCLUDE_MPU_MAG
};

typedef struct pios_mpu_dev * pios_mpu_dev_t;

/**
 * @brief Initialize the MPU-xxxx 6/9-axis sensor on I2C
 * @param[in] i2c_id PiOS I2C driver instance ID/pointer
 * @param[in] cfg MPU config
 * @return 0 for success, -1 for failure to allocate, -10 for failure to get irq
 */
int32_t PIOS_MPU_I2C_Init(pios_mpu_dev_t *dev, uint32_t i2c_id, const struct pios_mpu_cfg *cfg);
/**
 * @brief Initialize the MPU-xxxx 6/9-axis sensor on SPI
 * @return 0 for success, -1 for failure to allocate, -10 for failure to get irq
 */
int32_t PIOS_MPU_SPI_Init(pios_mpu_dev_t *dev, uint32_t spi_id, uint32_t slave_num, const struct pios_mpu_cfg *cfg);
int32_t PIOS_MPU_Test();
int32_t PIOS_MPU_SetGyroRange(enum pios_mpu_gyro_range range);
int32_t PIOS_MPU_SetAccelRange(enum pios_mpu_accel_range range);

/**
 * Set the sample rate in Hz by determining the nearest divisor
 * @param[in] sample rate in Hz
 */
int32_t PIOS_MPU_SetSampleRate(uint16_t samplerate_hz);

/**
 * @brief Sets the bandwidth desired from the gyro.
 * The driver will automatically select the lowest bandwidth
 * low-pass filter capable of providing the desired bandwidth.
 * @param[in] bandwidth The desired bandwidth [Hz]
 */
void PIOS_MPU_SetGyroBandwidth(uint16_t bandwidth);

/**
 * @brief Sets the bandwidth desired from the accelerometer.
 * The driver will automatically select the lowest bandwidth
 * low-pass filter capable of providing the desired bandwidth.
 * @param[in] bandwidth The desired bandwidth [Hz]
 */
void PIOS_MPU_SetAccelBandwidth(uint16_t bandwidth);

/**
 * @brief The IMU interrupt handler.
 * Fetches new data from the IMU.
 */
bool PIOS_MPU_IRQHandler(void);

/**
 * @brief Which type of MPU was detected?
 */
enum pios_mpu_type PIOS_MPU_GetType(void);

#endif /* PIOS_MPU_H */

/** 
  * @}
  * @}
  */