/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Sparky Tau Labs Sparky support files
 * @{
 *
 * @file       pios_board.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      The board specific initialization routines
 * @see        The GNU Public License (GPL) Version 3
 *
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

/* Pull in the board-specific static HW definitions.
 * Including .c files is a bit ugly but this allows all of
 * the HW definitions to be const and static to limit their
 * scope.
 *
 * NOTE: THIS IS THE ONLY PLACE THAT SHOULD EVER INCLUDE THIS FILE
 */

#include "board_hw_defs.c"

#include <pios.h>
//#include <pios_hal.h>
#include <openpilot.h>
#include <uavobjectsinit.h>
#include "hwsparky.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"

/**
 * Configuration for the MS5611 chip
 */
#if defined(PIOS_INCLUDE_MS5611)
#include "pios_ms5611_priv.h"
static const struct pios_ms5611_cfg pios_ms5611_cfg = {
	.oversampling = MS5611_OSR_1024,
	.temperature_interleaving = 1,
};
#endif /* PIOS_INCLUDE_MS5611 */

/**
 * Configuration for the external HMC5883 chip
 */
#if defined(PIOS_INCLUDE_HMC5883)
#include "pios_hmc5883_priv.h"
static const struct pios_hmc5883_cfg pios_hmc5883_external_cfg = {
	.exti_cfg            = NULL,
	.M_ODR               = PIOS_HMC5883_ODR_75,
	.Meas_Conf           = PIOS_HMC5883_MEASCONF_NORMAL,
	.Gain                = PIOS_HMC5883_GAIN_1_9,
	.Mode                = PIOS_HMC5883_MODE_SINGLE,
	.Default_Orientation = PIOS_HMC5883_TOP_0DEG,
};
#endif /* PIOS_INCLUDE_HMC5883 */

bool external_mag_fail;

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void)
{
	/* Delay system */
	//PIOS_DELAY_Init();

	const struct pios_board_info *bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_LED)
	const struct pios_led_cfg *led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
	PIOS_Assert(led_cfg);
	PIOS_LED_Init(led_cfg);
#endif	/* PIOS_INCLUDE_LED */

#if defined(PIOS_INCLUDE_RTC)
	/* Initialize the real-time clock and its associated tick */
	PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif

	/* Set up pulse timers */
	//inputs

	//outputs
#if 0
	PIOS_TIM_InitClock(&tim_1_cfg);
	PIOS_TIM_InitClock(&tim_2_cfg);
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_15_cfg);
	PIOS_TIM_InitClock(&tim_16_cfg);
	PIOS_TIM_InitClock(&tim_17_cfg);
#endif

#if 0
	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwSparkySetDefaults(HwSparkyHandle(), 0);
		ModuleSettingsSetDefaults(ModuleSettingsHandle(), 0);
		AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
	}
#endif

	/* Configure the IO ports */
	
#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_internal_id, &pios_i2c_internal_cfg))
		PIOS_DEBUG_Assert(0);

	if (PIOS_I2C_CheckClear(pios_i2c_internal_id) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_I2C_INT);
	else
		if (AlarmsGet(SYSTEMALARMS_ALARM_I2C) == SYSTEMALARMS_ALARM_UNINITIALISED)
			AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_OK);
#endif  // PIOS_INCLUDE_I2C

#if 0
	pios_servo_cfg.num_channels = number_of_pwm_outputs;

	if (hw_rcvrport != HWSHARED_PORTTYPES_PPM) {
		PIOS_Servo_Init(&pios_servo_cfg);
	} else {
		PIOS_Servo_Init(&pios_servo_slow_cfg);
	}
#endif

#if defined(PIOS_INCLUDE_ADC)
	if (number_of_adc_ports > 0) {
		internal_adc_cfg.adc_pin_count = number_of_adc_ports;
		uint32_t internal_adc_id;
		if (PIOS_INTERNAL_ADC_Init(&internal_adc_id, &internal_adc_cfg) < 0)
			PIOS_Assert(0);
		PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id);
	}
#endif /* PIOS_INCLUDE_ADC */

