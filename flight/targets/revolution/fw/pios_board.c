/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup RevoMini OpenPilot Revolution Mini
 * @{
 *
 * @file       revolution/fw/pios_board.c
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
#include "hwrevolution.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"
#include <rfm22bstatus.h>
#include <pios_max7456.h>
#include <pios_rfm22b_rcvr_priv.h>
#include <pios_openlrs_rcvr_priv.h>

#if defined(PIOS_INCLUDE_WS2811)
#include "rgbledsettings.h"
#endif

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
#define PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN 40
uintptr_t pios_com_debug_id;
#endif	/* PIOS_INCLUDE_DEBUG_CONSOLE */

uintptr_t pios_internal_adc_id = 0;
uintptr_t pios_uavo_settings_fs_id;
uintptr_t pios_com_openlog_logging_id;

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
	bool use_rxport_usart = false;

	/* some clone boards omit components, we will ignore that if running on a clone */
	bool is_modified_clone = false;

	const struct pios_board_info * bdinfo = &pios_board_info_blob;

#if defined(PIOS_INCLUDE_ANNUNC)
	const struct pios_annunc_cfg * led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
	PIOS_Assert(led_cfg);
	PIOS_ANNUNC_Init(led_cfg);
#endif	/* PIOS_INCLUDE_ANNUNC */

	pios_spi_t pios_spi_gyro_id;
	/* Set up the SPI interface to the gyro/acelerometer */
	if (PIOS_SPI_Init(&pios_spi_gyro_id, &pios_spi_gyro_cfg)) {
		PIOS_DEBUG_Assert(0);
	}

	/* Set up the SPI interface to the flash and rfm22b */
	if (PIOS_SPI_Init(&pios_spi_telem_flash_id, &pios_spi_telem_flash_cfg)) {
		PIOS_DEBUG_Assert(0);
	}

#if defined(PIOS_INCLUDE_FLASH)
	/* Inititialize all flash drivers */
	bool flash_chip_ok = false;
	for (int i = 0; i < NELEMENTS(flash_chip_cfgs); i++) {
		if (PIOS_Flash_Jedec_Init(&pios_external_flash_id, pios_spi_telem_flash_id,
				1, &flash_chip_cfgs[i]) == 0) {
			flash_chip_ok = true;
			is_modified_clone = i > 0;
			break;
		}
	}

	if (!flash_chip_ok) {
		/* Cope with clones with no flash, too. */

		is_modified_clone = true;

		/* Override LED configuration for Omnibus things */
		PIOS_ANNUNC_Init(&pios_annunc_omnibus_cfg);

		pios_external_flash_id = 0;
	}

	PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg);

	/* Register the partition table */
	const struct pios_flash_partition * flash_partition_table;
	uint32_t num_partitions;

	flash_partition_table = PIOS_BOARD_HW_DEFS_GetPartitionTable(bdinfo->board_rev, &num_partitions);
	PIOS_FLASH_register_partition_table(flash_partition_table, num_partitions);

	/* Mount all filesystems */
	if(PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_settings_cfg, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_HEARTBEAT, PIOS_HAL_PANIC_FILESYS);
#endif	/* PIOS_INCLUDE_FLASH */

	PIOS_RESET_Clear(); // Clear the RCC reset flags after use.

	/* Initialize the hardware UAVOs */
	HwRevolutionInitialize();
	ModuleSettingsInitialize();

#if defined(PIOS_INCLUDE_RTC)
	PIOS_RTC_Init(&pios_rtc_main_cfg);
#endif

	/* Initialize watchdog as early as possible to catch faults during init
	 * but do it only if there is no debugger connected
	 */
	if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0) {
		PIOS_WDG_Init();
	}

	/* Set up pulse timers */
	PIOS_TIM_InitClock(&tim_1_cfg);
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_4_cfg);
	PIOS_TIM_InitClock(&tim_5_cfg);
	PIOS_TIM_InitClock(&tim_8_cfg);
	PIOS_TIM_InitClock(&tim_9_cfg);
	PIOS_TIM_InitClock(&tim_10_cfg);
	PIOS_TIM_InitClock(&tim_11_cfg);
	PIOS_TIM_InitClock(&tim_12_cfg);
	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwRevolutionSetDefaults(HwRevolutionHandle(), 0);
		ModuleSettingsSetDefaults(ModuleSettingsHandle(),0);
		AlarmsSet(SYSTEMALARMS_ALARM_BOOTFAULT, SYSTEMALARMS_ALARM_CRITICAL);
	}


	//PIOS_IAP_Init();

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
	HwRevolutionUSB_VCPPortGet(&hw_usb_vcpport);

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport,
			pios_usb_id, &pios_usb_cdc_cfg);
