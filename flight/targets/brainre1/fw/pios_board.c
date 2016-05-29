/**
 ******************************************************************************
 * @addtogroup dRonin Targets
 * @{
 * @addtogroup BrainRE1 support files
 * @{
 *
 * @file       pios_board.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
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
#include <misc_math.h>
#include <uavobjectsinit.h>
#include "hwbrainre1.h"
#include "flightstatus.h"
#include "modulesettings.h"
#include "manualcontrolsettings.h"
#include "onscreendisplaysettings.h"

#include "pios_ir_transponder.h"

#if defined(PIOS_INCLUDE_BMI160)
#include "pios_bmi160.h"

static const struct pios_exti_cfg pios_exti_bmi160_cfg __exti_config = {
	.vector = PIOS_BMI160_IRQHandler,
	.line = EXTI_Line13,
	.pin = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin = GPIO_Pin_13,
			.GPIO_Speed = GPIO_Speed_2MHz,  // XXXX
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
			.EXTI_Line = EXTI_Line13, // matches above GPIO pin
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

#if defined(PIOS_INCLUDE_FRSKY_RSSI)
#include "pios_frsky_rssi_priv.h"
#endif /* PIOS_INCLUDE_FRSKY_RSSI */

bool external_mag_fail;

uintptr_t pios_com_logging_id;
uintptr_t pios_com_openlog_logging_id;
uintptr_t pios_internal_adc_id = 0;
uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_waypoints_settings_fs_id;


const uint8_t named_color_value[HWBRAINRE1_LEDCOLOR_GLOBAL_MAXOPTVAL + 1][3] = {
	[HWBRAINRE1_LEDCOLOR_OFF]    = {0,   0,   0},
	[HWBRAINRE1_LEDCOLOR_CUSTOM] = {0,   0,   0},
	[HWBRAINRE1_LEDCOLOR_WHITE]  = {255, 255, 255},
	[HWBRAINRE1_LEDCOLOR_RED]    = {255, 0,   0},
	[HWBRAINRE1_LEDCOLOR_ORANGE] = {255, 69,   0},
	[HWBRAINRE1_LEDCOLOR_YELLOW] = {255, 255, 0},
	[HWBRAINRE1_LEDCOLOR_GREEN]  = {0,   255, 0},
	[HWBRAINRE1_LEDCOLOR_AQUA]   = {0,   255, 255},
	[HWBRAINRE1_LEDCOLOR_BLUE]   = {0,   0,   255},
	[HWBRAINRE1_LEDCOLOR_PURPLE] = {255, 0,   255},
};


/**
 * settingdUpdatedCb()
 * Called when RE1 hardware settings are updated
 */
static void settingdUpdatedCb(UAVObjEvent * ev, void *ctx, void *obj, int len)
{
	(void) ev; (void) ctx; (void) len;

	HwBrainRE1Data * hwre1data = (HwBrainRE1Data *)obj;

	/* Set the colors of the RGB LEDs */
	if (hwre1data->NumberOfLEDs > 0) {
		uint8_t led_color[3];
		memset(led_color, 0, 3);
		switch (hwre1data->LEDColor) {
			case HWBRAINRE1_LEDCOLOR_CUSTOM:
				led_color[0] = hwre1data->CustomLEDColor[0];
				led_color[1] = hwre1data->CustomLEDColor[1];
				led_color[2] = hwre1data->CustomLEDColor[2];
				break;
			default:
				led_color[0] = named_color_value[hwre1data->LEDColor][0];
				led_color[1] = named_color_value[hwre1data->LEDColor][1];
				led_color[2] = named_color_value[hwre1data->LEDColor][2];
		}
		PIOS_RE1FPGA_SetLEDColor(hwre1data->NumberOfLEDs, led_color[0],
			led_color[1], led_color[2]);
	}

	/* Configure the IR emitter for lap timing */
	uint8_t ir_data[6];

	switch (hwre1data->IRProtocol) {
		case HWBRAINRE1_IRPROTOCOL_DISABLED:
			PIOS_RE1FPGA_SetIRProtocol(PIOS_RE1FPGA_IR_PROTOCOL_OFF);
			break;
		case HWBRAINRE1_IRPROTOCOL_ILAP:
			pios_ir_generate_ilap_packet(hwre1data->IRIDILap, ir_data, 6);
			PIOS_RE1FPGA_SetIRData(ir_data, 6);
			PIOS_RE1FPGA_SetIRProtocol(PIOS_RE1FPGA_IR_PROTOCOL_ILAP);
			break;
		case HWBRAINRE1_IRPROTOCOL_TRACKMATE:
			pios_ir_generate_trackmate_packet(hwre1data->IRIDTrackmate, ir_data, 4);
			PIOS_RE1FPGA_SetIRData(ir_data, 4);
			PIOS_RE1FPGA_SetIRProtocol(PIOS_RE1FPGA_IR_PROTOCOL_TRACKMATE);
			break;
	}

	/* Configure the failsafe buzzer */
	if (hwre1data->FailsafeBuzzer == HWBRAINRE1_FAILSAFEBUZZER_4KHZ_BUZZER) {
		PIOS_RE1FPGA_SetBuzzerType(PIOS_RE1FPGA_BUZZER_AC);
	}
	else {
		PIOS_RE1FPGA_SetBuzzerType(PIOS_RE1FPGA_BUZZER_DC);
	}

	/* Set video sync detector threshold */
	PIOS_RE1FPGA_SetSyncThreshold(hwre1data->VideoSyncDetectorThreshold);
}

