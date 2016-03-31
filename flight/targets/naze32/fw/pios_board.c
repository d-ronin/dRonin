/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Naze family support files
 * @{
 *
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
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
#include "firmwareiap.h"
#include "hwnaze.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"


#define PIOS_COM_TELEM_RF_RX_BUF_LEN 32
#define PIOS_COM_TELEM_RF_TX_BUF_LEN 12

uintptr_t pios_com_lighttelemetry_id;

uintptr_t pios_uavo_settings_fs_id;

uintptr_t pios_internal_adc_id;

#if defined(PIOS_INCLUDE_MSP_BRIDGE)
extern uintptr_t pios_com_msp_id;
#endif

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

//#include <pios_board_info.h>
//from flight/targets/naze32/board-info/system_stm32f10x.c
extern uint32_t hse_value;
static enum board_revision board_rev;

void PIOS_Board_Init(void) {

	/* Delay system */
	PIOS_DELAY_Init();

	board_rev = (hse_value == 12000000) ? BOARD_REVISION_5 : BOARD_REVISION_4;

	//TODO: Buzzer
	//rev5 needs inverted beeper.

	//const struct pios_board_info * bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_LED)
	const struct pios_led_cfg * led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(1);
	PIOS_Assert(led_cfg);
	PIOS_LED_Init(led_cfg);
#endif	/* PIOS_INCLUDE_LED */

#if defined(PIOS_INCLUDE_SPI)
	/* Set up the SPI interface to the serial flash */

	if (PIOS_SPI_Init(&pios_spi_generic_id, &pios_spi_generic_cfg)) {
		PIOS_Assert(0);
	}
#endif

#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_internal_id, &pios_i2c_internal_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
	if (PIOS_I2C_CheckClear(pios_i2c_internal_id) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_I2C_INT);
#endif

#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
	if (PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_FLASH);

	/* Register the partition table */
	const struct pios_flash_partition * flash_partition_table;
	uint32_t num_partitions;
	flash_partition_table = PIOS_BOARD_HW_DEFS_GetPartitionTable(1, &num_partitions);
	PIOS_FLASH_register_partition_table(flash_partition_table, num_partitions);

	/* Mount all filesystems */
	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_internal_settings_cfg, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_FILESYS);

#endif	/* PIOS_INCLUDE_FLASH */

	/* Initialize UAVObject libraries */
	UAVObjInitialize();

	/* Initialize the hardware UAVOs */
	HwNazeInitialize();
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

	/* Initialize the alarms library. Reads RCC reset flags */
	AlarmsInitialize();
	PIOS_RESET_Clear(); // Clear the RCC reset flags after use.

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Set up pulse timers */
	//inputs

	//outputs
	PIOS_TIM_InitClock(&tim_1_cfg);
	PIOS_TIM_InitClock(&tim_2_cfg);
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
		HwNazeSetDefaults(HwNazeHandle(), 0);
		ModuleSettingsSetDefaults(ModuleSettingsHandle(),0);
		AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
	}

#if (defined(PIOS_INCLUDE_TELEMETRY) || defined(PIOS_INCLUDE_MSP_BRIDGE)) && defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)

	/* UART1 Port */
	uint8_t hw_mainport;
	HwNazeMainPortGet(&hw_mainport);

	PIOS_HAL_ConfigurePort(hw_mainport,          // port type protocol
			&pios_usart_main_cfg,                // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			NULL,                                // i2c_id
			NULL,                                // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			NULL,                                // dsm_cfg
			0,                                   // dsm_mode
			NULL);                               // sbus_cfg