#if defined(PIOS_INCLUDE_MPU9150)
#if defined(PIOS_INCLUDE_MPU6050)
	// Enable autoprobing when both 6050 and 9050 compiled in
	bool mpu9150_found = false;
	if (PIOS_MPU9150_Probe(pios_i2c_internal_id, PIOS_MPU9150_I2C_ADD_A0_LOW) == 0) {
		mpu9150_found = true;
#else
	{
#endif /* PIOS_INCLUDE_MPU6050 */

		uint8_t magnetometer;
		HwSparkyMagnetometerGet(&magnetometer);

		if (magnetometer == HWSPARKY_MAGNETOMETER_INTERNAL)
			use_mpu_mag = true;
		else
			use_mpu_mag = false;


		int retval;
		retval = PIOS_MPU9150_Init(pios_i2c_internal_id, PIOS_MPU9150_I2C_ADD_A0_LOW, &pios_mpu9150_cfg, use_mpu_mag);
		if (retval == -10)
			PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);
		if (retval != 0)
			PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

		// To be safe map from UAVO enum to driver enum
		uint8_t hw_gyro_range;
		HwSparkyGyroRangeGet(&hw_gyro_range);
		switch (hw_gyro_range) {
		case HWSPARKY_GYRORANGE_250:
			PIOS_MPU9150_SetGyroRange(PIOS_MPU60X0_SCALE_250_DEG);
			break;
		case HWSPARKY_GYRORANGE_500:
			PIOS_MPU9150_SetGyroRange(PIOS_MPU60X0_SCALE_500_DEG);
			break;
		case HWSPARKY_GYRORANGE_1000:
			PIOS_MPU9150_SetGyroRange(PIOS_MPU60X0_SCALE_1000_DEG);
			break;
		case HWSPARKY_GYRORANGE_2000:
			PIOS_MPU9150_SetGyroRange(PIOS_MPU60X0_SCALE_2000_DEG);
			break;
		}

		uint8_t hw_accel_range;
		HwSparkyAccelRangeGet(&hw_accel_range);
		switch (hw_accel_range) {
		case HWSPARKY_ACCELRANGE_2G:
			PIOS_MPU9150_SetAccelRange(PIOS_MPU60X0_ACCEL_2G);
			break;
		case HWSPARKY_ACCELRANGE_4G:
			PIOS_MPU9150_SetAccelRange(PIOS_MPU60X0_ACCEL_4G);
			break;
		case HWSPARKY_ACCELRANGE_8G:
			PIOS_MPU9150_SetAccelRange(PIOS_MPU60X0_ACCEL_8G);
			break;
		case HWSPARKY_ACCELRANGE_16G:
			PIOS_MPU9150_SetAccelRange(PIOS_MPU60X0_ACCEL_16G);
			break;
		}

		uint8_t hw_mpu9150_dlpf;
		HwSparkyMPU9150DLPFGet(&hw_mpu9150_dlpf);
		enum pios_mpu60x0_filter mpu9150_dlpf = \
		                                        (hw_mpu9150_dlpf == HWSPARKY_MPU9150DLPF_188) ? PIOS_MPU60X0_LOWPASS_188_HZ : \
		                                        (hw_mpu9150_dlpf == HWSPARKY_MPU9150DLPF_98) ? PIOS_MPU60X0_LOWPASS_98_HZ : \
		                                        (hw_mpu9150_dlpf == HWSPARKY_MPU9150DLPF_42) ? PIOS_MPU60X0_LOWPASS_42_HZ : \
		                                        (hw_mpu9150_dlpf == HWSPARKY_MPU9150DLPF_20) ? PIOS_MPU60X0_LOWPASS_20_HZ : \
		                                        (hw_mpu9150_dlpf == HWSPARKY_MPU9150DLPF_10) ? PIOS_MPU60X0_LOWPASS_10_HZ : \
		                                        (hw_mpu9150_dlpf == HWSPARKY_MPU9150DLPF_5) ? PIOS_MPU60X0_LOWPASS_5_HZ : \
		                                        pios_mpu9150_cfg.default_filter;
		PIOS_MPU9150_SetLPF(mpu9150_dlpf);

		uint8_t hw_mpu9150_samplerate;
		HwSparkyMPU9150RateGet(&hw_mpu9150_samplerate);
		uint16_t mpu9150_samplerate = \
		                              (hw_mpu9150_samplerate == HWSPARKY_MPU9150RATE_200) ? 200 : \
		                              (hw_mpu9150_samplerate == HWSPARKY_MPU9150RATE_333) ? 333 : \
		                              (hw_mpu9150_samplerate == HWSPARKY_MPU9150RATE_500) ? 500 : \
		                              (hw_mpu9150_samplerate == HWSPARKY_MPU9150RATE_1000) ? 1000 : \
		                              pios_mpu9150_cfg.default_samplerate;
		PIOS_MPU9150_SetSampleRate(mpu9150_samplerate);
	}

#endif /* PIOS_INCLUDE_MPU9150 */

#if defined(PIOS_INCLUDE_MPU6050)
#if defined(PIOS_INCLUDE_MPU9150)
	// MPU9150 looks like an MPU6050 _plus_ additional hardware.  So we cannot try and
	// probe if MPU9150 is found or we will find a duplicate
	if (mpu9150_found == false)