#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwRevolutionUSB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWREVOLUTION_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);
#endif	/* PIOS_INCLUDE_USB_HID */

	if (usb_hid_present || usb_cdc_present) {
		PIOS_USBHOOK_Activate();
	}
#endif	/* PIOS_INCLUDE_USB */

	/* Configure IO ports */

#if defined(PIOS_INCLUDE_I2C)
	if (PIOS_I2C_Init(&pios_i2c_mag_pressure_adapter_id, &pios_i2c_mag_pressure_adapter_cfg))
		PIOS_DEBUG_Assert(0);

	if (PIOS_I2C_CheckClear(pios_i2c_mag_pressure_adapter_id) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_HEARTBEAT, PIOS_HAL_PANIC_I2C_INT);
	else
		if (AlarmsGet(SYSTEMALARMS_ALARM_I2C) == SYSTEMALARMS_ALARM_UNINITIALISED)
			AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_OK);
#endif  // PIOS_INCLUDE_I2C

	HwRevolutionDSMxModeOptions hw_DSMxMode;
	HwRevolutionDSMxModeGet(&hw_DSMxMode);

	/* Configure main USART port */

	uint8_t hw_mainport;
	HwRevolutionMainPortGet(&hw_mainport);

	PIOS_HAL_ConfigurePort(hw_mainport,          // port type protocol
			&pios_usart_main_cfg,                // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			NULL,                                // i2c_id
			NULL,                                // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_HEARTBEAT,                      // led_id
			&pios_dsm_main_cfg,                  // dsm_cfg
			hw_DSMxMode >= HWREVOLUTION_DSMXMODE_BIND3PULSES ? HWREVOLUTION_DSMXMODE_AUTODETECT : hw_DSMxMode /* No bind on main port */,
			&pios_sbus_cfg);                     // sbus_cfg

	/* Configure FlexiPort */
	uint8_t hw_flexiport;
	HwRevolutionFlexiPortGet(&hw_flexiport);

	PIOS_HAL_ConfigurePort(hw_flexiport,         // port type protocol
			&pios_usart_flexi_cfg,               // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			&pios_i2c_flexiport_adapter_id,      // i2c_id
			&pios_i2c_flexiport_adapter_cfg,     // i2c_cfg
			NULL,                                // ppm_cfg
			NULL,                                // pwm_cfg
			PIOS_LED_HEARTBEAT,                      // led_id
			&pios_dsm_flexi_cfg,                 // dsm_cfg
			hw_DSMxMode,                         // dsm_mode
			NULL);                               // sbus_cfg

	/* Configure the receiver port*/
	uint8_t hw_rxport;
	HwRevolutionRxPortGet(&hw_rxport);

	switch (hw_rxport) {
	case HWREVOLUTION_RXPORT_DISABLED:
		break;

	case HWREVOLUTION_RXPORT_PWM:
		PIOS_HAL_ConfigurePort(HWSHARED_PORTTYPES_PWM,  // port type protocol
				NULL,                                   // usart_port_cfg
				NULL,                                   // com_driver
				NULL,                                   // i2c_id
				NULL,                                   // i2c_cfg
				NULL,                                   // ppm_cfg
				&pios_pwm_cfg,                          // pwm_cfg
				PIOS_LED_HEARTBEAT,                         // led_id
				NULL,                                   // dsm_cfg
				0,                                      // dsm_mode
				NULL);                                  // sbus_cfg
		break;

	case HWREVOLUTION_RXPORT_PPMFRSKY:
		// special mode that enables PPM, FrSky RSSI, and Sensor Hub
		PIOS_HAL_ConfigurePort(HWSHARED_PORTTYPES_FRSKYSENSORHUB,  // port type protocol
				&pios_rxportusart_cfg,                             // usart_port_cfg
				&pios_usart_com_driver,                            // com_driver
				NULL,                                              // i2c_id
				NULL,                                              // i2c_cfg
				NULL,                                              // ppm_cfg
				NULL,                                              // pwm_cfg
				PIOS_LED_HEARTBEAT,                                    // led_id
				NULL,                                              // dsm_cfg
				0,                                                 // dsm_mode
				NULL);                                             // sbus_cfg

	case HWREVOLUTION_RXPORT_PPM:
	case HWREVOLUTION_RXPORT_PPMOUTPUTS:
		PIOS_HAL_ConfigurePort(HWSHARED_PORTTYPES_PPM,  // port type protocol
				NULL,                                   // usart_port_cfg
				NULL,                                   // com_driver
				NULL,                                   // i2c_id
				NULL,                                   // i2c_cfg
				&pios_ppm_cfg,                          // ppm_cfg
				NULL,                                   // pwm_cfg
				PIOS_LED_HEARTBEAT,                         // led_id
				NULL,                                   // dsm_cfg
				0,                                      // dsm_mode
				NULL);                                  // sbus_cfg
		break;

	case HWREVOLUTION_RXPORT_UART:
		use_rxport_usart = true;
		break;

	case HWREVOLUTION_RXPORT_PPMUART:
		use_rxport_usart = true;

		PIOS_HAL_ConfigurePort(HWSHARED_PORTTYPES_PPM,  // port type protocol
				NULL,                                   // usart_port_cfg
				NULL,                                   // com_driver
				NULL,                                   // i2c_id
				NULL,                                   // i2c_cfg
				&pios_ppm_cfg,                          // ppm_cfg
				NULL,                                   // pwm_cfg
				PIOS_LED_HEARTBEAT,                         // led_id
				NULL,                                   // dsm_cfg
				0,                                      // dsm_mode
				NULL);                                  // sbus_cfg
		break;
	}

	/* Configure the RxPort USART */
	if (use_rxport_usart) {
		uint8_t hw_rxportusart;
		HwRevolutionRxPortUsartGet(&hw_rxportusart);

		PIOS_HAL_ConfigurePort(hw_rxportusart,       // port type protocol
				&pios_rxportusart_cfg,               // usart_port_cfg
				&pios_usart_com_driver,              // com_driver
				NULL,                                // i2c_id
				NULL,                                // i2c_cfg
				NULL,                                // ppm_cfg
				NULL,                                // pwm_cfg
				PIOS_LED_HEARTBEAT,                      // led_id
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
	case HWREVOLUTION_RXPORT_DISABLED:
	case HWREVOLUTION_RXPORT_PWM:
	case HWREVOLUTION_RXPORT_PPM:
	case HWREVOLUTION_RXPORT_UART:
	case HWREVOLUTION_RXPORT_PPMUART:
	/* Set up the servo outputs */
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_cfg);
#endif
		break;
	case HWREVOLUTION_RXPORT_PPMOUTPUTS:
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_rcvr_ppm_cfg);
#endif
		break;
	case HWREVOLUTION_RXPORT_PPMFRSKY:
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_cfg);
#endif
#if defined(PIOS_INCLUDE_FRSKY_RSSI)
		PIOS_FrSkyRssi_Init(&pios_frsky_rssi_cfg);
