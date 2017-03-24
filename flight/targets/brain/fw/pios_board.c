/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup FlyingF4 FlyingF4 support files
 * @{
 *
 * @file       pios_board.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
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
#include "hwbrain.h"
#include "modulesettings.h"
#include "manualcontrolsettings.h"
#include "onscreendisplaysettings.h"


#if defined(PIOS_INCLUDE_FRSKY_RSSI)
#include "pios_frsky_rssi_priv.h"
#endif /* PIOS_INCLUDE_FRSKY_RSSI */

uintptr_t pios_internal_adc_id;
uintptr_t pios_com_logging_id;
uintptr_t pios_com_openlog_logging_id;

uintptr_t pios_uavo_settings_fs_id;


/**
 * @brief   Early initialization code.
 * @details This initialization must be performed just after stack setup
 *          and before any other initialization.
 */
void __early_init(void)
{
	stm32_clock_init();
}

/**
 * @brief   Board-specific RTOS initialization code.
 */
void boardInit(void)
{
}


/**
* Initialise PWM Output for black/white level setting
*/

#if defined(PIOS_INCLUDE_VIDEO)
void OSD_configure_bw_levels(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	/* --------------------------- System Clocks Configuration -----------------*/
	/* TIM1 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Connect TIM1 pins to AF */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);

	/*-------------------------- GPIO Configuration ----------------------------*/
	GPIO_StructInit(&GPIO_InitStructure); // Reset init structure
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = (PIOS_SYSCLK / 25500000) - 1; // Get clock to 25 MHz on STM32F2/F4
	TIM_TimeBaseStructure.TIM_Period = 255;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	/* Enable TIM1 Preload register on ARR */
	TIM_ARRPreloadConfig(TIM1, ENABLE);

	/* TIM PWM1 Mode configuration */
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 90;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	/* Output Compare PWM1 Mode configuration: Channel1 PA.08 */
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

	/* TIM1 Main Output Enable */
	TIM_CtrlPWMOutputs(TIM1, ENABLE);

	/* TIM1 enable counter */
	TIM_Cmd(TIM1, ENABLE);
	TIM1->CCR1 = 30;
	TIM1->CCR3 = 110;
}
#endif /* PIOS_INCLUDE_VIDEO */

/**
 * Check the brown out reset threshold is 2.7 volts and if not
 * resets it.
 */
void check_bor()
{
	uint8_t bor = FLASH_OB_GetBOR();

	if (bor != OB_BOR_LEVEL3) {
		FLASH_OB_Unlock();
		FLASH_OB_BORConfig(OB_BOR_LEVEL3);
		FLASH_OB_Launch();
		while (FLASH_WaitForLastOperation() == FLASH_BUSY) {
			;
		}
		FLASH_OB_Lock();
		while (FLASH_WaitForLastOperation() == FLASH_BUSY) {
			;
		}
	}
}

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