/**
 * flightStatusUpdatedCb()
 * Used to toggle buzzer
 */
static void flightStatusUpdatedCb(UAVObjEvent * ev, void *ctx, void *obj, int len)
{
	(void) ev; (void) ctx; (void) len;
	static bool was_armed = false;

	FlightStatusData * data = (FlightStatusData *)obj;

	if (data->Armed == FLIGHTSTATUS_ARMED_ARMED) {
		was_armed = true;
	}

	if (was_armed && (data->FlightMode == FLIGHTSTATUS_FLIGHTMODE_FAILSAFE)) {
		uint8_t buzzer_setting;
		HwBrainRE1FailsafeBuzzerGet(&buzzer_setting);
		PIOS_RE1FPGA_Buzzer(buzzer_setting != HWBRAINRE1_FAILSAFEBUZZER_DISABLED);
	}
	else {
		PIOS_RE1FPGA_Buzzer(false);
	}

}

#include <pios_board_info.h>

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */
void PIOS_Board_Init(void) {
	/* Delay system */
	PIOS_DELAY_Init();

#if defined(PIOS_INCLUDE_LED)
	PIOS_LED_Init(&pios_led_cfg);
#endif	/* PIOS_INCLUDE_LED */

#if defined(PIOS_INCLUDE_SPI)
	if (PIOS_SPI_Init(&pios_spi_gyro_id, &pios_spi_gyro_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
	if (PIOS_SPI_Init(&pios_spi_flash_id, &pios_spi_flash_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
#endif

#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_external_id, &pios_i2c_external_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
	if (PIOS_I2C_CheckClear(pios_i2c_external_id) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_I2C_EXT);
#endif /* defined(PIOS_INCLUDE_I2C) */

#if defined(PIOS_INCLUDE_RE1_FPGA)
	if (PIOS_RE1FPGA_Init(pios_spi_flash_id, 1, &pios_re1fpga_cfg, false) != 0) {
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_CAN); // XXX need to add custom code
	}
#endif /* defined(PIOS_INCLUDE_RE1_FPGA) */

#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
	if (PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_FLASH);
	if (PIOS_Flash_Jedec_Init(&pios_external_flash_id, pios_spi_flash_id, 0, &flash_s25fl127_cfg) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_FLASH);

	/* Register the partition table */
	PIOS_FLASH_register_partition_table(pios_flash_partition_table, NELEMENTS(pios_flash_partition_table));

	/* Mount all filesystems */
	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_settings_cfg, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_FILESYS);
	if (PIOS_FLASHFS_Logfs_Init(&pios_waypoints_settings_fs_id, &flashfs_waypoints_cfg, FLASH_PARTITION_LABEL_WAYPOINTS) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_FILESYS);

#if defined(ERASE_FLASH)
	PIOS_FLASHFS_Format(pios_uavo_settings_fs_id);
#endif

#endif	/* PIOS_INCLUDE_FLASH */

	RCC_ClearFlag(); // The flags cleared after use

	/* Initialize UAVObject libraries */
	UAVObjInitialize();

	HwBrainRE1Initialize();

	/* Connect callback to update FPGA settings when UAVO changes */
	HwBrainRE1ConnectCallback(settingdUpdatedCb);

	/* Set the HW revision, this also invokes the callback */
	uint8_t hw_rev = PIOS_RE1FPGA_GetHWRevision();
	HwBrainRE1HWRevisionSet(&hw_rev);

	/* Connect callback for buzzer */
	FlightStatusInitialize();
	FlightStatusConnectCallback(flightStatusUpdatedCb);

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

	/* Initialize the alarms library */
	AlarmsInitialize();

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Set up pulse timers */
	//inputs
	PIOS_TIM_InitClock(&tim_12_cfg);
	//outputs
	PIOS_TIM_InitClock(&tim_1_cfg);
	PIOS_TIM_InitClock(&tim_5_cfg);
	PIOS_TIM_InitClock(&tim_8_cfg);

	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwBrainRE1SetDefaults(HwBrainRE1Handle(), 0);
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
	PIOS_USB_Init(&pios_usb_id, &pios_usb_main_cfg);

#if defined(PIOS_INCLUDE_USB_CDC)

	uint8_t hw_usb_vcpport;
	/* Configure the USB VCP port */
	HwBrainRE1USB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWBRAINRE1_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwBrainRE1USB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWBRAINRE1_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);
#endif	/* PIOS_INCLUDE_USB_HID */

	if (usb_hid_present || usb_cdc_present) {
		PIOS_USBHOOK_Activate();
	}
#endif	/* PIOS_INCLUDE_USB */

	/* Configure the RxPort*/

	/* Configure IO ports */
	HwBrainRE1DSMxModeOptions hw_DSMxMode;
	HwBrainRE1DSMxModeGet(&hw_DSMxMode);

	uint8_t hw_rxport;
	HwBrainRE1RxPortGet(&hw_rxport);

	PIOS_HAL_ConfigurePort(hw_rxport, // port type protocol
			&pios_usart3_cfg, // usart_port_cfg
			&pios_usart_com_driver,
			NULL, // i2c_id
			NULL, // i2c_cfg
			&pios_ppm_cfg, // ppm_cfg
			NULL, // pwm_cfg
			PIOS_LED_ALARM, // led_id
			&pios_rxport_dsm_cfg, // dsm_cfg
			hw_DSMxMode, // dsm_mode
			NULL);

	if (hw_rxport == HWSHARED_PORTTYPES_SBUS) {
		PIOS_RE1FPGA_SerialRxInvert(true);
	}

	/* SerialPort1 */
	uint8_t hw_sp;
	HwBrainRE1SerialPortGet(&hw_sp);

	PIOS_HAL_ConfigurePort(hw_sp,                // port type protocol
			&pios_usart1_cfg,                    // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			NULL,                                // i2c_id
			NULL,                                // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			NULL,                                // dsm_cfg
			0,                                   // dsm_mode
			NULL);                               // sbus_rcvr_cfg

	/* Multi-function port (pwm, serial, etc) */
	uint8_t hw_mp;
	HwBrainRE1MultiPortGet(&hw_mp);

	switch (hw_mp) {
		case HWBRAINRE1_MULTIPORT_PWM:
			// for us, this means PWM output, not input
			break;
		default:
			PIOS_HAL_ConfigurePort(hw_mp,                // port type protocol
					&pios_usart6_cfg,                    // usart_port_cfg
					&pios_usart_com_driver,              // com_driver
					NULL,                                // i2c_id
					NULL,                                // i2c_cfg
					NULL,                                // ppm_cfg
					NULL,                                // pwm_cfg
					PIOS_LED_ALARM,                      // led_id
					NULL,                                // dsm_cfg
					0,                                   // dsm_mode
					NULL);                               // sbus_rcvr_cfg
			break;
	}

	// Enable / disable RE1 specific hardware inverters and FrSky RSSI support
	switch (hw_mp) {
		case HWBRAINRE1_MULTIPORT_FRSKYSENSORHUB:
			PIOS_RE1FPGA_MPTxPinMode(false, true);
#if defined(PIOS_INCLUDE_FRSKY_RSSI)
			PIOS_FrSkyRssi_Init(&pios_frsky_rssi_cfg);
#endif /* PIOS_INCLUDE_FRSKY_RSSI */
			break;
		case HWBRAINRE1_MULTIPORT_FRSKYSPORTTELEMETRY:
			PIOS_RE1FPGA_MPTxPinMode(true, true);
			PIOS_RE1FPGA_MPTxPinPullUpDown(true, false);
#if defined(PIOS_INCLUDE_FRSKY_RSSI)
			PIOS_FrSkyRssi_Init(&pios_frsky_rssi_cfg);
#endif /* PIOS_INCLUDE_FRSKY_RSSI */
			break;
	}

	/* Configure PWM Outputs */
#if defined(PIOS_INCLUDE_SERVO) && defined(PIOS_INCLUDE_TIM)
	if (hw_mp == HWBRAINRE1_MULTIPORT_PWM) {
		// all 8 PWM outputs are used
		PIOS_Servo_Init(&pios_servo_all_cfg);
	}
	else {
		// only 6 PWM outputs are used
		PIOS_Servo_Init(&pios_servo_cfg);
	}
#endif /* defined(PIOS_INCLUDE_SERVO) && defined(PIOS_INCLUDE_TIM) */

	PIOS_WDG_Clear();
	PIOS_DELAY_WaitmS(50);

	PIOS_SENSORS_Init();

#if defined(PIOS_INCLUDE_ADC)
	uint32_t internal_adc_id;
	PIOS_INTERNAL_ADC_Init(&internal_adc_id, &pios_adc_cfg);
	PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id);
#endif /* defined(PIOS_INCLUDE_ADC) */

#if defined(PIOS_INCLUDE_SPI)
#if defined(PIOS_INCLUDE_BMI160)
	if(PIOS_BMI160_Init(pios_spi_gyro_id, 0, &pios_bmi160_cfg) != 0){
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);
	}
#endif /* PIOS_INCLUDE_BMI160 */
#endif /* PIOS_INCLUDE_SPI */
}

/**
 * @}
 */