#endif /* PIOS_INCLUDE_FRSKY_RSSI */
		break;
	case HWREVOLUTION_RXPORT_OUTPUTS:
#ifdef PIOS_INCLUDE_SERVO
		PIOS_Servo_Init(&pios_servo_rcvr_all_cfg);
#endif
		break;
	}
#else
	PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
#endif

	HwRevolutionData hwRevoMini;
	HwRevolutionGet(&hwRevoMini);

	uint8_t hw_spiperiph;

	HwRevolutionSPIPeripheralGet(&hw_spiperiph);

	switch (hw_spiperiph) {
		default:
			PIOS_Assert(0);
			break;
		case HWREVOLUTION_SPIPERIPHERAL_DISABLED:
			break;

#ifdef PIOS_INCLUDE_MAX7456
		case HWREVOLUTION_SPIPERIPHERAL_MAX7456OSD:
			if (!PIOS_MAX7456_init(&pios_max7456_id, pios_spi_telem_flash_id,
						0)) {
				// XXX do something?
			}
			break;
#endif

#ifdef PIOS_INCLUDE_RFM22B
		case HWREVOLUTION_SPIPERIPHERAL_RADIO:
			(void) 0;
			const struct pios_openlrs_cfg *openlrs_cfg =PIOS_BOARD_HW_DEFS_GetOpenLRSCfg(bdinfo->board_rev);
			const struct pios_rfm22b_cfg *rfm22b_cfg = PIOS_BOARD_HW_DEFS_GetRfm22Cfg(bdinfo->board_rev);

			PIOS_HAL_ConfigureRFM22B(hwRevoMini.Radio,
					pios_spi_telem_flash_id,
					bdinfo->board_type, bdinfo->board_rev,
					hwRevoMini.MaxRfPower,
					hwRevoMini.MaxRfSpeed,
					hwRevoMini.RfBand,
					openlrs_cfg, rfm22b_cfg,
					hwRevoMini.MinChannel,
					hwRevoMini.MaxChannel,
					hwRevoMini.CoordID, 1);
			break;
#endif /* PIOS_INCLUDE_RFM22B */
	}