void PIOS_Board_Init(void) {
	bool use_rxport_usart = false;

	check_bor();

	/* Delay system */
	PIOS_DELAY_Init();

#if defined(PIOS_INCLUDE_ANNUNC)
	PIOS_ANNUNC_Init(&pios_annunc_cfg);
#endif	/* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_internal_id, &pios_i2c_internal_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
	if (PIOS_I2C_CheckClear(pios_i2c_internal_id) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_I2C_INT);
#endif

#if defined(PIOS_INCLUDE_SPI)
	if (PIOS_SPI_Init(&pios_spi_flash_id, &pios_spi_flash_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
#endif

#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
	if (PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FLASH);
	if (PIOS_Flash_Jedec_Init(&pios_external_flash_id, pios_spi_flash_id, 0, &flash_mx25_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FLASH);

	/* Register the partition table */
	PIOS_FLASH_register_partition_table(pios_flash_partition_table, NELEMENTS(pios_flash_partition_table));

	/* Mount all filesystems */
	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_settings_cfg, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FILESYS);
#endif	/* PIOS_INCLUDE_FLASH */

	/* Initialize UAVObject libraries */
	UAVObjInitialize();

	HwBrainInitialize();
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

	/* Initialize the alarms library */
	AlarmsInitialize();
	PIOS_RESET_Clear();

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Set up pulse timers */
	//inputs
	PIOS_TIM_InitClock(&tim_8_cfg);
	PIOS_TIM_InitClock(&tim_12_cfg);
	//outputs
	PIOS_TIM_InitClock(&tim_5_cfg);

	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwBrainSetDefaults(HwBrainHandle(), 0);
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
	HwBrainUSB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWBRAIN_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);
	
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwBrainUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWBRAIN_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);
	
#endif	/* PIOS_INCLUDE_USB_HID */

	if (usb_hid_present || usb_cdc_present) {
		PIOS_USBHOOK_Activate();
	}
#endif	/* PIOS_INCLUDE_USB */

	/* Configure the IO ports */
	HwBrainDSMxModeOptions hw_DSMxMode;
	HwBrainDSMxModeGet(&hw_DSMxMode);

	/* Main Port */
	uint8_t hw_mainport;
	HwBrainMainPortGet(&hw_mainport);

	PIOS_HAL_ConfigurePort(hw_mainport,          // port type protocol
			&pios_mainport_cfg,                  // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			NULL,                                // i2c_id
			NULL,                                // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			&pios_mainport_dsm_aux_cfg,          // dsm_cfg
			0,                                   // dsm_mode
			&pios_mainport_sbus_aux_cfg);        // sbus_cfg

	/* Flx Port */
	uint8_t hw_flxport;
	HwBrainFlxPortGet(&hw_flxport);

	PIOS_HAL_ConfigurePort(hw_flxport,           // port type protocol
			&pios_flxport_cfg,                   // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			&pios_i2c_flexi_id,                  // i2c_id
			&pios_i2c_flexi_cfg,                 // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			&pios_flxport_dsm_aux_cfg,           // dsm_cfg
			hw_DSMxMode,                         // dsm_mode
			NULL);                               // sbus_cfg

	/* Configure the rcvr port */
	uint8_t hw_rxport;
	HwBrainRxPortGet(&hw_rxport);

	switch (hw_rxport) {
	case HWBRAIN_RXPORT_DISABLED:
		break;

	case HWBRAIN_RXPORT_PWM:
		PIOS_HAL_ConfigurePort(HWSHARED_PORTTYPES_PWM,  // port type protocol
				NULL,                                   // usart_port_cfg
				NULL,                                   // com_driver
				NULL,                                   // i2c_id
				NULL,                                   // i2c_cfg
				NULL,                                   // ppm_cfg
				&pios_pwm_cfg,                          // pwm_cfg
				PIOS_LED_ALARM,                         // led_id
				NULL,                                   // dsm_cfg
				0,                                      // dsm_mode
				NULL);                                  // sbus_cfg
		break;

	case HWBRAIN_RXPORT_PPMFRSKY:
		// special mode that enables PPM, FrSky RSSI, and Sensor Hub
		PIOS_HAL_ConfigurePort(HWSHARED_PORTTYPES_FRSKYSENSORHUB,  // port type protocol
				&pios_rxportusart_cfg,                             // usart_port_cfg
				&pios_usart_com_driver,                            // com_driver
				NULL,                                              // i2c_id
				NULL,                                              // i2c_cfg
				NULL,                                              // ppm_cfg
				NULL,                                              // pwm_cfg
				PIOS_LED_ALARM,                                    // led_id
				NULL,                                              // dsm_cfg
				0,                                                 // dsm_mode
				NULL);                                             // sbus_cfg

	case HWBRAIN_RXPORT_PPM:
	case HWBRAIN_RXPORT_PPMOUTPUTS:
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
		break;

	case HWBRAIN_RXPORT_UART:
	case HWBRAIN_RXPORT_UARTOUTPUTS:
		use_rxport_usart = true;
		break;

	case HWBRAIN_RXPORT_PPMUART:
	case HWBRAIN_RXPORT_PPMUARTOUTPUTS:
		use_rxport_usart = true;

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
		break;
	}

	/* Configure the RxPort USART */
	if (use_rxport_usart) {
		uint8_t hw_rxportusart;
		HwBrainRxPortUsartGet(&hw_rxportusart);

		PIOS_HAL_ConfigurePort(hw_rxportusart,       // port type protocol
				&pios_rxportusart_cfg,               // usart_port_cfg
				&pios_usart_com_driver,              // com_driver
				NULL,                                // i2c_id
				NULL,                                // i2c_cfg
				NULL,                                // ppm_cfg
				NULL,                                // pwm_cfg
				PIOS_LED_ALARM,                      // led_id
				&pios_rxportusart_dsm_aux_cfg,       // dsm_cfg
				hw_DSMxMode,                         // dsm_mode
				NULL);                               // sbus_cfg
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

#ifndef PIOS_DEBUG_ENABLE_DEBUG_PINS
	switch (hw_rxport) {
	case HWBRAIN_RXPORT_DISABLED:
	case HWBRAIN_RXPORT_PWM:
	case HWBRAIN_RXPORT_PPM:
	case HWBRAIN_RXPORT_UART:
	case HWBRAIN_RXPORT_PPMUART:
	/* Set up the servo outputs */
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_cfg);
#endif
		break;
	case HWBRAIN_RXPORT_PPMOUTPUTS:
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_rcvr_ppm_cfg);
#endif
		break;
	case HWBRAIN_RXPORT_PPMUARTOUTPUTS:
	case HWBRAIN_RXPORT_UARTOUTPUTS:
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_rcvr_ppm_uart_out_cfg);
#endif
		break;
	case HWBRAIN_RXPORT_PPMFRSKY:
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_cfg);
#endif
#if defined(PIOS_INCLUDE_FRSKY_RSSI)
		PIOS_FrSkyRssi_Init(&pios_frsky_rssi_cfg);
