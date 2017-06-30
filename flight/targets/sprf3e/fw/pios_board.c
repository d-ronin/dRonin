/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup SPRF3E SP Racing F3 Evo
 * @{
 *
 * @file       sprf3e/board-info/pios_board.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
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
#include <pios_hal.h>
#include <openpilot.h>
#include <uavobjectsinit.h>
#include "hwsprf3e.h"
#include "hwshared.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"

#if defined(PIOS_INCLUDE_WS2811)
#include "rgbledsettings.h"
#endif

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
#define PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN 40
uintptr_t pios_com_debug_id;
#endif	/* PIOS_INCLUDE_DEBUG_CONSOLE */

uintptr_t pios_com_aux_id;
uintptr_t pios_com_telem_rf_id;
uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_internal_adc_id;
uintptr_t pios_com_openlog_logging_id;

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void)
{

	/* Delay system */
	PIOS_DELAY_Init();

	const struct pios_board_info *bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_ANNUNC)
	const struct pios_annunc_cfg *led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
	PIOS_Assert(led_cfg);
	PIOS_ANNUNC_Init(led_cfg);
#endif	/* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_SPI)
	pios_spi_t pios_spi_gyro_id;

	/* Set up the SPI interface to the gyro/acelerometer */
	if (PIOS_SPI_Init(&pios_spi_gyro_id, &pios_spi_gyro_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
#endif /* PIOS_INCLUDE_SPI */

#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
	if (PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FLASH);

	/* Register the partition table */
	const struct pios_flash_partition *flash_partition_table;
	uint32_t num_partitions;
	flash_partition_table = PIOS_BOARD_HW_DEFS_GetPartitionTable(bdinfo->board_rev, &num_partitions);
	PIOS_FLASH_register_partition_table(flash_partition_table, num_partitions);

	/* Mount all filesystems */
	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_settings_cfg, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FILESYS);
#endif	/* PIOS_INCLUDE_FLASH */

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Initialize UAVObject libraries */
	UAVObjInitialize();

	/* Initialize the alarms library. Reads RCC reset flags */
	AlarmsInitialize();
	PIOS_RESET_Clear(); // Clear the RCC reset flags after use.

	/* Initialize the hardware UAVOs */
	HwSprf3eInitialize();
	ModuleSettingsInitialize();

#if defined(PIOS_INCLUDE_RTC)
	/* Initialize the real-time clock and its associated tick */
	PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif

	/* Initialize watchdog as early as possible to catch faults during init
	 * but do it only if there is no debugger connected
	 */
	if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0) {
		PIOS_WDG_Init();
	}

	/* Set up pulse timers */
	//inputs
	PIOS_TIM_InitClock(&tim_8_cfg);

	//outputs
	PIOS_TIM_InitClock(&tim_2_cfg);
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_15_cfg);
	PIOS_TIM_InitClock(&tim_16_cfg);
	PIOS_TIM_InitClock(&tim_17_cfg);

	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwSprf3eSetDefaults(HwSprf3eHandle(), 0);
		ModuleSettingsSetDefaults(ModuleSettingsHandle(), 0);
		AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
	}

#if defined(PIOS_INCLUDE_USB)
	/* Initialize board specific USB data */
	PIOS_USB_BOARD_DATA_Init();

	/* Flags to determine if various USB interfaces are advertised */
	bool usb_hid_present = false;

#if defined(PIOS_INCLUDE_USB_CDC)
	bool usb_cdc_present = false;
	if (PIOS_USB_DESC_HID_CDC_Init()) {
		PIOS_Assert(0);
	}
	usb_hid_present = true;
	usb_cdc_present = true;
#else
	if (PIOS_USB_DESC_HID_ONLY_Init()) {
		PIOS_Assert(0);
	}
	usb_hid_present = true;
#endif

	uintptr_t pios_usb_id;
	PIOS_USB_Init(&pios_usb_id, PIOS_BOARD_HW_DEFS_GetUsbCfg(bdinfo->board_rev));

#if defined(PIOS_INCLUDE_USB_CDC)

	uint8_t hw_usb_vcpport;
	/* Configure the USB VCP port */
	HwSprf3eUSB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWSPRF3E_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwSprf3eUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWSPRF3E_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif	/* PIOS_INCLUDE_USB_HID */
#endif	/* PIOS_INCLUDE_USB */

