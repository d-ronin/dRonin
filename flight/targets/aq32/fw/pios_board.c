/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup AQ32 Aeroquad AQ32
 * @{
 *
 * @file       aq32/fw/pios_board.c
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
#include "hwaq32.h"
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
    /* Delay system */
    PIOS_DELAY_Init();

    const struct pios_board_info * bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_ANNUNC)
    const struct pios_annunc_cfg * led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
    PIOS_Assert(led_cfg);
    PIOS_ANNUNC_Init(led_cfg);
#endif    /* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_SPI)
    if (PIOS_SPI_Init(&pios_spi_internal_id, &pios_spi_internal_cfg)) {
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

    /* Initialize the task monitor library */
    TaskMonitorInitialize();

    /* Initialize UAVObject libraries */
    UAVObjInitialize();

    /* Initialize the alarms library */
    AlarmsInitialize();

    HwAQ32Initialize();
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

    // Timers used for inputs (5)
    PIOS_TIM_InitClock(&tim_5_cfg);

    // Timers used for outputs (2, 3, 4, 8)
    PIOS_TIM_InitClock(&tim_2_cfg);
    PIOS_TIM_InitClock(&tim_3_cfg);
    PIOS_TIM_InitClock(&tim_4_cfg);
    PIOS_TIM_InitClock(&tim_8_cfg);

#ifdef PIOS_INCLUDE_SPI
    uint8_t hw_osdport;

    HwAQ32OSDPortGet(&hw_osdport);

    if (hw_osdport != HWAQ32_OSDPORT_DISABLED) {
        if (PIOS_SPI_Init(&pios_spi_external_id, &pios_spi_external_cfg)) {
            PIOS_Assert(0);
        }

        switch (hw_osdport) {
#ifdef PIOS_INCLUDE_MAX7456
            case HWAQ32_OSDPORT_MAX7456OSD:
                if (!PIOS_MAX7456_init(&pios_max7456_id,
                        pios_spi_external_id, 0)) {
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
        HwAQ32SetDefaults(HwAQ32Handle(), 0);
        ModuleSettingsSetDefaults(ModuleSettingsHandle(),0);
        AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
    }

#if defined(PIOS_INCLUDE_USB)
    /* Initialize USB disconnect GPIO */
    GPIO_Init(pios_usb_main_cfg.disconnect.gpio, (GPIO_InitTypeDef*)&pios_usb_main_cfg.disconnect.init);
    GPIO_SetBits(pios_usb_main_cfg.disconnect.gpio, pios_usb_main_cfg.disconnect.init.GPIO_Pin);

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
    HwAQ32USB_VCPPortGet(&hw_usb_vcpport);

    if (!usb_cdc_present) {
        /* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
        hw_usb_vcpport = HWAQ32_USB_VCPPORT_DISABLED;
    }

    PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);

#endif    /* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
    /* Configure the usb HID port */
    uint8_t hw_usb_hidport;
    HwAQ32USB_HIDPortGet(&hw_usb_hidport);

    if (!usb_hid_present) {
        /* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
        hw_usb_hidport = HWAQ32_USB_HIDPORT_DISABLED;
    }

    PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif    /* PIOS_INCLUDE_USB_HID */

    if (usb_hid_present || usb_cdc_present) {
        PIOS_USBHOOK_Activate();
    }

    /* Issue USB Disconnect Pulse */
    PIOS_WDG_Clear();

    GPIO_ResetBits(pios_usb_main_cfg.disconnect.gpio, pios_usb_main_cfg.disconnect.init.GPIO_Pin);

    PIOS_DELAY_WaitmS(200);

    GPIO_SetBits(pios_usb_main_cfg.disconnect.gpio, pios_usb_main_cfg.disconnect.init.GPIO_Pin);

    PIOS_WDG_Clear();
#endif    /* PIOS_INCLUDE_USB */

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

    HwAQ32DSMxModeOptions hw_DSMxMode;
    HwAQ32DSMxModeGet(&hw_DSMxMode);

    /* UART1 Port */
    uint8_t hw_uart1;
    HwAQ32Uart1Get(&hw_uart1);

    PIOS_HAL_ConfigurePort(hw_uart1,             // port type protocol
            &pios_usart1_cfg,                    // usart_port_cfg
            &pios_usart_com_driver,              // com_driver
            NULL,                                // i2c_id
            NULL,                                // i2c_cfg
            NULL,                                // ppm_cfg
            NULL,                                // pwm_cfg
            PIOS_LED_ALARM,                      // led_id
            NULL,                                // dsm_cfg
            0,                                   // dsm_mode
            NULL);                               // sbus_cfg

    /* UART2 Port */
    uint8_t hw_uart2;
    HwAQ32Uart2Get(&hw_uart2);

    PIOS_HAL_ConfigurePort(hw_uart2,             // port type protocol
            &pios_usart2_cfg,                    // usart_port_cfg
            &pios_usart_com_driver,              // com_driver
            NULL,                                // i2c_id
            NULL,                                // i2c_cfg
            NULL,                                // ppm_cfg
            NULL,                                // pwm_cfg
            PIOS_LED_ALARM,                      // led_id
            NULL,                                // dsm_cfg
            0,                                   // dsm_mode
            NULL);                               // sbus_cfg

    /* UART3 Port */
    uint8_t hw_uart3;
    HwAQ32Uart3Get(&hw_uart3);

    PIOS_HAL_ConfigurePort(hw_uart3,             // port type protocol
            &pios_usart3_cfg,                    // usart_port_cfg
            &pios_usart_com_driver,              // com_driver
            NULL,                                // i2c_id
            NULL,                                // i2c_cfg
            NULL,                                // ppm_cfg
            NULL,                                // pwm_cfg
            PIOS_LED_ALARM,                      // led_id
            NULL,                                // dsm_cfg
            0,                                   // dsm_mode
            &pios_usart3_sbus_aux_cfg);          // sbus_cfg

    if (hw_uart3 == HWAQ32_UART3_FRSKYSENSORHUB)
    {
        GPIO_Init(pios_usart3_sbus_aux_cfg.inv.gpio, (GPIO_InitTypeDef*)&pios_usart3_sbus_aux_cfg.inv.init);
        GPIO_WriteBit(pios_usart3_sbus_aux_cfg.inv.gpio, pios_usart3_sbus_aux_cfg.inv.init.GPIO_Pin, pios_usart3_sbus_aux_cfg.gpio_inv_enable);
    }

    /* UART4 Port */
    uint8_t hw_uart4;
    HwAQ32Uart4Get(&hw_uart4);

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

    /* UART6 Port */
    uint8_t hw_uart6;
    HwAQ32Uart6Get(&hw_uart6);

    PIOS_HAL_ConfigurePort(hw_uart6,             // port type protocol
            &pios_usart6_cfg,                    // usart_port_cfg
            &pios_usart_com_driver,              // com_driver
            NULL,                                // i2c_id
            NULL,                                // i2c_cfg
            NULL,                                // ppm_cfg
            NULL,                                // pwm_cfg
            PIOS_LED_ALARM,                      // led_id
            &pios_usart6_dsm_aux_cfg,            // dsm_cfg
            hw_DSMxMode,                         // dsm_mode
            NULL);                               // sbus_cfg

	uint8_t hw_rcvrport;
    HwAQ32RcvrPortGet(&hw_rcvrport);

    /* Configure the rcvr port */
    PIOS_HAL_ConfigurePort(hw_rcvrport,          // port type protocol
            NULL,                                // usart_port_cfg
            NULL,                                // com_driver
            NULL,                                // i2c_id
            NULL,                                // i2c_cfg
            &pios_ppm_cfg,                       // ppm_cfg
            NULL,                                // pwm_cfg
            PIOS_LED_ALARM,                      // led_id
            NULL,                                // dsm_cfg
            0,                                   // dsm_mode
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
#endif    /* PIOS_INCLUDE_GCSRCVR */

#if defined(PIOS_INCLUDE_SERVO) & defined(PIOS_INCLUDE_DMASHOT)
	PIOS_DMAShot_Init(&dmashot_config);
#endif

#if defined(PIOS_INCLUDE_TIM) && defined(PIOS_INCLUDE_SERVO)
#ifndef PIOS_DEBUG_ENABLE_DEBUG_PINS
    PIOS_Servo_Init(&pios_servo_cfg);
#else
    PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
#endif
#endif

#ifdef PIOS_INCLUDE_WS2811
	RGBLEDSettingsInitialize();

	uint8_t temp;

	RGBLEDSettingsNumLedsGet(&temp);

	if (temp > 0) {
		PIOS_WS2811_init(&pios_ws2811, &pios_ws2811_cfg, temp);

		// Drive default value (off) to strand once at startup
		PIOS_WS2811_trigger_update(pios_ws2811);
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
	if (PIOS_MPU_SPI_Init(&mpu_dev, pios_spi_internal_id, 0, &pios_mpu_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

	HwAQ32GyroRangeOptions hw_gyro_range;
	HwAQ32GyroRangeGet(&hw_gyro_range);
	switch(hw_gyro_range) {
		case HWAQ32_GYRORANGE_250:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_250_DEG);
			break;
		case HWAQ32_GYRORANGE_500:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_500_DEG);
			break;
		case HWAQ32_GYRORANGE_1000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_1000_DEG);
			break;
		case HWAQ32_GYRORANGE_2000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_2000_DEG);
			break;
	}

	HwAQ32AccelRangeOptions hw_accel_range;
	HwAQ32AccelRangeGet(&hw_accel_range);
	switch(hw_accel_range) {
		case HWAQ32_ACCELRANGE_2G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_2G);
			break;
		case HWAQ32_ACCELRANGE_4G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_4G);
			break;
		case HWAQ32_ACCELRANGE_8G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_8G);
			break;
		case HWAQ32_ACCELRANGE_16G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_16G);
			break;
	}

	// the filter has to be set before rate else divisor calculation will fail
	HwAQ32MPU6000DLPFOptions hw_mpu_dlpf;
	HwAQ32MPU6000DLPFGet(&hw_mpu_dlpf);
	uint16_t bandwidth = \
		(hw_mpu_dlpf == HWAQ32_MPU6000DLPF_188) ? 188 : \
		(hw_mpu_dlpf == HWAQ32_MPU6000DLPF_98)  ? 98  : \
		(hw_mpu_dlpf == HWAQ32_MPU6000DLPF_42)  ? 42  : \
		(hw_mpu_dlpf == HWAQ32_MPU6000DLPF_20)  ? 20  : \
		(hw_mpu_dlpf == HWAQ32_MPU6000DLPF_10)  ? 10  : \
		(hw_mpu_dlpf == HWAQ32_MPU6000DLPF_5)   ? 5   : \
		188;
	PIOS_MPU_SetGyroBandwidth(bandwidth);
#endif

#if defined(PIOS_INCLUDE_I2C)
	PIOS_WDG_Clear();

	uint8_t hw_magnetometer;
	HwAQ32MagnetometerGet(&hw_magnetometer);

	switch (hw_magnetometer) {
		case HWAQ32_MAGNETOMETER_NONE:
			break;

		case HWAQ32_MAGNETOMETER_INTERNAL:
#if defined(PIOS_INCLUDE_HMC5883)
			if ((PIOS_HMC5883_Init(pios_i2c_internal_id, &pios_hmc5883_internal_cfg) != 0) ||
					(PIOS_HMC5883_Test() != 0)) {
				PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_MAG);
			}
#endif /* PIOS_INCLUDE_HMC5883 */
			break;

		/* default external mags and handle them in PiOS HAL rather than maintaining list here */
		default:
			PIOS_HAL_ConfigurePort(HWSHARED_PORTTYPES_I2C,  // port type protocol
				NULL,                                   // usart_port_cfg
				NULL,                                   // com_driver
				&pios_i2c_external_id,                  // i2c_id
				&pios_i2c_external_cfg,                 // i2c_cfg
				NULL,                                   // ppm_cfg
				NULL,                                   // pwm_cfg
				PIOS_LED_ALARM,                         // led_id
				NULL,                                   // dsm_cfg
				0,                                      // dsm_mode
				NULL);                                  // sbus_cfg

			uint8_t hw_orientation;
			HwAQ32ExtMagOrientationGet(&hw_orientation);

			PIOS_HAL_ConfigureExternalMag(hw_magnetometer, hw_orientation,
				&pios_i2c_external_id, &pios_i2c_external_cfg);

			break;
	}


	/* I2C is slow, sensor init as well, reset watchdog to prevent reset here */
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MS5611)
    if ((PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_internal_id) != 0)
		    || (PIOS_MS5611_Test() != 0))
        PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_BARO);
#endif

    //I2C is slow, sensor init as well, reset watchdog to prevent reset here
    PIOS_WDG_Clear();

#endif    /* PIOS_INCLUDE_I2C */

#if defined(PIOS_INCLUDE_ADC)
    /* Configure the adc port(s) */
    uint8_t hw_adcport;

    HwAQ32ADCInputsGet(&hw_adcport);

    switch (hw_adcport)    {
    case HWAQ32_ADCINPUTS_DISABLED:
        break;
    case HWAQ32_ADCINPUTS_ENABLED:
        {
            uint32_t internal_adc_id;

            if (PIOS_INTERNAL_ADC_Init(&internal_adc_id, &pios_adc_cfg) < 0) {
                PIOS_Assert(0);
                    PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_ADC);
            }

            if (PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id) < 0)
                PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_ADC);
        }
        break;
    }
#endif

    /* Make sure we have at least one telemetry link configured or else fail initialization */
    PIOS_Assert(pios_com_telem_serial_id || pios_com_telem_usb_id);
}

/**
 * @}
 * @}
 */