#endif /* PIOS_INCLUDE_FRSKY_RSSI */
		break;
	case HWBRAIN_RXPORT_OUTPUTS:
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_rcvr_all_cfg);
#endif
		break;
	}
#else
	PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
#endif

#if defined(PIOS_INCLUDE_GPIO)
	PIOS_GPIO_Init();
#endif

#ifdef PIOS_INCLUDE_DAC
        PIOS_DAC_init(&pios_dac, &pios_dac_cfg);

        PIOS_HAL_ConfigureDAC(pios_dac);
#endif /* PIOS_INCLUDE_DAC */


	/* init sensor queue registration */
	PIOS_SENSORS_Init();

	//I2C is slow, sensor init as well, reset watchdog to prevent reset here
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MS5611)
	if ((PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_internal_id) != 0)
			|| (PIOS_MS5611_Test() != 0))
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_BARO);
#endif

	PIOS_WDG_Clear();

	/* Magnetometer selection */
	bool use_internal_mag = true;
	uint8_t hw_magnetometer;
	HwBrainMagnetometerGet(&hw_magnetometer);
	switch (hw_magnetometer) {
		case HWBRAIN_MAGNETOMETER_NONE:
			use_internal_mag = false;
			break;

		case HWBRAIN_MAGNETOMETER_INTERNAL:
			use_internal_mag = true;
			break;

		/* default external mags and handle them in PiOS HAL rather than maintaining list here */
		default:
			use_internal_mag = false;

			if (hw_flxport == HWSHARED_PORTTYPES_I2C) {
				uint8_t hw_orientation;
				HwBrainExtMagOrientationGet(&hw_orientation);

				PIOS_HAL_ConfigureExternalMag(hw_magnetometer, hw_orientation,
					&pios_i2c_flexi_id, &pios_i2c_flexi_cfg);
			} else {
				PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
			}
			break;
	}