#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_internal_id, &pios_i2c_internal_cfg))
		PIOS_DEBUG_Assert(0);

	if (PIOS_I2C_CheckClear(pios_i2c_internal_id) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_I2C_INT);
	else
		if (AlarmsGet(SYSTEMALARMS_ALARM_I2C) == SYSTEMALARMS_ALARM_UNINITIALISED)
			AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_OK);
#endif  // PIOS_INCLUDE_I2C

	/* Configure the IO ports */
	HwSprf3eDSMxModeOptions hw_DSMxMode;
	HwSprf3eDSMxModeGet(&hw_DSMxMode);

	uint8_t hw_uart3;
	HwSprf3eUart3Get(&hw_uart3);
	PIOS_HAL_ConfigurePort(hw_uart3,		// port_type
			&pios_uart3_cfg,		// usart_port_cfg
			&pios_usart_com_driver,		// com_driver
			NULL,				// i2c_id
			NULL,				// i2c_cfg
			NULL,				// ppm_cfg
			NULL,				// pwm_cfg
			PIOS_LED_ALARM,			// led_id
			&pios_uart3_dsm_aux_cfg,	// dsm_cfg
			hw_DSMxMode,			// dsm_mode
			NULL);				// sbus_cfg

	uint8_t hw_uart2;
	HwSprf3eUart2Get(&hw_uart2);
	PIOS_HAL_ConfigurePort(hw_uart2,		// port_type
			&pios_uart2_cfg,		// usart_port_cfg
			&pios_usart_com_driver,		// com_driver
			NULL,				// i2c_id
			NULL,				// i2c_cfg
			&pios_ppm_cfg,			// ppm_cfg
			NULL,				// pwm_cfg
			PIOS_LED_ALARM,			// led_id
			&pios_uart2_dsm_aux_cfg,	// dsm_cfg
			hw_DSMxMode,			// dsm_mode
			NULL);				// sbus_cfg

	uint8_t hw_uart1;
	HwSprf3eUart1Get(&hw_uart1);
	PIOS_HAL_ConfigurePort(hw_uart1,		// port_type
			&pios_uart1_cfg,		// usart_port_cfg
			&pios_usart_com_driver,		// com_driver
			NULL,				// i2c_id
			NULL,				// i2c_cfg
			NULL,				// ppm_cfg
			NULL,				// pwm_cfg
			PIOS_LED_ALARM,			// led_id
			&pios_uart1_dsm_aux_cfg,	// dsm_cfg
			hw_DSMxMode,			// dsm_mode
			NULL);				// sbus_cfg

#if defined(PIOS_INCLUDE_GCSRCVR)
	GCSReceiverInitialize();
	uintptr_t pios_gcsrcvr_id;
	PIOS_GCSRCVR_Init(&pios_gcsrcvr_id);
	uintptr_t pios_gcsrcvr_rcvr_id;
	if (PIOS_RCVR_Init(&pios_gcsrcvr_rcvr_id, &pios_gcsrcvr_rcvr_driver, pios_gcsrcvr_id)) {
		PIOS_Assert(0);
	}
	pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_GCS] = pios_gcsrcvr_rcvr_id;
#endif	/* PIOS_INCLUDE_GCSRCVR */

#ifdef PIOS_INCLUDE_WS2811
	uint8_t num_leds;

	RGBLEDSettingsInitialize();

	RGBLEDSettingsNumLedsGet(&num_leds);

	PIOS_WS2811_init(&pios_ws2811, &pios_ws2811_cfg, num_leds);
	PIOS_WS2811_trigger_update(pios_ws2811);
#endif

#ifndef PIOS_DEBUG_ENABLE_DEBUG_PINS
#ifdef PIOS_INCLUDE_SERVO
	PIOS_Servo_Init(&pios_servo_cfg);
#endif
#else
	PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
#endif

#if defined(PIOS_INCLUDE_ADC)
	uint32_t internal_adc_id;
	if (PIOS_INTERNAL_ADC_Init(&internal_adc_id, &internal_adc_cfg) < 0)
		PIOS_Assert(0);
	PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id);
#endif /* PIOS_INCLUDE_ADC */
	PIOS_WDG_Clear();
	PIOS_DELAY_WaitmS(200);
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_BMP280)
#include "pios_bmp280.h"
#include "pios_bmp280_priv.h"
	uint8_t hw_baro;
	HwSprf3eBarometerGet(&hw_baro);
	switch(hw_baro) {
		case HWSPRF3E_BAROMETER_BMP280:
		if (PIOS_HAL_ConfigureExternalBaro(hw_baro,
			&pios_i2c_internal_id, &pios_i2c_internal_cfg))
			PIOS_SENSORS_SetMissing(PIOS_SENSOR_BARO);
		break;
	}
	PIOS_WDG_Clear();