#endif /* PIOS_INCLUDE_TELEMETRY || PIOS_INCLUDE_MSP_BRIDGE */

	/* Configure the rcvr port */
	uint8_t hw_rcvrport;
	HwNazeRcvrPortGet(&hw_rcvrport);

	switch (hw_rcvrport) {

	case HWNAZE_RCVRPORT_DISABLED:
		break;

	case HWNAZE_RCVRPORT_PWM:
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

	case HWNAZE_RCVRPORT_PPMSERIAL:
	case HWNAZE_RCVRPORT_SERIAL:
		{
			uint8_t hw_rcvrserial;
			HwNazeRcvrSerialGet(&hw_rcvrserial);

			HwNazeDSMxModeOptions hw_DSMxMode;
			HwNazeDSMxModeGet(&hw_DSMxMode);

			PIOS_HAL_ConfigurePort(hw_rcvrserial,        // port type protocol
					&pios_usart_rcvrserial_cfg,          // usart_port_cfg
					&pios_usart_com_driver,              // com_driver
					NULL,                                // i2c_id
					NULL,                                // i2c_cfg
					NULL,                                // ppm_cfg
					NULL,                                // pwm_cfg
					PIOS_LED_ALARM,                      // led_id
					&pios_dsm_rcvrserial_cfg,            // dsm_cfg
					hw_DSMxMode,                         // dsm_mode
					NULL);                               // sbus_cfg
		}

		if (hw_rcvrport == HWNAZE_RCVRPORT_SERIAL)
			break;

		// Else fall through to set up PPM.

	case HWNAZE_RCVRPORT_PPM:
	case HWNAZE_RCVRPORT_PPMOUTPUTS:
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

	case HWNAZE_RCVRPORT_PPMPWM:
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

		PIOS_HAL_ConfigurePort(HWSHARED_PORTTYPES_PWM,  // port type protocol
				NULL,                                   // usart_port_cfg
				NULL,                                   // com_driver
				NULL,                                   // i2c_id
				NULL,                                   // i2c_cfg
				NULL,                                   // ppm_cfg
				&pios_pwm_with_ppm_cfg,                 // pwm_cfg
				PIOS_LED_ALARM,                         // led_id
				NULL,                                   // dsm_cfg
				0,                                      // dsm_mode
				NULL);                                  // sbus_cfg
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

	/* Remap AFIO pin for PB4 (Servo 5 Out)*/
	GPIO_PinRemapConfig( GPIO_Remap_SWJ_NoJTRST, ENABLE);

#ifndef PIOS_DEBUG_ENABLE_DEBUG_PINS
#ifdef PIOS_INCLUDE_SERVO
	switch (hw_rcvrport) {
		case HWNAZE_RCVRPORT_DISABLED:
		case HWNAZE_RCVRPORT_PWM:
		case HWNAZE_RCVRPORT_PPM:
		case HWNAZE_RCVRPORT_PPMPWM:
		case HWNAZE_RCVRPORT_PPMSERIAL:
		case HWNAZE_RCVRPORT_SERIAL:
			PIOS_Servo_Init(&pios_servo_cfg);
			break;
		case HWNAZE_RCVRPORT_PPMOUTPUTS:
		case HWNAZE_RCVRPORT_OUTPUTS:
			PIOS_Servo_Init(&pios_servo_rcvr_cfg);
			break;
	}
#endif
#else
	PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
#endif

#if defined(PIOS_INCLUDE_ADC)
	{
		uint16_t number_of_adc_pins = 2; // first two pins are always available
		switch(hw_rcvrport) {
		case HWNAZE_RCVRPORT_PPM:
		case HWNAZE_RCVRPORT_PPMSERIAL:
		case HWNAZE_RCVRPORT_SERIAL:
			number_of_adc_pins += 2; // rcvr port pins also available
			break;
		default:
			break;
		}
		uint32_t internal_adc_id;
		PIOS_INTERNAL_ADC_LIGHT_Init(&internal_adc_id, &internal_adc_cfg, number_of_adc_pins);
		PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id);
	}
#endif /* PIOS_INCLUDE_ADC */
	PIOS_WDG_Clear();
	PIOS_DELAY_WaitmS(200);
	PIOS_WDG_Clear();

	PIOS_SENSORS_Init();

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

