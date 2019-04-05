/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup MATEK405 Matek MATEK405
 * @{
 *
 * @file       matek405/fw/pios_board.c
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
#include "hwmatek405.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"
#include "rgbledsettings.h"

uintptr_t pios_com_openlog_logging_id;
uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_internal_adc_id;

#ifdef PIOS_INCLUDE_MAX7456
max7456_dev_t pios_max7456_id;
#endif

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void) {
	const struct pios_board_info * bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_ANNUNC)
	const struct pios_annunc_cfg * led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
	PIOS_Assert(led_cfg);
	PIOS_ANNUNC_Init(led_cfg);
#endif    /* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_SPI)
	if (PIOS_SPI_Init(&pios_spi_gyro_id, &pios_spi_gyro_cfg)) {
		PIOS_Assert(0);
	}
#endif

#if defined(PIOS_INCLUDE_FLASH)
	/* Initialize all flash drivers */
	if (PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FLASH);

	/* Register the partition table */
	const struct pios_flash_partition * flash_partition_table;
	uint32_t num_partitions;
	flash_partition_table = PIOS_BOARD_HW_DEFS_GetPartitionTable(bdinfo->board_rev, &num_partitions);
	PIOS_FLASH_register_partition_table(flash_partition_table, num_partitions);

	/* Mount all filesystems */
	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_settings_cfg, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FILESYS);
#endif    /* PIOS_INCLUDE_FLASH */

	HwMatek405Initialize();
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

	HwMatek405FC_Pad_S6Options hw_fcPadS6;
	HwMatek405FC_Pad_S6Get(&hw_fcPadS6);
	
	/* Set up pulse timers */

	// Timers used for inputs (5)
	PIOS_TIM_InitClock(&tim_5_cfg);

	// Timers used for outputs (3, 8, 2, 1, 4)
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_8_cfg);
	PIOS_TIM_InitClock(&tim_2_cfg);
	
	if (hw_fcPadS6 == HWMATEK405_FC_PAD_S6_PWM_6)
		PIOS_TIM_InitClock(&tim_1_cfg);
	
	PIOS_TIM_InitClock(&tim_4_cfg);
	
#ifdef PIOS_INCLUDE_SPI
	uint8_t hw_osdport;

	HwMatek405OSDPortGet(&hw_osdport);

	if (hw_osdport != HWMATEK405_OSDPORT_DISABLED) {
		if (PIOS_SPI_Init(&pios_spi_osd_id, &pios_spi_osd_cfg)) {
			PIOS_Assert(0);
		}

		switch (hw_osdport) {
#ifdef PIOS_INCLUDE_MAX7456
			case HWMATEK405_OSDPORT_MAX7456OSD:
				if (!PIOS_MAX7456_init(&pios_max7456_id, pios_spi_osd_id, 0)) {
					// XXX do something?
				}
				break;
#endif
			default:
				PIOS_Assert(0);
				break;
		}
	}
#endif /* PIOS_INCLUDE_SPI */

	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwMatek405SetDefaults(HwMatek405Handle(), 0);
		ModuleSettingsSetDefaults(ModuleSettingsHandle(),0);
		AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
	}

#if defined(PIOS_INCLUDE_USB)
	/* Initialize board specific USB data */
	PIOS_USB_BOARD_DATA_Init();

	/* Flags to determine if various USB interfaces are advertised */
	bool usb_hid_present = false;
	bool usb_cdc_present = false;

#if defined(PIOS_INCLUDE_USB_CDC)
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
	HwMatek405USB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWMATEK405_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);

#endif    /* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwMatek405USB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWMATEK405_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif    /* PIOS_INCLUDE_USB_HID */

	if (usb_hid_present || usb_cdc_present) {
		PIOS_USBHOOK_Activate();
	}

	PIOS_WDG_Clear();
#endif    /* PIOS_INCLUDE_USB */

	/* Configure the IO ports */

#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_internal_id, &pios_i2c_internal_cfg))
		PIOS_DEBUG_Assert(0);

	if (PIOS_I2C_CheckClear(pios_i2c_internal_id) != 0)
		AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_WARNING);
	else
		if (AlarmsGet(SYSTEMALARMS_ALARM_I2C) == SYSTEMALARMS_ALARM_UNINITIALISED)
			AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_OK);
