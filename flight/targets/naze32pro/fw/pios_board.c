/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup AFRO Naze32Pro
 * @{
 *
 * @file       naze32pro/fw/pios_board.c
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
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
#include "hwnaze32pro.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
#define PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN 40
uintptr_t pios_com_debug_id;
#endif	/* PIOS_INCLUDE_DEBUG_CONSOLE */

uintptr_t pios_com_aux_id;
uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_com_openlog_logging_id;

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void)
{
	const struct pios_board_info * bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_ANNUNC)
	const struct pios_annunc_cfg * led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
	PIOS_Assert(led_cfg);
	PIOS_ANNUNC_Init(led_cfg);
#endif	/* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_SPI)
	pios_spi_t pios_spi_internal_id;

	if (PIOS_SPI_Init(&pios_spi_internal_id, &pios_spi_internal_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
#endif

#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
	if (PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_HEARTBEAT, PIOS_HAL_PANIC_FLASH);;

	/* Register the partition table */
	const struct pios_flash_partition * flash_partition_table;
	uint32_t num_partitions;
	flash_partition_table = PIOS_BOARD_HW_DEFS_GetPartitionTable(bdinfo->board_rev, &num_partitions);
	PIOS_FLASH_register_partition_table(flash_partition_table, num_partitions);

	/* Mount all filesystems */
	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_settings_cfg, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_HEARTBEAT, PIOS_HAL_PANIC_FILESYS);

#endif	/* PIOS_INCLUDE_FLASH */

    PIOS_RESET_Clear(); // Clear the RCC reset flags after use.

	/* Initialize the hardware UAVOs */
	HwNaze32ProInitialize();
	ModuleSettingsInitialize();

#if defined(PIOS_INCLUDE_RTC)
	/* Initialize the real-time clock and its associated tick */
	PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif

#ifndef ERASE_FLASH
	/* Initialize watchdog as early as possible to catch faults during init
	 * but do it only if there is no debugger connected
	 */
	if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0) {
		PIOS_WDG_Init();
	}
#endif

    /* Set up pulse timers */

	//inputs
    PIOS_TIM_InitClock(&tim_1_cfg);
    PIOS_TIM_InitClock(&tim_16_cfg);

	//outputs
	PIOS_TIM_InitClock(&tim_2_cfg);
	PIOS_TIM_InitClock(&tim_15_cfg);
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_4_cfg);

	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwNaze32ProSetDefaults(HwNaze32ProHandle(), 0);
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
	HwNaze32ProUSB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWNAZE32PRO_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);

#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)

	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;

	HwNaze32ProUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWNAZE32PRO_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif	/* PIOS_INCLUDE_USB_HID */
#endif	/* PIOS_INCLUDE_USB */

	HwNaze32ProDSMxModeOptions hw_DSMxMode;
    HwNaze32ProDSMxModeGet(&hw_DSMxMode);

    /* Configure the IO ports */

	/* UART1 Port */
	uint8_t hw_uart1;
	HwNaze32ProUart1Get(&hw_uart1);

    PIOS_HAL_ConfigurePort(hw_uart1,             // port type protocol
            &pios_uart1_cfg,                     // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
            NULL,                                // i2c_id
            NULL,                                // i2c_cfg
            NULL,                                // ppm_cfg
            NULL,                                // pwm_cfg
            PIOS_LED_HEARTBEAT,                  // led_id
            &pios_uart1_dsm_bind_cfg,            // dsm_cfg
            hw_DSMxMode,                         // dsm_mode
            NULL);                               // sbus_cfg

	/* UART2 Port */
	uint8_t hw_uart2;
	HwNaze32ProUart2Get(&hw_uart2);

    PIOS_HAL_ConfigurePort(hw_uart2,             // port type protocol
            &pios_uart2_cfg,                     // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
            NULL,                                // i2c_id
            NULL,                                // i2c_cfg
            NULL,                                // ppm_cfg
            NULL,                                // pwm_cfg
            PIOS_LED_HEARTBEAT,                  // led_id
            &pios_uart2_dsm_bind_cfg,            // dsm_cfg
            hw_DSMxMode,                         // dsm_mode
            NULL);                               // sbus_cfg

	/* Configure the rcvr port */
	uint8_t hw_rcvrport;
	HwNaze32ProRcvrPortGet(&hw_rcvrport);

    PIOS_HAL_ConfigurePort(hw_rcvrport,          // port type protocol
            &pios_rcvr_usart_cfg,                // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
            NULL,                                // i2c_id
            NULL,                                // i2c_cfg
            &pios_ppm_cfg,                       // ppm_cfg
            NULL,                                // pwm_cfg
            PIOS_LED_HEARTBEAT,                  // led_id
            &pios_rcvr_dsm_bind_cfg,             // dsm_cfg
            hw_DSMxMode,                         // dsm_mode
            NULL);                               // sbus_cfg

