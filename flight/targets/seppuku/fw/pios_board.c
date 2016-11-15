/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Seppuku Seppuku support files
 * @{
 *
 * @file       pios_board.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
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
#include "hwseppuku.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"
#include "onscreendisplaysettings.h"

 
/**
 * Sensor configurations
 */
#if defined(PIOS_INCLUDE_BMI160)
#include "pios_bmi160.h"

static const struct pios_exti_cfg pios_exti_bmi160_cfg __exti_config = {
	.vector = PIOS_BMI160_IRQHandler,
	.line = EXTI_Line4,
	.pin = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin = GPIO_Pin_4,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd = GPIO_PuPd_NOPULL,
		},
	},
	.irq = {
		.init = {
			.NVIC_IRQChannel = EXTI4_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.exti = {
		.init = {
			.EXTI_Line = EXTI_Line4, // matches above GPIO pin
			.EXTI_Mode = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Rising,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

static const struct pios_bmi160_cfg pios_bmi160_cfg = {
	.exti_cfg = &pios_exti_bmi160_cfg,
	.orientation = PIOS_BMI160_TOP_0DEG,
	.odr = PIOS_BMI160_ODR_1600_Hz,
	.acc_range = PIOS_BMI160_RANGE_8G,
	.gyro_range = PIOS_BMI160_RANGE_2000DPS,
	.temperature_interleaving = 50
};
#endif /* PIOS_INCLUDE_BMI160 */
#if defined(PIOS_INCLUDE_WS2811)
#include "rgbledsettings.h"
#endif

uintptr_t pios_com_openlog_logging_id;
uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_internal_adc_id;

uintptr_t external_i2c_adapter_id = 0;

/**
* Initialise PWM Output for black/white level setting
*/

#if defined(PIOS_INCLUDE_VIDEO)
void OSD_configure_bw_levels(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	/*-------------------------- GPIO Configuration ----------------------------*/
	GPIO_StructInit(&GPIO_InitStructure); // Reset init structure
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_TIM11);  // White
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM12); // Black

	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = 0;  // Run as fast as we can.
	TIM_TimeBaseStructure.TIM_Period = 255;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM11, &TIM_TimeBaseStructure);
	TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);

	/* PWM mode configuation */
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 90;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC1Init(TIM11, &TIM_OCInitStructure);
	TIM_OC2Init(TIM12, &TIM_OCInitStructure);

	/* TIM11/12 Main Output Enable */
	TIM_CtrlPWMOutputs(TIM11, ENABLE);
	TIM_CtrlPWMOutputs(TIM12, ENABLE);

	/* TIM1 enable counter */
	TIM_Cmd(TIM11, ENABLE);
	TIM_Cmd(TIM12, ENABLE);

	TIM11->CCR1 = 110;
	TIM12->CCR2 = 30;
}
#endif /* PIOS_INCLUDE_VIDEO */

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
#endif	/* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_SPI)
	if (PIOS_SPI_Init(&pios_spi_gyro_accel_id, &pios_spi_gyro_accel_cfg)) {
		PIOS_Assert(0);
	}
#endif


#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
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
#endif	/* PIOS_INCLUDE_FLASH */

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Initialize UAVObject libraries */
	UAVObjInitialize();

	/* Initialize the alarms library. Reads RCC reset flags */
	AlarmsInitialize();
	PIOS_RESET_Clear(); // Clear the RCC reset flags after use.

	/* Initialize the hardware UAVOs */
	HwSeppukuInitialize();
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

	//Timer used for inputs (9)
	PIOS_TIM_InitClock(&tim_8_cfg);

	// Timers used for outputs (8, 14, 3, 5)
	PIOS_TIM_InitClock(&tim_8_cfg);
	PIOS_TIM_InitClock(&tim_14_cfg);
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_5_cfg);

	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwSeppukuSetDefaults(HwSeppukuHandle(), 0);
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
	HwSeppukuUSB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWSEPPUKU_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);
	
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwSeppukuUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWSEPPUKU_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif	/* PIOS_INCLUDE_USB_HID */

	if (usb_hid_present || usb_cdc_present) {
		PIOS_USBHOOK_Activate();
	}