#endif  // PIOS_INCLUDE_I2C

	HwMatek405DSMxModeOptions hw_DSMxMode;
	HwMatek405DSMxModeGet(&hw_DSMxMode);

	/* UART1 Port */
	uint8_t hw_uart1;
	HwMatek405Uart1Get(&hw_uart1);

	PIOS_HAL_ConfigurePort(hw_uart1,             // port type protocol
			&pios_usart1_cfg,                    // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			NULL,                                // i2c_id
			NULL,                                // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			&pios_usart1_dsm_aux_cfg,            // dsm_cfg
			hw_DSMxMode,                         // dsm_mode
			NULL);                               // sbus_cfg

	uint8_t hw_ppm;
	HwMatek405PPM_ReceiverGet(&hw_ppm);

	if (hw_ppm == HWMATEK405_PPM_RECEIVER_ENABLED) {
		PIOS_HAL_ConfigurePort(HWSHARED_PORTTYPES_PPM,  // port type protocol
				NULL,                                   // usart_port_cfg
				NULL,                                   // com_driver
				NULL,                                   // i2c_id
				NULL,                                   // i2c_cfg
				&pios_ppm_cfg,                          // ppm_cfg
				NULL,                                   // pwm_cfg
				PIOS_LED_ALARM,                         // led_id
				NULL,                                   // dsm_cfg
				0,                                      // dsm_mode
				NULL);                                  // sbus_cfg
	} else {
		/* UART2 Port */
		uint8_t hw_uart2;
		HwMatek405Uart2Get(&hw_uart2);

		PIOS_HAL_ConfigurePort(hw_uart2,             // port type protocol
				&pios_usart2_cfg,                    // usart_port_cfg
				&pios_usart_com_driver,              // com_driver
				NULL,                                // i2c_id
				NULL,                                // i2c_cfg
				NULL,                                // ppm_cfg
				NULL,                                // pwm_cfg
				PIOS_LED_ALARM,                      // led_id
				&pios_usart2_dsm_aux_cfg,            // dsm_cfg
				hw_DSMxMode,                         // dsm_mode
				NULL);                               // sbus_cfg
	}

	/* UART3 Port */
	uint8_t hw_uart3;
	HwMatek405Uart3Get(&hw_uart3);

	PIOS_HAL_ConfigurePort(hw_uart3,             // port type protocol
			&pios_usart3_cfg,                    // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			NULL,                                // i2c_id
			NULL,                                // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			&pios_usart3_dsm_aux_cfg,            // dsm_cfg
			hw_DSMxMode,                         // dsm_mode
			NULL);                               // sbus_cfg

	/* UART4 Port */
	uint8_t hw_uart4;
	HwMatek405Uart4Get(&hw_uart4);

	PIOS_HAL_ConfigurePort(hw_uart4,             // port type protocol
			&pios_usart4_cfg,                    // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			NULL,                                // i2c_id
			NULL,                                // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			&pios_usart4_dsm_aux_cfg,            // dsm_cfg
			hw_DSMxMode,                         // dsm_mode
			NULL);                               // sbus_cfg

	/* UART5 Port */
	uint8_t hw_uart5;
	HwMatek405Uart5Get(&hw_uart5);

	PIOS_HAL_ConfigurePort(hw_uart5,             // port type protocol
			&pios_usart5_cfg,                    // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			NULL,                                // i2c_id
			NULL,                                // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			&pios_usart5_dsm_aux_cfg,            // dsm_cfg
			hw_DSMxMode,                         // dsm_mode
			NULL);                               // sbus_cfg

#if defined(PIOS_INCLUDE_SERVO) & defined(PIOS_INCLUDE_DMASHOT)
	if (hw_fcPadS6 == HWMATEK405_FC_PAD_S6_PWM_6) 
		PIOS_DMAShot_Init(&dmashot_config_without_led_string);
	else
		PIOS_DMAShot_Init(&dmashot_config_with_led_string);
#endif

