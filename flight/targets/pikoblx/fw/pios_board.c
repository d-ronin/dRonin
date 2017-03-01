/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup PikoBLX Tau Labs PikoBLX support files
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
#include "hwpikoblx.h"
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
 * Configuration for the MPU6050 chip
 */
#if defined(PIOS_INCLUDE_MPU6050)
#include "pios_mpu6050.h"
static const struct pios_exti_cfg pios_exti_mpu6050_cfg __exti_config = {
	.vector = PIOS_MPU6050_IRQHandler,
	.line = EXTI_Line15,
	.pin = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin = GPIO_Pin_15,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd = GPIO_PuPd_NOPULL,
		},
	},
	.irq = {
		.init = {
			.NVIC_IRQChannel = EXTI15_10_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.exti = {
		.init = {
			.EXTI_Line = EXTI_Line15, // matches above GPIO pin
			.EXTI_Mode = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Rising,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

static const struct pios_mpu60x0_cfg pios_mpu6050_cfg = {
	.exti_cfg = &pios_exti_mpu6050_cfg,
	.default_samplerate = 500,
	.interrupt_cfg = PIOS_MPU60X0_INT_CLR_ANYRD,
	.interrupt_en = PIOS_MPU60X0_INTEN_DATA_RDY,
	.User_ctl = 0,
	.Pwr_mgmt_clk = PIOS_MPU60X0_PWRMGMT_PLL_Z_CLK,
	.default_filter = PIOS_MPU60X0_LOWPASS_188_HZ,
	.orientation = PIOS_MPU60X0_TOP_180DEG
};
#endif /* PIOS_INCLUDE_MPU6050 */

/**
 * Configuration for the MPU9150 chip
 */
#if defined(PIOS_INCLUDE_MPU9150)
#include "pios_mpu9150.h"
static const struct pios_exti_cfg pios_exti_mpu9150_cfg __exti_config = {
	.vector = PIOS_MPU9150_IRQHandler,
	.line = EXTI_Line15,
	.pin = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin = GPIO_Pin_15,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd = GPIO_PuPd_NOPULL,
		},
	},
	.irq = {
		.init = {
			.NVIC_IRQChannel = EXTI15_10_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.exti = {
		.init = {
			.EXTI_Line = EXTI_Line15, // matches above GPIO pin
			.EXTI_Mode = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Rising,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

static const struct pios_mpu60x0_cfg pios_mpu9150_cfg = {
	.exti_cfg = &pios_exti_mpu9150_cfg,
	.default_samplerate = 500,
	.interrupt_cfg = PIOS_MPU60X0_INT_CLR_ANYRD,
	.interrupt_en = PIOS_MPU60X0_INTEN_DATA_RDY,
	.User_ctl = 0,
	.Pwr_mgmt_clk = PIOS_MPU60X0_PWRMGMT_PLL_Z_CLK,
	.default_filter = PIOS_MPU60X0_LOWPASS_188_HZ,
	.orientation = PIOS_MPU60X0_TOP_180DEG,
};
#endif /* PIOS_INCLUDE_MPU9150 */

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

#if defined(PIOS_INCLUDE_SPI)
	uint32_t pios_spi_gyro_id;

	/* Set up the SPI interface to the gyro/accelerometer */
	if (PIOS_SPI_Init(&pios_spi_gyro_id, &pios_spi_gyro_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
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
	HwPikoBLXInitialize();
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
		HwPikoBLXSetDefaults(HwPikoBLXHandle(), 0);
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
	HwPikoBLXUSB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWPIKOBLX_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwPikoBLXUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWPIKOBLX_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif	/* PIOS_INCLUDE_USB_HID */
#endif	/* PIOS_INCLUDE_USB */

	/* Configure the IO ports */
	
	HwPikoBLXDSMxModeOptions hw_DSMxMode;
	HwPikoBLXDSMxModeGet(&hw_DSMxMode);

	/* XXX add I2C support */
	HwSharedPortTypesOptions hw_uart1;
	HwPikoBLXUart1Get(&hw_uart1);
	PIOS_HAL_ConfigurePort(hw_uart1, // port_type
						&pios_uart1_usart_cfg, // usart_port_cfg
						&pios_usart_com_driver, // com_driver
						NULL, // i2c_id
						NULL, // i2c_cfg
						NULL, // ppm_cfg
						NULL, // pwm_cfg
						PIOS_LED_ALARM, // led_id
						&pios_uart1_dsm_cfg, // dsm_cfg
						hw_DSMxMode, // dsm_mode
						NULL); // sbus_cfg
	
	/* Configure Uart2 */
	HwSharedPortTypesOptions hw_uart2;
	HwPikoBLXUart2Get(&hw_uart2);
	PIOS_HAL_ConfigurePort(hw_uart2, // port_type
						&pios_uart2_usart_cfg, // usart_port_cfg
						&pios_usart_com_driver, // com_driver
						NULL, // i2c_id
						NULL, // i2c_cfg
						NULL, // ppm_cfg
						NULL, // pwm_cfg
						PIOS_LED_ALARM, // led_id
						&pios_uart2_dsm_cfg, // dsm_cfg
						hw_DSMxMode, // dsm_mode
						NULL); // sbus_cfg

	/* Configure Uart3 */
	HwSharedPortTypesOptions hw_uart3;
	HwPikoBLXUart3Get(&hw_uart3);
	PIOS_HAL_ConfigurePort(hw_uart3, // port_type
						&pios_uart3_usart_cfg, // usart_port_cfg
						&pios_usart_com_driver, // com_driver
						NULL, // i3c_id
						NULL, // i3c_cfg
						NULL, // ppm_cfg
						NULL, // pwm_cfg
						PIOS_LED_ALARM, // led_id
						&pios_uart3_dsm_cfg, // dsm_cfg
						hw_DSMxMode, // dsm_mode
						NULL); // sbus_cfg


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

#if defined(PIOS_INCLUDE_MPU)
	pios_mpu_dev_t mpu_dev = NULL;
	if (PIOS_MPU_SPI_Init(&mpu_dev, pios_spi_gyro_id, 0, &pios_mpu_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

	HwPikoBLXGyroRangeOptions hw_gyro_range;
	HwPikoBLXGyroRangeGet(&hw_gyro_range);
	switch(hw_gyro_range) {
		case HWPIKOBLX_GYRORANGE_250:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_250_DEG);
			break;
		case HWPIKOBLX_GYRORANGE_500:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_500_DEG);
			break;
		case HWPIKOBLX_GYRORANGE_1000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_1000_DEG);
			break;
		case HWPIKOBLX_GYRORANGE_2000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_2000_DEG);
			break;
	}

	HwPikoBLXAccelRangeOptions hw_accel_range;
	HwPikoBLXAccelRangeGet(&hw_accel_range);
	switch(hw_accel_range) {
		case HWPIKOBLX_ACCELRANGE_2G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_2G);
			break;
		case HWPIKOBLX_ACCELRANGE_4G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_4G);
			break;
		case HWPIKOBLX_ACCELRANGE_8G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_8G);
			break;
		case HWPIKOBLX_ACCELRANGE_16G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_16G);
			break;
	}

	// the filter has to be set before rate else divisor calculation will fail
	HwPikoBLXGyroLPFOptions hw_mpu_gyro_dlpf;
	HwPikoBLXGyroLPFGet(&hw_mpu_gyro_dlpf);
	uint16_t gyro_bandwidth =
		(hw_mpu_gyro_dlpf == HWPIKOBLX_GYROLPF_188) ? 188 :
		(hw_mpu_gyro_dlpf == HWPIKOBLX_GYROLPF_98)  ?  98 :
		(hw_mpu_gyro_dlpf == HWPIKOBLX_GYROLPF_42)  ?  42 :
		(hw_mpu_gyro_dlpf == HWPIKOBLX_GYROLPF_20)  ?  20 :
		(hw_mpu_gyro_dlpf == HWPIKOBLX_GYROLPF_10)  ?  10 :
		(hw_mpu_gyro_dlpf == HWPIKOBLX_GYROLPF_5)   ?   5 :
		184;
	PIOS_MPU_SetGyroBandwidth(gyro_bandwidth);

	HwPikoBLXAccelLPFOptions hw_mpu_accel_dlpf;
	HwPikoBLXAccelLPFGet(&hw_mpu_accel_dlpf);
	uint16_t acc_bandwidth = 
		(hw_mpu_accel_dlpf = HWPIKOBLX_ACCELLPF_218) ? 218 :
		(hw_mpu_accel_dlpf = HWPIKOBLX_ACCELLPF_99)  ?  99 :
		(hw_mpu_accel_dlpf = HWPIKOBLX_ACCELLPF_45)  ?  45 :
		(hw_mpu_accel_dlpf = HWPIKOBLX_ACCELLPF_21)  ?  21 :
		(hw_mpu_accel_dlpf = HWPIKOBLX_ACCELLPF_10)  ?  10 :
		(hw_mpu_accel_dlpf = HWPIKOBLX_ACCELLPF_5)   ?   5 :
		184;
	PIOS_MPU_SetAccelBandwidth(acc_bandwidth);
#endif /* PIOS_INCLUDE_MPU */

	/* I2C is slow, sensor init as well, reset watchdog to prevent reset here */
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