#endif	/* PIOS_INCLUDE_USB */

	/* Configure the IO ports */
	HwSeppukuDSMxModeOptions hw_DSMxMode;
	HwSeppukuDSMxModeGet(&hw_DSMxMode);

	/* Configure the receiver port*/
	uint8_t hw_rcvrport;
	HwSeppukuRcvrPortGet(&hw_rcvrport);

	PIOS_HAL_ConfigurePort(hw_rcvrport,            // port type protocol
			&pios_usart2_cfg,                      // usart_port_cfg
			&pios_usart_com_driver,                // com_driver
			NULL,                                  // i2c_id
			NULL,                                  // i2c_cfg
			&pios_ppm_cfg,                         // ppm_cfg
			NULL,                                  // pwm_cfg
			PIOS_LED_ALARM,                        // led_id
			&pios_usart2_dsm_aux_cfg,              // dsm_cfg
			hw_DSMxMode,                           // dsm_mode
			&pios_usart2_sbus_aux_cfg);            // sbus_cfg

	/* UART1 Port */
	uint8_t hw_uart1;
	HwSeppukuUart1Get(&hw_uart1);

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

	/* UART3 Port */
	uint8_t hw_uart3;
	HwSeppukuUart3Get(&hw_uart3);

	PIOS_HAL_ConfigurePort(hw_uart3,             // port type protocol
			&pios_usart3_cfg,                    // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			&pios_i2c_usart3_adapter_id,         // i2c_id
			&pios_i2c_usart3_adapter_cfg,        // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			&pios_usart3_dsm_aux_cfg,            // dsm_cfg
			hw_DSMxMode,                         // dsm_mode
			NULL);                               // sbus_cfg

	uint8_t hw_outputconf;
	HwSeppukuOutputConfigurationGet(&hw_outputconf);

	switch (hw_outputconf) {
		case HWSEPPUKU_OUTPUTCONFIGURATION_SIXCHANNELSUART4:
			(void) 0;
			uint8_t hw_uart4;
			HwSeppukuUart4Get(&hw_uart4);

			PIOS_HAL_ConfigurePort(hw_uart4,             // port type protocol
					&pios_uart4_cfg,                    // usart_port_cfg
					&pios_usart_com_driver,              // com_driver
					NULL,                                // i2c_id
					NULL,                                // i2c_cfg
					NULL,                                // ppm_cfg
					NULL,                                // pwm_cfg
					PIOS_LED_ALARM,                      // led_id
					&pios_uart4_dsm_aux_cfg,            // dsm_cfg
					hw_DSMxMode,                         // dsm_mode
					NULL);                               // sbus_cfg

#ifdef PIOS_INCLUDE_SERVO
			PIOS_Servo_Init(&pios_servo_cfg);
#endif
			break;
		default:
#ifdef PIOS_INCLUDE_SERVO
			PIOS_Servo_Init(&pios_servo_cfg);
#endif
			break;
	}

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

	if (num_leds < 1) {
		num_leds = 1;	// Always enable the on-board LED.
	}

	PIOS_WS2811_init(&pios_ws2811, &pios_ws2811_cfg, num_leds);
	PIOS_WS2811_trigger_update(pios_ws2811);
#endif

/* init sensor queue registration */
	PIOS_SENSORS_Init();

	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_SPI)

#if defined(PIOS_INCLUDE_BMI160)
	uint8_t bmi160_foc;
	HwSeppukuBMI160FOCGet(&bmi160_foc);

	bool do_foc = (bmi160_foc == HWSEPPUKU_BMI160FOC_DO_FOC);

	if(PIOS_BMI160_Init(pios_spi_gyro_accel_id, 0, &pios_bmi160_cfg, do_foc) != 0){
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);
	}

	/* Disable FOC. We only do this once. */
	if (do_foc) {
		bmi160_foc = HWSEPPUKU_BMI160FOC_DISABLED;
		HwSeppukuBMI160FOCSet(&bmi160_foc);
		UAVObjSave(HwSeppukuHandle(), 0);
	}
#endif /* PIOS_INCLUDE_BMI160 */

	// XXX BMP280 support
#endif /* PIOS_INCLUDE_SPI */

	PIOS_WDG_Clear();

	uint8_t hw_magnetometer;
	HwSeppukuMagnetometerGet(&hw_magnetometer);

	switch (hw_magnetometer) {
		case HWSEPPUKU_MAGNETOMETER_NONE:
			break;

		case HWSEPPUKU_MAGNETOMETER_INTERNAL:
#if defined(PIOS_INCLUDE_SPI)
			/* XXX magnetometer support */
#endif /* PIOS_INCLUDE_SPI */
			break;

		/* default external mags and handle them in PiOS HAL rather than maintaining list here */
		default:
#if defined(PIOS_INCLUDE_I2C)
		{
			if (pios_i2c_usart3_adapter_id) {
				uint8_t hw_orientation;
				HwSeppukuExtMagOrientationGet(&hw_orientation);

				PIOS_HAL_ConfigureExternalMag(hw_magnetometer, hw_orientation,
					&pios_i2c_usart3_adapter_id,
					&pios_i2c_usart3_adapter_cfg);
			} else {
				PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
			}
		}
#else
			PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
#endif // PIOS_INCLUDE_I2C

			break;
	}

	/* I2C is slow, sensor init as well, reset watchdog to prevent reset here */
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_GPIO)
	PIOS_GPIO_Init();
#endif

#if defined(PIOS_INCLUDE_ADC)
	uint32_t internal_adc_id;
	PIOS_INTERNAL_ADC_Init(&internal_adc_id, &pios_adc_cfg);
	PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id);
#endif

#if defined(PIOS_INCLUDE_VIDEO)
	// make sure the mask pin is low
	GPIO_Init(pios_video_cfg.mask.miso.gpio, (GPIO_InitTypeDef*)&pios_video_cfg.mask.miso.init);
	GPIO_ResetBits(pios_video_cfg.mask.miso.gpio, pios_video_cfg.mask.miso.init.GPIO_Pin);

	// Initialize settings
	OnScreenDisplaySettingsInitialize();

	uint8_t osd_state;
	OnScreenDisplaySettingsOSDEnabledGet(&osd_state);
	if (osd_state == ONSCREENDISPLAYSETTINGS_OSDENABLED_ENABLED) {
		OSD_configure_bw_levels();
	}
#endif

	/* Make sure we have at least one telemetry link configured or else fail initialization */
	PIOS_Assert(pios_com_telem_serial_id || pios_com_telem_usb_id);
}

/**
 * @}
 */