#if defined(PIOS_INCLUDE_MPU9250_BRAIN)
#if defined(PIOS_INCLUDE_MPU6050)
	// Enable autoprobing when both 6050 and 9050 compiled in
	bool mpu9250_found = false;
	if (PIOS_MPU9250_Probe(pios_i2c_internal_id, PIOS_MPU9250_I2C_ADD_A0_LOW) == 0) {
		mpu9250_found = true;
#else
	{
#endif /* PIOS_INCLUDE_MPU6050 */
		uint8_t hw_mpu9250_dlpf;
		HwBrainMPU9250GyroLPFGet(&hw_mpu9250_dlpf);
		enum pios_mpu9250_gyro_filter mpu9250_gyro_lpf = \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250GYROLPF_184) ? PIOS_MPU9250_GYRO_LOWPASS_184_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250GYROLPF_92) ? PIOS_MPU9250_GYRO_LOWPASS_92_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250GYROLPF_41) ? PIOS_MPU9250_GYRO_LOWPASS_41_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250GYROLPF_20) ? PIOS_MPU9250_GYRO_LOWPASS_20_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250GYROLPF_10) ? PIOS_MPU9250_GYRO_LOWPASS_10_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250GYROLPF_5) ? PIOS_MPU9250_GYRO_LOWPASS_5_HZ : \
			PIOS_MPU9250_GYRO_LOWPASS_184_HZ;

		HwBrainMPU9250AccelLPFGet(&hw_mpu9250_dlpf);
		enum pios_mpu9250_accel_filter mpu9250_accel_lpf = \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250ACCELLPF_460) ? PIOS_MPU9250_ACCEL_LOWPASS_460_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250ACCELLPF_184) ? PIOS_MPU9250_ACCEL_LOWPASS_184_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250ACCELLPF_92) ? PIOS_MPU9250_ACCEL_LOWPASS_92_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250ACCELLPF_41) ? PIOS_MPU9250_ACCEL_LOWPASS_41_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250ACCELLPF_20) ? PIOS_MPU9250_ACCEL_LOWPASS_20_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250ACCELLPF_10) ? PIOS_MPU9250_ACCEL_LOWPASS_10_HZ : \
			(hw_mpu9250_dlpf == HWBRAIN_MPU9250ACCELLPF_5) ? PIOS_MPU9250_ACCEL_LOWPASS_5_HZ : \
			PIOS_MPU9250_ACCEL_LOWPASS_184_HZ;

		int retval;
		retval = PIOS_MPU9250_Init(pios_i2c_internal_id, PIOS_MPU9250_I2C_ADD_A0_LOW, use_internal_mag,
									mpu9250_gyro_lpf, mpu9250_accel_lpf, &pios_mpu9250_cfg);
		if (retval == -10)
			PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);
		if (retval != 0)
			PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

		// To be safe map from UAVO enum to driver enum
		uint8_t hw_gyro_range;
		HwBrainGyroFullScaleGet(&hw_gyro_range);
		switch(hw_gyro_range) {
			case HWBRAIN_GYROFULLSCALE_250:
				PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_250_DEG);
				break;
			case HWBRAIN_GYROFULLSCALE_500:
				PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_500_DEG);
				break;
			case HWBRAIN_GYROFULLSCALE_1000:
				PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_1000_DEG);
				break;
			case HWBRAIN_GYROFULLSCALE_2000:
				PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_2000_DEG);
				break;
		}

		uint8_t hw_accel_range;
		HwBrainAccelFullScaleGet(&hw_accel_range);
		switch(hw_accel_range) {
			case HWBRAIN_ACCELFULLSCALE_2G:
				PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_2G);
				break;
			case HWBRAIN_ACCELFULLSCALE_4G:
				PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_4G);
				break;
			case HWBRAIN_ACCELFULLSCALE_8G:
				PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_8G);
				break;
			case HWBRAIN_ACCELFULLSCALE_16G:
				PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_16G);
				break;
		}
	}
#endif /* PIOS_INCLUDE_MPU9250_BRAIN */

	PIOS_WDG_Clear();

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