#if defined PIOS_INCLUDE_ADC && defined PIOS_INCLUDE_SERVO

    uint8_t hw_outport;
	uint8_t number_of_pwm_outputs;
	uint8_t number_of_adc_ports;

	HwNaze32ProOutPortGet(&hw_outport);

	switch (hw_outport)
	{
	case HWNAZE32PRO_OUTPORT_PWM8:
		number_of_pwm_outputs = 8;
		number_of_adc_ports   = 0;
		break;

	case HWNAZE32PRO_OUTPORT_PWM8ADC1:
		number_of_pwm_outputs = 8;
		number_of_adc_ports   = 2;
		break;

	case HWNAZE32PRO_OUTPORT_PWM8ADC2:
		number_of_pwm_outputs = 8;
		number_of_adc_ports   = 3;
		break;

	case HWNAZE32PRO_OUTPORT_PWM8ADC3:
		number_of_pwm_outputs = 8;
		number_of_adc_ports   = 4;
		break;

	default:

		PIOS_Assert(0);
		break;
	}
#endif

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
	if(number_of_adc_ports > 0) {
		internal_adc_cfg.adc_pin_count = number_of_adc_ports;
		uintptr_t unused_adc;
		if(PIOS_INTERNAL_ADC_Init(&pios_internal_adc_id, &internal_adc_cfg) < 0)
			PIOS_Assert(0);
		PIOS_ADC_Init(&unused_adc, &pios_internal_adc_driver, pios_internal_adc_id);
	}
#endif /* PIOS_INCLUDE_ADC */

    PIOS_WDG_Clear();
	PIOS_DELAY_WaitmS(200);
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MPU)
	pios_mpu_dev_t mpu_dev = NULL;
	if (PIOS_MPU_SPI_Init(&mpu_dev, pios_spi_internal_id, 0, &pios_mpu_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_HEARTBEAT, PIOS_HAL_PANIC_IMU);

	HwNaze32ProGyroRangeOptions hw_gyro_range;
	HwNaze32ProGyroRangeGet(&hw_gyro_range);

	switch(hw_gyro_range) {
		case HWNAZE32PRO_GYRORANGE_250:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_250_DEG);
			break;

		case HWNAZE32PRO_GYRORANGE_500:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_500_DEG);
			break;

		case HWNAZE32PRO_GYRORANGE_1000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_1000_DEG);
			break;

		case HWNAZE32PRO_GYRORANGE_2000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_2000_DEG);
			break;
	}

	HwNaze32ProAccelRangeOptions hw_accel_range;
	HwNaze32ProAccelRangeGet(&hw_accel_range);
	switch(hw_accel_range) {
		case HWNAZE32PRO_ACCELRANGE_2G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_2G);
			break;

		case HWNAZE32PRO_ACCELRANGE_4G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_4G);
			break;

		case HWNAZE32PRO_ACCELRANGE_8G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_8G);
			break;

		case HWNAZE32PRO_ACCELRANGE_16G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_16G);
			break;
	}

	// the filter has to be set before rate else divisor calculation will fail
	HwNaze32ProMPU6000DLPFOptions hw_mpu_dlpf;
	HwNaze32ProMPU6000DLPFGet(&hw_mpu_dlpf);
	uint16_t bandwidth = \
	    (hw_mpu_dlpf == HWNAZE32PRO_MPU6000DLPF_188) ? 188 : \
	    (hw_mpu_dlpf == HWNAZE32PRO_MPU6000DLPF_98)  ? 98  : \
	    (hw_mpu_dlpf == HWNAZE32PRO_MPU6000DLPF_42)  ? 42  : \
	    (hw_mpu_dlpf == HWNAZE32PRO_MPU6000DLPF_20)  ? 20  : \
	    (hw_mpu_dlpf == HWNAZE32PRO_MPU6000DLPF_10)  ? 10 : \
	    (hw_mpu_dlpf == HWNAZE32PRO_MPU6000DLPF_5)   ? 5   : \
	    188;
	PIOS_MPU_SetGyroBandwidth(bandwidth);

#endif /* PIOS_INCLUDE_MPU6000 */

   PIOS_WDG_Clear();

    uint8_t hw_magnetometer;
	HwNaze32ProMagnetometerGet(&hw_magnetometer);

	if (hw_magnetometer == HWNAZE32PRO_MAGNETOMETER_INTERNAL)
	{
#if defined(PIOS_INCLUDE_HMC5983)
        if (PIOS_HMC5983_SPI_Init(pios_spi_internal_id, 1, &pios_hmc5983_cfg) != 0)
		    PIOS_HAL_CriticalError(PIOS_LED_HEARTBEAT, PIOS_HAL_PANIC_MAG);

	    PIOS_WDG_Clear();
#endif /* PIOS_INCLUDE_HMC5983 */
	}
	else
	{
		uint8_t hw_ext_mag_orientation;

		HwNaze32ProExtMagOrientationGet(&hw_ext_mag_orientation);

		PIOS_HAL_ConfigureExternalMag(hw_magnetometer, hw_ext_mag_orientation, &pios_i2c_external_id, &pios_i2c_external_cfg);
	}

    PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MS5611_SPI)
	if (PIOS_MS5611_SPI_Init(pios_spi_internal_id, 2, &pios_ms5611_cfg) != 0) {
		PIOS_Assert(0);
	}
#endif	/* PIOS_INCLUDE_MS5611_SPI */

    PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_GPIO)
	PIOS_GPIO_Init();
#endif

	/* Make sure we have at least one telemetry link configured or else fail initialization */
	PIOS_Assert(pios_com_telem_serial_id || pios_com_telem_usb_id);
}

/**
 * @}
 */