#if defined(PIOS_INCLUDE_TIM) && defined(PIOS_INCLUDE_SERVO)
#ifndef PIOS_DEBUG_ENABLE_DEBUG_PINS
	if (hw_fcPadS6 == HWMATEK405_FC_PAD_S6_PWM_6) 
		PIOS_Servo_Init(&pios_servo_cfg_without_led_string);
	else
		PIOS_Servo_Init(&pios_servo_cfg_with_led_string);
#else
	PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
#endif
#endif

#ifdef PIOS_INCLUDE_WS2811
	if (hw_fcPadS6 == HWMATEK405_FC_PAD_S6_LEDSTRING) {
		RGBLEDSettingsInitialize();

		uint8_t temp;

		RGBLEDSettingsNumLedsGet(&temp);

		if (temp > 0) {
			PIOS_WS2811_init(&pios_ws2811, &pios_ws2811_cfg, temp);

			// Drive default value (off) to strand once at startup
			PIOS_WS2811_trigger_update(pios_ws2811);
		}
	}
#endif

#ifdef PIOS_INCLUDE_DAC
	PIOS_DAC_init(&pios_dac, &pios_dac_cfg);

	PIOS_HAL_ConfigureDAC(pios_dac);
#endif /* PIOS_INCLUDE_DAC */

/* init sensor queue registration */
	PIOS_SENSORS_Init();

	PIOS_WDG_Clear();
	PIOS_DELAY_WaitmS(200);
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MPU)
	pios_mpu_dev_t mpu_dev = NULL;
	if (PIOS_MPU_SPI_Init(&mpu_dev, pios_spi_gyro_id, 0, &pios_mpu_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

	PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_2000_DEG);
	
	PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_8G);
	
	// From pios_mpu.h:
	//   PIOS_MPU60X0_GYRO_LOWPASS_188_HZ = 0x01,    (both gyro and accel)
	//   PIOS_ICM206XX_GYRO_LOWPASS_176_HZ  = 0x01,  (gyro only)
	//   So calling PIOS_MPU_SetGyroBandwidth with an argument of 0x01
	//   will set the desired bandwidth for either imu.
	
	PIOS_MPU_SetGyroBandwidth(0x01);

	// PIOS_MPU_SetAccelBandwidth returns immediately if 
	// mpu_dev->mpu_type is set to PIOS_MPU60X0, so safe
	// to call for either imu.
	
	PIOS_MPU_SetAccelBandwidth(PIOS_ICM206XX_ACCEL_LOWPASS_218_HZ);
#endif

#if defined(PIOS_INCLUDE_I2C)
	PIOS_WDG_Clear();

	uint8_t hw_ext_mag;
	HwMatek405MagnetometerGet(&hw_ext_mag);
	
	if (hw_ext_mag != HWMATEK405_MAGNETOMETER_NONE)	{
		if (pios_i2c_internal_id) {
			uint8_t hw_orientation;
			HwMatek405ExtMagOrientationGet(&hw_orientation);
			
			PIOS_HAL_ConfigureExternalMag(hw_ext_mag, hw_orientation,
				&pios_i2c_internal_id,
				&pios_i2c_internal_cfg);
		} else {
			PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
		}
	}

#if defined(PIOS_INCLUDE_BMP280)
	// BMP280 support
	if (PIOS_BMP280_Init(&pios_bmp280_cfg, pios_i2c_internal_id)) {
		PIOS_SENSORS_SetMissing(PIOS_SENSOR_BARO);
	} else if (PIOS_BMP280_Test()) {
		PIOS_SENSORS_SetMissing(PIOS_SENSOR_BARO);
	}
#endif

	//I2C is slow, sensor init as well, reset watchdog to prevent reset here
	PIOS_WDG_Clear();

#endif    /* PIOS_INCLUDE_I2C */

#if defined(PIOS_INCLUDE_ADC)
	/* Configure the adc port(s) */
	
	uintptr_t internal_adc_id;

	if (PIOS_INTERNAL_ADC_Init(&internal_adc_id, &pios_adc_cfg) < 0) {
		PIOS_Assert(0);
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_ADC);
	}

	if (PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id) < 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_ADC);
#endif

	/* Make sure we have at least one telemetry link configured or else fail initialization */
	PIOS_Assert(pios_com_telem_serial_id || pios_com_telem_usb_id);
}

/**
 * @}
 * @}
 */