#endif /* PIOS_INCLUDE_MPU9150 */
	{
		if (PIOS_MPU6050_Init(pios_i2c_internal_id, PIOS_MPU6050_I2C_ADD_A0_LOW, &pios_mpu6050_cfg) != 0)
			PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);
		if (PIOS_MPU6050_Test() != 0)
			PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

		// To be safe map from UAVO enum to driver enum
		uint8_t hw_gyro_range;
		HwSparkyGyroRangeGet(&hw_gyro_range);
		switch (hw_gyro_range) {
		case HWSPARKY_GYRORANGE_250:
			PIOS_MPU6050_SetGyroRange(PIOS_MPU60X0_SCALE_250_DEG);
			break;
		case HWSPARKY_GYRORANGE_500:
			PIOS_MPU6050_SetGyroRange(PIOS_MPU60X0_SCALE_500_DEG);
			break;
		case HWSPARKY_GYRORANGE_1000:
			PIOS_MPU6050_SetGyroRange(PIOS_MPU60X0_SCALE_1000_DEG);
			break;
		case HWSPARKY_GYRORANGE_2000:
			PIOS_MPU6050_SetGyroRange(PIOS_MPU60X0_SCALE_2000_DEG);
			break;
		}

		uint8_t hw_accel_range;
		HwSparkyAccelRangeGet(&hw_accel_range);
		switch (hw_accel_range) {
		case HWSPARKY_ACCELRANGE_2G:
			PIOS_MPU6050_SetAccelRange(PIOS_MPU60X0_ACCEL_2G);
			break;
		case HWSPARKY_ACCELRANGE_4G:
			PIOS_MPU6050_SetAccelRange(PIOS_MPU60X0_ACCEL_4G);
			break;
		case HWSPARKY_ACCELRANGE_8G:
			PIOS_MPU6050_SetAccelRange(PIOS_MPU60X0_ACCEL_8G);
			break;
		case HWSPARKY_ACCELRANGE_16G:
			PIOS_MPU6050_SetAccelRange(PIOS_MPU60X0_ACCEL_16G);
			break;
		}
	}

#endif /* PIOS_INCLUDE_MPU6050 */

	//I2C is slow, sensor init as well, reset watchdog to prevent reset here
	//PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_HMC5883)
	{
		uint8_t magnetometer;
		HwSparkyMagnetometerGet(&magnetometer);

		external_mag_fail = false;
		
		if (magnetometer == HWSPARKY_MAGNETOMETER_EXTERNALI2CFLEXIPORT) {
			if (PIOS_HMC5883_Init(pios_i2c_flexi_id, &pios_hmc5883_external_cfg) == 0) {
				if (PIOS_HMC5883_Test() == 0) {
					// External mag configuration was successful

					// setup sensor orientation
					uint8_t ext_mag_orientation;
					HwSparkyExtMagOrientationGet(&ext_mag_orientation);

					enum pios_hmc5883_orientation hmc5883_externalOrientation = \
						(ext_mag_orientation == HWSPARKY_EXTMAGORIENTATION_TOP0DEGCW)      ? PIOS_HMC5883_TOP_0DEG      : \
						(ext_mag_orientation == HWSPARKY_EXTMAGORIENTATION_TOP90DEGCW)     ? PIOS_HMC5883_TOP_90DEG     : \
						(ext_mag_orientation == HWSPARKY_EXTMAGORIENTATION_TOP180DEGCW)    ? PIOS_HMC5883_TOP_180DEG    : \
						(ext_mag_orientation == HWSPARKY_EXTMAGORIENTATION_TOP270DEGCW)    ? PIOS_HMC5883_TOP_270DEG    : \
						(ext_mag_orientation == HWSPARKY_EXTMAGORIENTATION_BOTTOM0DEGCW)   ? PIOS_HMC5883_BOTTOM_0DEG   : \
						(ext_mag_orientation == HWSPARKY_EXTMAGORIENTATION_BOTTOM90DEGCW)  ? PIOS_HMC5883_BOTTOM_90DEG  : \
						(ext_mag_orientation == HWSPARKY_EXTMAGORIENTATION_BOTTOM180DEGCW) ? PIOS_HMC5883_BOTTOM_180DEG : \
						(ext_mag_orientation == HWSPARKY_EXTMAGORIENTATION_BOTTOM270DEGCW) ? PIOS_HMC5883_BOTTOM_270DEG : \
						pios_hmc5883_external_cfg.Default_Orientation;
					PIOS_HMC5883_SetOrientation(hmc5883_externalOrientation);
				}
				else
					external_mag_fail = true;  // External HMC5883 Test Failed
			}
			else
				external_mag_fail = true;  // External HMC5883 Init Failed
		}
	}
#endif /* PIOS_INCLUDE_HMC5883 */

	//I2C is slow, sensor init as well, reset watchdog to prevent reset here
	//PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MS5611)
	PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_internal_id);
	if (PIOS_MS5611_Test() != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_BARO);
#endif

#if defined(PIOS_INCLUDE_GPIO)
	PIOS_GPIO_Init();
#endif
}

/**
 * @}
 */