#endif

#if defined(PIOS_INCLUDE_MPU)
	uint8_t hw_mag;
	uint8_t hw_orientation;
	HwSprf3eMagnetometerGet(&hw_mag);
	HwSprf3eExtMagOrientationGet(&hw_orientation);

	// XXX TODO MIXING ENUMS HERE NOT SAFE/CORRECT
	switch(hw_mag) {
		case HWSPRF3E_MAGNETOMETER_INTERNAL:
			pios_mpu_cfg.use_internal_mag=true;
			break;
		case HWSPRF3E_MAGNETOMETER_NONE:
			pios_mpu_cfg.use_internal_mag=false;
			break;
		case HWSPRF3E_MAGNETOMETER_EXTERNALHMC5883:
			pios_mpu_cfg.use_internal_mag=false;
#if defined(PIOS_INCLUDE_I2C)
			PIOS_WDG_Clear();
			if (PIOS_HAL_ConfigureExternalMag(hw_mag, hw_orientation,
				&pios_i2c_internal_id, &pios_i2c_internal_cfg))
				PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
			PIOS_WDG_Clear();
#else
			PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
#endif // PIOS_INCLUDE_I2C
			break;
		case HWSPRF3E_MAGNETOMETER_EXTERNALHMC5983:
			pios_mpu_cfg.use_internal_mag=false;
#ifdef PIOS_INCLUDE_HMC5983_I2C
			PIOS_WDG_Clear();
			if (PIOS_HAL_ConfigureExternalMag(hw_mag, hw_orientation,
				&pios_i2c_internal_id, &pios_i2c_internal_cfg))
				PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
			PIOS_WDG_Clear();
#else
			PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
#endif /* PIOS_INCLUDE_HMC5983_I2C */
			break;
	}

	pios_mpu_dev_t mpu_dev = NULL;
	if (PIOS_MPU_SPI_Init(&mpu_dev, pios_spi_gyro_id, 0, &pios_mpu_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

	HwSprf3eGyroRangeOptions hw_gyro_range;
	HwSprf3eGyroRangeGet(&hw_gyro_range);

	switch(hw_gyro_range) {
		case HWSPRF3E_GYRORANGE_250:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_250_DEG);
			break;
		case HWSPRF3E_GYRORANGE_500:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_500_DEG);
			break;
		case HWSPRF3E_GYRORANGE_1000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_1000_DEG);
			break;
		case HWSPRF3E_GYRORANGE_2000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_2000_DEG);
			break;
	}

	HwSprf3eAccelRangeOptions hw_accel_range;
	HwSprf3eAccelRangeGet(&hw_accel_range);
	switch(hw_accel_range) {
		case HWSPRF3E_ACCELRANGE_2G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_2G);
			break;
		case HWSPRF3E_ACCELRANGE_4G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_4G);
			break;
		case HWSPRF3E_ACCELRANGE_8G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_8G);
			break;
		case HWSPRF3E_ACCELRANGE_16G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_16G);
			break;
	}

	HwSprf3eMPU9250GyroLPFOptions hw_mpu_dlpf;
	HwSprf3eMPU9250GyroLPFGet(&hw_mpu_dlpf);
	uint16_t bandwidth = \
		(hw_mpu_dlpf == HWSPRF3E_MPU9250GYROLPF_184) ? 184 : \
		(hw_mpu_dlpf == HWSPRF3E_MPU9250GYROLPF_92)  ? 92  : \
		(hw_mpu_dlpf == HWSPRF3E_MPU9250GYROLPF_41)  ? 41  : \
		(hw_mpu_dlpf == HWSPRF3E_MPU9250GYROLPF_20)  ? 20  : \
		(hw_mpu_dlpf == HWSPRF3E_MPU9250GYROLPF_10)  ? 10  : \
		(hw_mpu_dlpf == HWSPRF3E_MPU9250GYROLPF_5)   ? 5   : \
		188;
	PIOS_MPU_SetGyroBandwidth(bandwidth);
#endif

	//I2C is slow, sensor init as well, reset watchdog to prevent reset here
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_GPIO)
	PIOS_GPIO_Init();
#endif

	/* Make sure we have at least one telemetry link configured or else fail initialization */
	PIOS_Assert(pios_com_telem_rf_id || pios_com_telem_usb_id);
}

/**
 * @}
 * @}
 */
