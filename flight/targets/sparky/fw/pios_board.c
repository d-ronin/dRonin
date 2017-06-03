/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup Sparky Tau Labs Sparky
 * @{
 *
 * @file       sparky/fw/pios_board.c
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
#include "hwsparky.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"


#define PIOS_COM_CAN_RX_BUF_LEN 256
#define PIOS_COM_CAN_TX_BUF_LEN 256

uintptr_t pios_com_aux_id;
uintptr_t pios_com_can_id;
uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_internal_adc_id;
uintptr_t pios_can_id;
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

#if defined(PIOS_INCLUDE_CAN)
	if (PIOS_CAN_Init(&pios_can_id, &pios_can_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_CAN);

	if (PIOS_COM_Init(&pios_com_can_id, &pios_can_com_driver, pios_can_id,
	                  PIOS_COM_CAN_RX_BUF_LEN,
	                  PIOS_COM_CAN_TX_BUF_LEN))
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_CAN);

	pios_com_bridge_id = pios_com_can_id;
#endif

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
#if defined(ERASE_FLASH)
	PIOS_FLASHFS_Format(pios_uavo_settings_fs_id);
#endif

#endif	/* PIOS_INCLUDE_FLASH */

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Initialize UAVObject libraries */
	UAVObjInitialize();

	/* Initialize the alarms library. Reads RCC reset flags */
	AlarmsInitialize();
	PIOS_RESET_Clear(); // Clear the RCC reset flags after use.

	/* Initialize the hardware UAVOs */
	HwSparkyInitialize();
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

	//outputs
	PIOS_TIM_InitClock(&tim_1_cfg);
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
		HwSparkySetDefaults(HwSparkyHandle(), 0);
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
	HwSparkyUSB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWSPARKY_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwSparkyUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWSPARKY_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif	/* PIOS_INCLUDE_USB_HID */
#endif	/* PIOS_INCLUDE_USB */

	/* Configure the IO ports */
	
#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_internal_id, &pios_i2c_internal_cfg))
		PIOS_DEBUG_Assert(0);

	if (PIOS_I2C_CheckClear(pios_i2c_internal_id) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_I2C_INT);
	else
		if (AlarmsGet(SYSTEMALARMS_ALARM_I2C) == SYSTEMALARMS_ALARM_UNINITIALISED)
			AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_OK);
#endif  // PIOS_INCLUDE_I2C

	HwSparkyDSMxModeOptions hw_DSMxMode;
	HwSparkyDSMxModeGet(&hw_DSMxMode);

	/* Configure main USART port */
	uint8_t hw_mainport;
	HwSparkyMainPortGet(&hw_mainport);
	
	PIOS_HAL_ConfigurePort(hw_mainport,          // port type protocol
	        &pios_main_usart_cfg,                // usart_port_cfg
	        &pios_usart_com_driver,              // com_driver
	        NULL,                                // i2c_id
	        NULL,                                // i2c_cfg
	        NULL,                                // ppm_cfg
	        NULL,                                // pwm_cfg
	        PIOS_LED_ALARM,                      // led_id
	        &pios_main_dsm_aux_cfg,              // dsm_cfg
	        hw_DSMxMode,                         // dsm_mode
	        NULL);                               // sbus_cfg

	/* Configure FlexiPort */
	uint8_t hw_flexiport;
	HwSparkyFlexiPortGet(&hw_flexiport);
	
	PIOS_HAL_ConfigurePort(hw_flexiport,         // port type protocol
	        &pios_flexi_usart_cfg,               // usart_port_cfg
	        &pios_usart_com_driver,              // com_driver
	        &pios_i2c_flexi_id,                  // i2c_id
	        &pios_i2c_flexi_cfg,                 // i2c_cfg
	        NULL,                                // ppm_cfg
	        NULL,                                // pwm_cfg
	        PIOS_LED_ALARM,                      // led_id
	        &pios_flexi_dsm_aux_cfg,             // dsm_cfg
	        hw_DSMxMode,                         // dsm_mode
	        NULL);                               // sbus_cfg

	/* Configure the rcvr port */
	uint8_t hw_rcvrport;
	HwSparkyRcvrPortGet(&hw_rcvrport);

	PIOS_HAL_ConfigurePort(hw_rcvrport,          // port type protocol
	        &pios_rcvr_usart_cfg,                // usart_port_cfg
	        &pios_usart_com_driver,              // com_driver
	        NULL,                                // i2c_id
	        NULL,                                // i2c_cfg
	        &pios_ppm_cfg,                       // ppm_cfg
	        NULL,                                // pwm_cfg
	        PIOS_LED_ALARM,                      // led_id
	        &pios_rcvr_dsm_aux_cfg,              // dsm_cfg
	        hw_DSMxMode,                         // dsm_mode
	        NULL);                               // sbus_cfg

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

	uint8_t hw_outport;
	uint8_t number_of_pwm_outputs;
	uint8_t number_of_adc_ports;
	bool use_pwm_in;
	HwSparkyOutPortGet(&hw_outport);
	switch (hw_outport) {
	case HWSPARKY_OUTPORT_PWM10:
		number_of_pwm_outputs = 10;
		number_of_adc_ports = 0;
		use_pwm_in = false;
		break;
	case HWSPARKY_OUTPORT_PWM82ADC:
		number_of_pwm_outputs = 8;
		number_of_adc_ports = 2;
		use_pwm_in = false;
		break;
	case HWSPARKY_OUTPORT_PWM73ADC:
		number_of_pwm_outputs = 7;
		number_of_adc_ports = 3;
		use_pwm_in = false;
		break;
	case HWSPARKY_OUTPORT_PWM9PWM_IN:
		number_of_pwm_outputs = 9;
		use_pwm_in = true;
		number_of_adc_ports = 0;
		break;
	case HWSPARKY_OUTPORT_PWM72ADCPWM_IN:
		number_of_pwm_outputs = 7;
		use_pwm_in = true;
		number_of_adc_ports = 2;
		break;
	default:
		PIOS_Assert(0);
		break;
	}