#ifdef PIOS_INCLUDE_WS2811
	uint8_t num_leds;

	RGBLEDSettingsInitialize();

	RGBLEDSettingsNumLedsGet(&num_leds);

	if (num_leds > 0) {
		switch (hwRevoMini.WS2811Pin) {
			default:
			case HWREVOLUTION_WS2811PIN_DISABLED:
				break;

			case HWREVOLUTION_WS2811PIN_PB12INPORTPIN3:
				PIOS_WS2811_init(&pios_ws2811,
						&pios_ws2811_cfg_pb12,
						num_leds);
				PIOS_WS2811_trigger_update(pios_ws2811);
				break;

			case HWREVOLUTION_WS2811PIN_PD2OMNIBUSF4LEDPORT:
				if (hw_spiperiph == HWREVOLUTION_SPIPERIPHERAL_RADIO) {
					// Not compatible with radio use.
					break;
				}

				PIOS_WS2811_init(&pios_ws2811,
						&pios_ws2811_cfg_pd2,
						num_leds);
				PIOS_WS2811_trigger_update(pios_ws2811);
				break;

			case HWREVOLUTION_WS2811PIN_PB6OMNIBUSF4V2LEDPORT:
				PIOS_WS2811_init(&pios_ws2811,
						&pios_ws2811_cfg_pb6,
						num_leds);
				PIOS_WS2811_trigger_update(pios_ws2811);
				break;

			case HWREVOLUTION_WS2811PIN_PA0SERVOOUT6:
				/* 0 indexed */
				PIOS_Servo_DisableChannel(6 - 1);
				PIOS_WS2811_init(&pios_ws2811,
						&pios_ws2811_cfg_pa0,
						num_leds);
				PIOS_WS2811_trigger_update(pios_ws2811);
				break;
		}
	}