#if defined(PIOS_INCLUDE_MPU)
	uint8_t mpu_pin;
	HwNazeMPU6050IntPinGet(&mpu_pin);
	if(((board_rev == BOARD_REVISION_5) && mpu_pin == HWNAZE_MPU6050INTPIN_AUTO) || mpu_pin == HWNAZE_MPU6050INTPIN_PC13) {
		pios_mpu_cfg.exti_cfg = &pios_exti_mpu_cfg_v5;
	}

	pios_mpu_dev_t mpu_dev = NULL;
	int retval = PIOS_MPU_I2C_Init(&mpu_dev, pios_i2c_internal_id, &pios_mpu_cfg);
	if (retval != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

	if (PIOS_MPU_GetType() == PIOS_MPU6500)
		board_rev = BOARD_REVISION_6;

	// we now know exactly what rev the hardware is
	FirmwareIAPSetBoardRev(board_rev);

	uint8_t hw_gyro_range;
	HwNazeGyroRangeGet(&hw_gyro_range);
	switch(hw_gyro_range) {
		case HWNAZE_GYRORANGE_250:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_250_DEG);
			break;
		case HWNAZE_GYRORANGE_500:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_500_DEG);
			break;
		case HWNAZE_GYRORANGE_1000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_1000_DEG);
			break;
		case HWNAZE_GYRORANGE_2000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_2000_DEG);
			break;
	}

	uint8_t hw_accel_range;
	HwNazeAccelRangeGet(&hw_accel_range);
	switch(hw_accel_range) {
		case HWNAZE_ACCELRANGE_2G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_2G);
			break;
		case HWNAZE_ACCELRANGE_4G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_4G);
			break;
		case HWNAZE_ACCELRANGE_8G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_8G);
			break;
		case HWNAZE_ACCELRANGE_16G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_16G);
			break;
	}

	// the filter has to be set before rate else divisor calculation will fail
	uint8_t hw_mpu_dlpf;
	HwNazeMPU6050DLPFGet(&hw_mpu_dlpf);
	uint16_t bandwidth = \
			    (hw_mpu_dlpf == HWNAZE_MPU6050DLPF_256) ? 250 : \
			    (hw_mpu_dlpf == HWNAZE_MPU6050DLPF_188) ? 184 : \
			    (hw_mpu_dlpf == HWNAZE_MPU6050DLPF_98) ? 92 : \
			    (hw_mpu_dlpf == HWNAZE_MPU6050DLPF_42) ? 41 : \
			    (hw_mpu_dlpf == HWNAZE_MPU6050DLPF_20) ? 20 : \
			    (hw_mpu_dlpf == HWNAZE_MPU6050DLPF_10) ? 10 : \
			    (hw_mpu_dlpf == HWNAZE_MPU6050DLPF_5) ? 5 : \
			    184;
	PIOS_MPU_SetGyroBandwidth(bandwidth);

	uint8_t hw_mpu_samplerate;
	HwNazeMPU6050RateGet(&hw_mpu_samplerate);
	uint16_t mpu_samplerate = \
			    (hw_mpu_samplerate == HWNAZE_MPU6050RATE_200) ? 200 : \
			    (hw_mpu_samplerate == HWNAZE_MPU6050RATE_333) ? 333 : \
			    (hw_mpu_samplerate == HWNAZE_MPU6050RATE_500) ? 500 : \
			    pios_mpu_cfg.default_samplerate;
	PIOS_MPU_SetSampleRate(mpu_samplerate);

#endif /* PIOS_INCLUDE_MPU */

	//I2C is slow, sensor init as well, reset watchdog to prevent reset here
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MS5611)
	if (PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_internal_id) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_BARO);
	if (PIOS_MS5611_Test() != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_BARO);
#endif

#if defined(PIOS_INCLUDE_HMC5883)
	//TODO: if(board_v5) { /* use PC14 instead of PB12 for MAG_DRDY (exti) */ }
	if (PIOS_HMC5883_Init(pios_i2c_internal_id, &pios_hmc5883_cfg) != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_MAG);
        if (PIOS_HMC5883_Test() != 0)
		PIOS_HAL_Panic(PIOS_LED_ALARM, PIOS_HAL_PANIC_MAG);
#endif

#if defined(PIOS_INCLUDE_GPIO)
	PIOS_GPIO_Init();
#endif
}

/**
 * @}
 */
