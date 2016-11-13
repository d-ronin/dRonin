/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup lux Tau Labs lux support files
 * @{
 *
 * @file       pios_board.c
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
#include <pios_hal.h>
#include <openpilot.h>
#include <uavobjectsinit.h>
#include "hwlux.h"
#include "hwshared.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"

/**
 * Configuration for the MPU9250 chip
 */
#if defined(PIOS_INCLUDE_MPU9250_SPI)
#include "pios_mpu9250.h"
static const struct pios_exti_cfg pios_exti_mpu9250_cfg __exti_config = {
    .vector = PIOS_MPU9250_IRQHandler,
    .line = EXTI_Line5,
    .pin = {
        .gpio = GPIOA,
        .init = {
            .GPIO_Pin = GPIO_Pin_5,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_IN,
            .GPIO_OType = GPIO_OType_OD,
            .GPIO_PuPd = GPIO_PuPd_NOPULL,
        },
    },
    .irq = {
        .init = {
            .NVIC_IRQChannel = EXTI9_5_IRQn,
            .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
            .NVIC_IRQChannelSubPriority = 0,
            .NVIC_IRQChannelCmd = ENABLE,
        },
    },
    .exti = {
        .init = {
            .EXTI_Line = EXTI_Line5, // matches above GPIO pin
            .EXTI_Mode = EXTI_Mode_Interrupt,
            .EXTI_Trigger = EXTI_Trigger_Rising,
            .EXTI_LineCmd = ENABLE,
        },
    },
};

static struct pios_mpu9250_cfg pios_mpu9250_cfg = {
    .exti_cfg = &pios_exti_mpu9250_cfg,
    .default_samplerate = 1000,
    .interrupt_cfg = PIOS_MPU60X0_INT_CLR_ANYRD,

    .use_magnetometer = false,
    .default_gyro_filter = PIOS_MPU9250_GYRO_LOWPASS_184_HZ,
    .default_accel_filter = PIOS_MPU9250_ACCEL_LOWPASS_184_HZ,
    .orientation = PIOS_MPU9250_TOP_180DEG
};
#endif /* PIOS_INCLUDE_MPU9250_SPI */

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
#define PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN 40
uintptr_t pios_com_debug_id;
#endif	/* PIOS_INCLUDE_DEBUG_CONSOLE */

uintptr_t pios_com_aux_id;
uintptr_t pios_com_telem_rf_id;
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

#if defined(PIOS_INCLUDE_SPI)
	uint32_t pios_spi_gyro_id;

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
	HwLuxInitialize();
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
		HwLuxSetDefaults(HwLuxHandle(), 0);
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
	HwLuxUSB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWLUX_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwLuxUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWLUX_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif	/* PIOS_INCLUDE_USB_HID */
#endif	/* PIOS_INCLUDE_USB */

	/* Configure the IO ports */
	HwLuxDSMxModeOptions hw_DSMxMode;
	HwLuxDSMxModeGet(&hw_DSMxMode);

	uint8_t hw_uart3;
	HwLuxUart3Get(&hw_uart3);
	PIOS_HAL_ConfigurePort(hw_uart3,              // port_type
                         &pios_uart3_cfg,         // usart_port_cfg
                         &pios_usart_com_driver,  // com_driver
                         NULL,                    // i2c_id
                         NULL,                    // i2c_cfg
                         NULL,                    // ppm_cfg
                         NULL,                    // pwm_cfg
                         PIOS_LED_ALARM,          // led_id
                         &pios_uart3_dsm_aux_cfg, // dsm_cfg
                         hw_DSMxMode,             // dsm_mode
                         NULL);                   // sbus_cfg

	uint8_t hw_uart2;
	HwLuxUart2Get(&hw_uart2);
	PIOS_HAL_ConfigurePort(hw_uart2,              // port_type
                         &pios_uart2_cfg,         // usart_port_cfg
                         &pios_usart_com_driver,  // com_driver
                         NULL,                    // i2c_id
                         NULL,                    // i2c_cfg
                         NULL,                    // ppm_cfg
                         NULL,                    // pwm_cfg
                         PIOS_LED_ALARM,          // led_id
                         &pios_uart2_dsm_aux_cfg, // dsm_cfg
                         hw_DSMxMode,             // dsm_mode
                         NULL);                   // sbus_cfg

	/* Configure the rx port */
	uint8_t hw_rxport;
	HwLuxRxPortGet(&hw_rxport);
	PIOS_HAL_ConfigurePort(hw_rxport,             // port_type
                         &pios_rxport_usart_cfg,  // usart_port_cfg
                         &pios_usart_com_driver,  // com_driver
                         NULL,                    // i2c_id
                         NULL,                    // i2c_cfg
                         NULL,                    // ppm_cfg
                         NULL,                    // pwm_cfg
                         PIOS_LED_ALARM,          // led_id
                         &pios_rxport_dsm_aux_cfg,// dsm_cfg
                         hw_DSMxMode,             // dsm_mode
                         NULL);                   // sbus_cfg

    /* Always configure PPM */
    const uint8_t hw_rxppm = HWSHARED_PORTTYPES_PPM;
    PIOS_HAL_ConfigurePort(hw_rxppm,              // port_type
                         NULL,                    // usart_port_cfg
                         NULL,                    // com_driver
                         NULL,                    // i2c_id
                         NULL,                    // i2c_cfg
                         &pios_ppm_cfg,           // ppm_cfg
                         NULL,                    // pwm_cfg
                         PIOS_LED_ALARM,          // led_id
                         NULL,                    // dsm_cfg
                         0,                       // dsm_mode
                         NULL);                   // sbus_cfg

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

#if defined(PIOS_INCLUDE_MPU9250_SPI)
#if !defined(PIOS_INCLUDE_SPI)
    #error MPU9250_SPI requires SPI
#endif
    
    if (PIOS_MPU9250_SPI_Init(pios_spi_gyro_id, 0, &pios_mpu9250_cfg) != 0)
        PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

    // To be safe map from UAVO enum to driver enum
    uint8_t hw_gyro_range;
    HwLuxGyroRangeGet(&hw_gyro_range);
    switch(hw_gyro_range) {
        case HWLUX_GYRORANGE_250:
            PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_250_DEG);
            break;
        case HWLUX_GYRORANGE_500:
            PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_500_DEG);
            break;
        case HWLUX_GYRORANGE_1000:
            PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_1000_DEG);
            break;
        case HWLUX_GYRORANGE_2000:
            PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_2000_DEG);
            break;
    }

    uint8_t hw_accel_range;
    HwLuxAccelRangeGet(&hw_accel_range);
    switch(hw_accel_range) {
        case HWLUX_ACCELRANGE_2G:
            PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_2G);
            break;
        case HWLUX_ACCELRANGE_4G:
            PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_4G);
            break;
        case HWLUX_ACCELRANGE_8G:
            PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_8G);
            break;
        case HWLUX_ACCELRANGE_16G:
            PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_16G);
            break;
    }

    // the filter has to be set before rate else divisor calculation will fail
    uint8_t hw_mpu9250_dlpf;
    HwLuxMPU9250GyroLPFGet(&hw_mpu9250_dlpf);
    enum pios_mpu9250_gyro_filter mpu9250_gyro_lpf = \
        (hw_mpu9250_dlpf == HWLUX_MPU9250GYROLPF_184) ? PIOS_MPU9250_GYRO_LOWPASS_184_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250GYROLPF_92) ? PIOS_MPU9250_GYRO_LOWPASS_92_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250GYROLPF_41) ? PIOS_MPU9250_GYRO_LOWPASS_41_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250GYROLPF_20) ? PIOS_MPU9250_GYRO_LOWPASS_20_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250GYROLPF_10) ? PIOS_MPU9250_GYRO_LOWPASS_10_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250GYROLPF_5) ? PIOS_MPU9250_GYRO_LOWPASS_5_HZ : \
        pios_mpu9250_cfg.default_gyro_filter;
    PIOS_MPU9250_SetGyroLPF(mpu9250_gyro_lpf);

    HwLuxMPU9250AccelLPFGet(&hw_mpu9250_dlpf);
    enum pios_mpu9250_accel_filter mpu9250_accel_lpf = \
        (hw_mpu9250_dlpf == HWLUX_MPU9250ACCELLPF_460) ? PIOS_MPU9250_ACCEL_LOWPASS_460_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250ACCELLPF_184) ? PIOS_MPU9250_ACCEL_LOWPASS_184_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250ACCELLPF_92) ? PIOS_MPU9250_ACCEL_LOWPASS_92_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250ACCELLPF_41) ? PIOS_MPU9250_ACCEL_LOWPASS_41_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250ACCELLPF_20) ? PIOS_MPU9250_ACCEL_LOWPASS_20_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250ACCELLPF_10) ? PIOS_MPU9250_ACCEL_LOWPASS_10_HZ : \
        (hw_mpu9250_dlpf == HWLUX_MPU9250ACCELLPF_5) ? PIOS_MPU9250_ACCEL_LOWPASS_5_HZ : \
        pios_mpu9250_cfg.default_accel_filter;
    PIOS_MPU9250_SetAccelLPF(mpu9250_accel_lpf);

    uint8_t hw_mpu9250_samplerate;
    HwLuxMPU9250RateGet(&hw_mpu9250_samplerate);
    uint16_t mpu9250_samplerate = \
        (hw_mpu9250_samplerate == HWLUX_MPU9250RATE_200) ? 200 : \
        (hw_mpu9250_samplerate == HWLUX_MPU9250RATE_250) ? 250 : \
        (hw_mpu9250_samplerate == HWLUX_MPU9250RATE_333) ? 333 : \
        (hw_mpu9250_samplerate == HWLUX_MPU9250RATE_500) ? 500 : \
        (hw_mpu9250_samplerate == HWLUX_MPU9250RATE_1000) ? 1000 :
        pios_mpu9250_cfg.default_samplerate;
    PIOS_MPU9250_SetSampleRate(mpu9250_samplerate);
#endif /* PIOS_INCLUDE_MPU9250_SPI */

	//I2C is slow, sensor init as well, reset watchdog to prevent reset here
	PIOS_WDG_Clear();

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
 */