#ifndef PIOS_DEBUG_ENABLE_DEBUG_PINS
#ifdef PIOS_INCLUDE_SERVO
	pios_servo_cfg.num_channels = number_of_pwm_outputs;

	if (hw_rcvrport != HWSHARED_PORTTYPES_PPM) {
		PIOS_Servo_Init(&pios_servo_cfg);
	} else {
		PIOS_Servo_Init(&pios_servo_slow_cfg);
	}
#endif
#else
	PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
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
#if defined(PIOS_INCLUDE_PWM)
	if (use_pwm_in) {
		if (number_of_adc_ports > 0)
			pios_pwm_cfg.channels = &pios_tim_rcvrport_pwm[1];
		uintptr_t pios_pwm_id;
		PIOS_PWM_Init(&pios_pwm_id, &pios_pwm_cfg);

		uintptr_t pios_pwm_rcvr_id;
		if (PIOS_RCVR_Init(&pios_pwm_rcvr_id, &pios_pwm_rcvr_driver, pios_pwm_id)) {
			PIOS_Assert(0);
		}
		pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM] = pios_pwm_rcvr_id;
	}
#endif	/* PIOS_INCLUDE_PWM */
	PIOS_WDG_Clear();
	PIOS_DELAY_WaitmS(200);
	PIOS_WDG_Clear();

	uint8_t hw_magnetometer;
	HwSparkyMagnetometerGet(&hw_magnetometer);

#if defined(PIOS_INCLUDE_MPU)
	if (hw_magnetometer != HWSPARKY_MAGNETOMETER_INTERNAL)
                pios_mpu_cfg.use_internal_mag = false;

	pios_mpu_dev_t mpu_dev = NULL;

	if (PIOS_MPU_I2C_Init(&mpu_dev, pios_i2c_internal_id, &pios_mpu_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

	HwSparkyGyroRangeOptions hw_gyro_range;
	HwSparkyGyroRangeGet(&hw_gyro_range);
	switch(hw_gyro_range) {
		case HWSPARKY_GYRORANGE_250:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_250_DEG);
			break;
		case HWSPARKY_GYRORANGE_500:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_500_DEG);
			break;
		case HWSPARKY_GYRORANGE_1000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_1000_DEG);
			break;
		case HWSPARKY_GYRORANGE_2000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_2000_DEG);
			break;
	}

	HwSparkyAccelRangeOptions hw_accel_range;
	HwSparkyAccelRangeGet(&hw_accel_range);
	switch(hw_accel_range) {
		case HWSPARKY_ACCELRANGE_2G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_2G);
			break;
		case HWSPARKY_ACCELRANGE_4G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_4G);
			break;
		case HWSPARKY_ACCELRANGE_8G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_8G);
			break;
		case HWSPARKY_ACCELRANGE_16G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_16G);
			break;
	}

	uint8_t hw_mpu_dlpf;
	HwSparkyMPU9150DLPFGet(&hw_mpu_dlpf);

	uint16_t bandwidth = \
		(hw_mpu_dlpf == HWSPARKY_MPU9150DLPF_188) ? 188 : \
		(hw_mpu_dlpf == HWSPARKY_MPU9150DLPF_98)  ? 98  : \
		(hw_mpu_dlpf == HWSPARKY_MPU9150DLPF_42)  ? 42  : \
		(hw_mpu_dlpf == HWSPARKY_MPU9150DLPF_20)  ? 20  : \
		(hw_mpu_dlpf == HWSPARKY_MPU9150DLPF_10)  ? 10  : \
		(hw_mpu_dlpf == HWSPARKY_MPU9150DLPF_5)   ? 5   : \
		188;
	PIOS_MPU_SetGyroBandwidth(bandwidth);
#endif /* PIOS_INCLUDE_MPU */

	/* I2C is slow, sensor init as well, reset watchdog to prevent reset here */
	PIOS_WDG_Clear();

	switch (hw_magnetometer) {
		case HWSPARKY_MAGNETOMETER_NONE:
		case HWSPARKY_MAGNETOMETER_INTERNAL:
			/* internal handled by MPU code above */
			break;

		/* default external mags and handle them in PiOS HAL rather than maintaining list here */
		default:
			if (hw_flexiport == HWSHARED_PORTTYPES_I2C) {
				uint8_t hw_orientation;
				HwSparkyExtMagOrientationGet(&hw_orientation);

				PIOS_HAL_ConfigureExternalMag(hw_magnetometer, hw_orientation,
					&pios_i2c_flexi_id, &pios_i2c_flexi_cfg);
			} else {
				PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
			}
			break;
	}

	/* I2C is slow, sensor init as well, reset watchdog to prevent reset here */
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MS5611)
	if ((PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_internal_id) != 0)
			|| (PIOS_MS5611_Test() != 0))
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_BARO);
#endif

	/* Make sure we have at least one telemetry link configured or else fail initialization */
	PIOS_Assert(pios_com_telem_serial_id || pios_com_telem_usb_id);
}

/**
 * @}
 * @}
 */