#endif /* PIOS_INCLUDE_WS2811 */

	/* init sensor queue registration */
	PIOS_SENSORS_Init();

	PIOS_WDG_Clear();
	PIOS_DELAY_WaitmS(200);
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MPU)
	pios_mpu_dev_t mpu_dev = NULL;
	if (PIOS_MPU_SPI_Init(&mpu_dev, pios_spi_gyro_id, 0, &pios_mpu_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_HEARTBEAT, PIOS_HAL_PANIC_IMU);

	HwRevolutionGyroRangeOptions hw_gyro_range;
	HwRevolutionGyroRangeGet(&hw_gyro_range);
	switch(hw_gyro_range) {
		case HWREVOLUTION_GYRORANGE_250:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_250_DEG);
			break;
		case HWREVOLUTION_GYRORANGE_500:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_500_DEG);
			break;
		case HWREVOLUTION_GYRORANGE_1000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_1000_DEG);
			break;
		case HWREVOLUTION_GYRORANGE_2000:
			PIOS_MPU_SetGyroRange(PIOS_MPU_SCALE_2000_DEG);
			break;
	}

	HwRevolutionAccelRangeOptions hw_accel_range;
	HwRevolutionAccelRangeGet(&hw_accel_range);
	switch(hw_accel_range) {
		case HWREVOLUTION_ACCELRANGE_2G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_2G);
			break;
		case HWREVOLUTION_ACCELRANGE_4G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_4G);
			break;
		case HWREVOLUTION_ACCELRANGE_8G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_8G);
			break;
		case HWREVOLUTION_ACCELRANGE_16G:
			PIOS_MPU_SetAccelRange(PIOS_MPU_SCALE_16G);
			break;
	}

	// the filter has to be set before rate else divisor calculation will fail
	HwRevolutionMPU6000DLPFOptions hw_mpu_dlpf;
	HwRevolutionMPU6000DLPFGet(&hw_mpu_dlpf);
	uint16_t bandwidth = \
		(hw_mpu_dlpf == HWREVOLUTION_MPU6000DLPF_188) ? 188 : \
		(hw_mpu_dlpf == HWREVOLUTION_MPU6000DLPF_98)  ? 98  : \
		(hw_mpu_dlpf == HWREVOLUTION_MPU6000DLPF_42)  ? 42  : \
		(hw_mpu_dlpf == HWREVOLUTION_MPU6000DLPF_20)  ? 20  : \
		(hw_mpu_dlpf == HWREVOLUTION_MPU6000DLPF_10)  ? 10  : \
		(hw_mpu_dlpf == HWREVOLUTION_MPU6000DLPF_5)   ? 5   : \
		188;
	PIOS_MPU_SetGyroBandwidth(bandwidth);
#endif

#if defined(PIOS_INCLUDE_I2C)
	PIOS_WDG_Clear();

	uint8_t hw_magnetometer;
	HwRevolutionMagnetometerGet(&hw_magnetometer);

	switch (hw_magnetometer) {
		case HWREVOLUTION_MAGNETOMETER_NONE:
			break;

		case HWREVOLUTION_MAGNETOMETER_INTERNAL:
#if defined(PIOS_INCLUDE_HMC5883)
			if ((PIOS_HMC5883_Init(pios_i2c_mag_pressure_adapter_id, &pios_hmc5883_cfg) != 0) ||
					(PIOS_HMC5883_Test() != 0)) {
				if (!is_modified_clone) {
					PIOS_HAL_CriticalError(PIOS_LED_HEARTBEAT, PIOS_HAL_PANIC_MAG);
				}
			}
#endif /* PIOS_INCLUDE_HMC5883 */
			break;

		/* default external mags and handle them in PiOS HAL rather than maintaining list here */
		default:
			if (hw_flexiport == HWSHARED_PORTTYPES_I2C) {
				uint8_t hw_orientation;
				HwRevolutionExtMagOrientationGet(&hw_orientation);

				PIOS_HAL_ConfigureExternalMag(hw_magnetometer, hw_orientation,
					&pios_i2c_flexiport_adapter_id, &pios_i2c_flexiport_adapter_cfg);
			} else {
				PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
			}
			break;
	}

	/* I2C is slow, sensor init as well, reset watchdog to prevent reset here */
	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_MS5611)
	if ((PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_mag_pressure_adapter_id) != 0) || (PIOS_MS5611_Test() != 0)) {
		if (!is_modified_clone) {
			PIOS_HAL_CriticalError(PIOS_LED_HEARTBEAT, PIOS_HAL_PANIC_BARO);
		}
	}
#endif

	//I2C is slow, sensor init as well, reset watchdog to prevent reset here
	PIOS_WDG_Clear();

#endif    /* PIOS_INCLUDE_I2C */

#if defined(PIOS_INCLUDE_ADC)
	uintptr_t internal_adc_id;
	PIOS_INTERNAL_ADC_Init(&internal_adc_id, &pios_adc_cfg);
	PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id);

		// configure the pullup for PA8 (inhibit pullups from current/sonar shared pin)
		GPIO_Init(pios_current_sonar_pin.gpio, &pios_current_sonar_pin.init);
#endif
}

/**
 * @}
 * @}
 */
