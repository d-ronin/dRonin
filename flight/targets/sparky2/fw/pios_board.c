/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Sparky2 Tau Labs Sparky2 support files
 * @{
 *
 * @file       pios_board.c 
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      Board initialization file
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
#include "hwsparky2.h"
#include "manualcontrolsettings.h"
#include "modulesettings.h"
#include <rfm22bstatus.h>
#include <rfm22breceiver.h>
#include <pios_rfm22b_rcvr_priv.h>
#include <pios_openlrs_rcvr_priv.h>

/**
 * Sensor configurations 
 */

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
 * Configuration for the MPU9250 chip
 */
#if defined(PIOS_INCLUDE_MPU9250_SPI)
#include "pios_mpu9250.h"
static const struct pios_exti_cfg pios_exti_mpu9250_cfg __exti_config = {
	.vector = PIOS_MPU9250_IRQHandler,
	.line = EXTI_Line5,
	.pin = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin = GPIO_Pin_5,
			.GPIO_Speed = GPIO_Speed_100MHz,
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

	.use_magnetometer = true,
	.default_gyro_filter = PIOS_MPU9250_GYRO_LOWPASS_184_HZ,
	.default_accel_filter = PIOS_MPU9250_ACCEL_LOWPASS_184_HZ,
	.orientation = PIOS_MPU9250_TOP_180DEG
};
#endif /* PIOS_INCLUDE_MPU9250_SPI */

/**
 * Configuration for the external HMC5883 chip
 */
#if defined(PIOS_INCLUDE_HMC5883)
#include "pios_hmc5883_priv.h"
static const struct pios_hmc5883_cfg pios_hmc5883_external_cfg = {
	.exti_cfg            = NULL,
	.M_ODR               = PIOS_HMC5883_ODR_75,
	.Meas_Conf           = PIOS_HMC5883_MEASCONF_NORMAL,
	.Gain                = PIOS_HMC5883_GAIN_1_9,
	.Mode                = PIOS_HMC5883_MODE_SINGLE,
	.Default_Orientation = PIOS_HMC5883_TOP_0DEG,
};
#endif /* PIOS_INCLUDE_HMC5883 */

#define PIOS_COM_CAN_RX_BUF_LEN 256
#define PIOS_COM_CAN_TX_BUF_LEN 256

uintptr_t pios_com_openlog_logging_id;
uintptr_t pios_com_can_id;
uintptr_t pios_internal_adc_id = 0;
uintptr_t pios_uavo_settings_fs_id;

uintptr_t pios_can_id;

void set_vtx_channel(HwSparky2VTX_ChOptions channel)
{
	uint8_t chan = 0;
	uint8_t band = 0xFF; // Set to "A" band

	switch (channel) {
	case HWSPARKY2_VTX_CH_BOSCAMACH15725:
		chan = 0;
		band = 0;
	case HWSPARKY2_VTX_CH_BOSCAMACH25745:
		chan = 1;
		band = 0;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMACH35765:
		chan = 2;
		band = 0;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMACH45785:
		chan = 3;
		band = 0;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMACH55805:
		chan = 4;
		band = 0;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMACH65825:
		chan = 5;
		band = 0;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMACH75845:
		chan = 6;
		band = 0;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMACH85865:
		chan = 7;
		band = 0;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMBCH15733:
		chan = 0;
		band = 1;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMBCH25752:
		chan = 1;
		band = 1;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMBCH35771:
		chan = 2;
		band = 1;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMBCH45790:
		chan = 3;
		band = 1;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMBCH55809:
		chan = 4;
		band = 1;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMBCH65828:
		chan = 5;
		band = 1;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMBCH75847:
		chan = 6;
		band = 1;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMBCH85866:
		chan = 7;
		band = 1;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMECH15705:
		chan = 0;
		band = 2;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMECH25685:
		chan = 1;
		band = 2;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMECH35665:
		chan = 2;
		band = 2;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMECH45645:
		chan = 3;
		band = 2;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMECH55885:
		chan = 4;
		band = 2;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMECH65905:
		chan = 5;
		band = 2;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMECH75925:
		chan = 6;
		band = 2;
		break;
	case HWSPARKY2_VTX_CH_BOSCAMECH85945:
		chan = 7;
		band = 2;
		break;
	case HWSPARKY2_VTX_CH_AIRWAVECH15740:
		chan = 0;
		band = 3;
		break;
	case HWSPARKY2_VTX_CH_AIRWAVECH25760:
		chan = 1;
		band = 3;
		break;
	case HWSPARKY2_VTX_CH_AIRWAVECH35780:
		chan = 2;
		band = 3;
		break;
	case HWSPARKY2_VTX_CH_AIRWAVECH45800:
		chan = 3;
		band = 3;
		break;
	case HWSPARKY2_VTX_CH_AIRWAVECH55820:
		chan = 4;
		band = 3;
		break;
	case HWSPARKY2_VTX_CH_AIRWAVECH65840:
		chan = 5;
		band = 3;
		break;
	case HWSPARKY2_VTX_CH_AIRWAVECH75860:
		chan = 6;
		band = 3;
		break;
	case HWSPARKY2_VTX_CH_AIRWAVECH85860:
		chan = 7;
		band = 3;
		break;
	}

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	if (chan & 0x01) {
		GPIO_SetBits(GPIOB, GPIO_Pin_14);
	} else {
		GPIO_ResetBits(GPIOB, GPIO_Pin_14);
	}

	if (chan & 0x02) {
		GPIO_SetBits(GPIOB, GPIO_Pin_13);
	} else {
		GPIO_ResetBits(GPIOB, GPIO_Pin_13);
	}

	if (chan & 0x04) {
		GPIO_SetBits(GPIOB, GPIO_Pin_12);
	} else {
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	}

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	if (band & 0x01) {
		GPIO_SetBits(GPIOA, GPIO_Pin_9);
	} else {
		GPIO_ResetBits(GPIOA, GPIO_Pin_9);
	}

	if (band & 0x02) {
		GPIO_SetBits(GPIOA, GPIO_Pin_10);
	} else {
		GPIO_ResetBits(GPIOA, GPIO_Pin_10);
	}
}

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

#include <pios_board_info.h>

/**
 * Check the brown out reset threshold is 2.7 volts and if not
 * resets it.  This solves an issue that can prevent boards
 * powering up with some BEC
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

void PIOS_Board_Init(void) {

	check_bor();

	const struct pios_board_info * bdinfo = &pios_board_info_blob;

	// Make sure all the PWM outputs are low
	const struct pios_servo_cfg * servo_cfg = get_servo_cfg(bdinfo->board_rev);
	const struct pios_tim_channel * channels = servo_cfg->channels;
	uint8_t num_channels = servo_cfg->num_channels;
	for (int i = 0; i < num_channels; i++) {
		GPIO_Init(channels[i].pin.gpio, (GPIO_InitTypeDef*) &channels[i].pin.init);
	}

	/* Delay system */
	PIOS_DELAY_Init();

#if defined(PIOS_INCLUDE_ANNUNC)
	const struct pios_annunc_cfg * led_cfg = PIOS_BOARD_HW_DEFS_GetLedCfg(bdinfo->board_rev);
	PIOS_Assert(led_cfg);
	PIOS_ANNUNC_Init(led_cfg);
#endif	/* PIOS_INCLUDE_ANNUNC */
	
	uint32_t pios_spi_gyro_id;

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
#if defined(PIOS_INCLUDE_FLASH_JEDEC)
	if (get_external_flash(bdinfo->board_rev)) {
		if (PIOS_Flash_Jedec_Init(&pios_external_flash_id, pios_spi_telem_flash_id, 1, &flash_m25p_cfg) != 0)
			PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FLASH);
	}
#endif /* PIOS_INCLUDE_FLASH_JEDEC */

	PIOS_Flash_Internal_Init(&pios_internal_flash_id, &flash_internal_cfg);

	/* Register the partition table */
	const struct pios_flash_partition * flash_partition_table;
	uint32_t num_partitions;
	flash_partition_table = PIOS_BOARD_HW_DEFS_GetPartitionTable(bdinfo->board_rev, &num_partitions);
	PIOS_FLASH_register_partition_table(flash_partition_table, num_partitions);

	/* Mount all filesystems */
	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, get_flashfs_settings_cfg(bdinfo->board_rev), FLASH_PARTITION_LABEL_SETTINGS))
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_FILESYS);
#if defined(PIOS_INCLUDE_FLASH_JEDEC)
#endif /* PIOS_INCLUDE_FLASH_JEDEC */

#endif	/* PIOS_INCLUDE_FLASH */

	/* Initialize UAVObject libraries */
	UAVObjInitialize();

	/* Initialize the hardware UAVOs */
	HwSparky2Initialize();
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

	/* Initialize the alarms library. Reads RCC reset flags */
	AlarmsInitialize();
	PIOS_RESET_Clear(); // Clear the RCC reset flags after use.

	/* Initialize the task monitor library */
	TaskMonitorInitialize();

	/* Set up pulse timers */
	PIOS_TIM_InitClock(&tim_3_cfg);
	PIOS_TIM_InitClock(&tim_5_cfg);
	PIOS_TIM_InitClock(&tim_8_cfg);
	PIOS_TIM_InitClock(&tim_9_cfg);
	PIOS_TIM_InitClock(&tim_12_cfg);

	NVIC_InitTypeDef tim_8_up_irq = {
		.NVIC_IRQChannel                   = TIM8_UP_TIM13_IRQn,
		.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
		.NVIC_IRQChannelSubPriority        = 0,
		.NVIC_IRQChannelCmd                = ENABLE,
	};
	NVIC_Init(&tim_8_up_irq);

	/* IAP System Setup */
	PIOS_IAP_Init();
	uint16_t boot_count = PIOS_IAP_ReadBootCount();
	if (boot_count < 3) {
		PIOS_IAP_WriteBootCount(++boot_count);
		AlarmsClear(SYSTEMALARMS_ALARM_BOOTFAULT);
	} else {
		/* Too many failed boot attempts, force hw config to defaults */
		HwSparky2SetDefaults(HwSparky2Handle(), 0);
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
	HwSparky2USB_VCPPortGet(&hw_usb_vcpport);

	if (!usb_cdc_present) {
		/* Force VCP port function to disabled if we haven't advertised VCP in our USB descriptor */
		hw_usb_vcpport = HWSPARKY2_USB_VCPPORT_DISABLED;
	}

	PIOS_HAL_ConfigureCDC(hw_usb_vcpport, pios_usb_id, &pios_usb_cdc_cfg);

#endif	/* PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID)
	/* Configure the usb HID port */
	uint8_t hw_usb_hidport;
	HwSparky2USB_HIDPortGet(&hw_usb_hidport);

	if (!usb_hid_present) {
		/* Force HID port function to disabled if we haven't advertised HID in our USB descriptor */
		hw_usb_hidport = HWSPARKY2_USB_HIDPORT_DISABLED;
	}

	PIOS_HAL_ConfigureHID(hw_usb_hidport, pios_usb_id, &pios_usb_hid_cfg);

#endif	/* PIOS_INCLUDE_USB_HID */

	if (usb_hid_present || usb_cdc_present) {
		PIOS_USBHOOK_Activate();
	}
#endif	/* PIOS_INCLUDE_USB */

	/* Configure IO ports */
	HwSparky2DSMxModeOptions hw_DSMxMode;
	HwSparky2DSMxModeGet(&hw_DSMxMode);
	
	/* Configure main USART port */
	uint8_t hw_mainport;
	HwSparky2MainPortGet(&hw_mainport);

	PIOS_HAL_ConfigurePort(hw_mainport,          // port type protocol
			&pios_usart_main_cfg,                // usart_port_cfg
			&pios_usart_com_driver,              // com_driver 
			NULL,                                // i2c_id 
			NULL,                                // i2c_cfg 
			NULL,                                // i2c_cfg 
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			&pios_dsm_main_cfg,                  // dsm_cfg
			hw_DSMxMode,                         // dsm_mode 
			NULL);                               // sbus_cfg

	/* Configure FlexiPort */
	uint8_t hw_flexiport;
	HwSparky2FlexiPortGet(&hw_flexiport);

	PIOS_HAL_ConfigurePort(hw_flexiport,         // port type protocol
			&pios_usart_flexi_cfg,               // usart_port_cfg
			&pios_usart_com_driver,              // com_driver
			&pios_i2c_flexiport_adapter_id,      // i2c_id
			&pios_i2c_flexiport_adapter_cfg,     // i2c_cfg 
			NULL,                                // i2c_cfg 
			NULL,                                // pwm_cfg
			PIOS_LED_ALARM,                      // led_id
			&pios_dsm_flexi_cfg,                 // dsm_cfg
			hw_DSMxMode,                         // dsm_mode 
			NULL);                               // sbus_cfg

#if defined(PIOS_INCLUDE_RFM22B)
	HwSparky2Data hwSparky2;
	HwSparky2Get(&hwSparky2);

	const struct pios_rfm22b_cfg *rfm22b_cfg = PIOS_BOARD_HW_DEFS_GetRfm22Cfg(bdinfo->board_rev);

	const struct pios_openlrs_cfg *openlrs_cfg = PIOS_BOARD_HW_DEFS_GetOpenLRSCfg(bdinfo->board_rev);

	PIOS_HAL_ConfigureRFM22B(hwSparky2.Radio,
			bdinfo->board_type, bdinfo->board_rev,
			hwSparky2.MaxRfPower, hwSparky2.MaxRfSpeed,
			hwSparky2.RfBand,
			openlrs_cfg, rfm22b_cfg,
			hwSparky2.MinChannel, hwSparky2.MaxChannel,
			hwSparky2.CoordID, 1);

#endif /* PIOS_INCLUDE_RFM22B */

	/* Configure the receiver port*/
	uint8_t hw_rcvrport;
	HwSparky2RcvrPortGet(&hw_rcvrport);

	if (bdinfo->board_rev != BRUSHEDSPARKY_V0_2 && hw_DSMxMode >= HWSPARKY2_DSMXMODE_BIND3PULSES) {
		hw_DSMxMode = HWSPARKY2_DSMXMODE_AUTODETECT; /* Do not try to bind through XOR */
	}

	PIOS_HAL_ConfigurePort(hw_rcvrport,            // port type protocol
			get_usart_rcvr_cfg(bdinfo->board_rev), // usart_port_cfg
			&pios_usart_com_driver,                // com_driver
			NULL,                                  // i2c_id
			NULL,                                  // i2c_cfg
			&pios_ppm_cfg,                         // ppm_cfg
			NULL,                                  // pwm_cfg
			PIOS_LED_ALARM,                        // led_id
			&pios_dsm_rcvr_cfg,                    // dsm_cfg
			hw_DSMxMode,                           // dsm_mode
			get_sbus_cfg(bdinfo->board_rev));      // sbus_cfg

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
	/* Set up the servo outputs */
	PIOS_Servo_Init(&pios_servo_cfg);
#else
	PIOS_DEBUG_Init(&pios_tim_servo_all_channels, NELEMENTS(pios_tim_servo_all_channels));
#endif
	
	if (PIOS_I2C_Init(&pios_i2c_mag_pressure_adapter_id, &pios_i2c_mag_pressure_adapter_cfg)) {
		PIOS_DEBUG_Assert(0);
	}

#if defined(PIOS_INCLUDE_CAN)
	if(get_use_can(bdinfo->board_rev)) {
		if (PIOS_CAN_Init(&pios_can_id, &pios_can_cfg) != 0)
			PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_CAN);

		if (PIOS_COM_Init(&pios_com_can_id, &pios_can_com_driver, pios_can_id,
		                  PIOS_COM_CAN_RX_BUF_LEN,
		                  PIOS_COM_CAN_TX_BUF_LEN))
			PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_CAN);

		/* pios_com_bridge_id = pios_com_can_id; */
	}
#endif

	PIOS_DELAY_WaitmS(50);

	PIOS_SENSORS_Init();

#if defined(PIOS_INCLUDE_ADC)
	uint32_t internal_adc_id;
	PIOS_INTERNAL_ADC_Init(&internal_adc_id, &pios_adc_cfg);
	PIOS_ADC_Init(&pios_internal_adc_id, &pios_internal_adc_driver, internal_adc_id);
#endif

#if defined(PIOS_INCLUDE_MS5611)
	if ((PIOS_MS5611_Init(&pios_ms5611_cfg, pios_i2c_mag_pressure_adapter_id) != 0)
			|| (PIOS_MS5611_Test() != 0))
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_BARO);
#endif

	uint8_t Magnetometer;
	HwSparky2MagnetometerGet(&Magnetometer);

	if (Magnetometer != HWSPARKY2_MAGNETOMETER_INTERNAL)
		pios_mpu9250_cfg.use_magnetometer = false;


#if defined(PIOS_INCLUDE_MPU9250_SPI)
	if (PIOS_MPU9250_SPI_Init(pios_spi_gyro_id, 0, &pios_mpu9250_cfg) != 0)
		PIOS_HAL_CriticalError(PIOS_LED_ALARM, PIOS_HAL_PANIC_IMU);

	// To be safe map from UAVO enum to driver enum
	uint8_t hw_gyro_range;
	HwSparky2GyroRangeGet(&hw_gyro_range);
	switch(hw_gyro_range) {
		case HWSPARKY2_GYRORANGE_250:
			PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_250_DEG);
			break;
		case HWSPARKY2_GYRORANGE_500:
			PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_500_DEG);
			break;
		case HWSPARKY2_GYRORANGE_1000:
			PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_1000_DEG);
			break;
		case HWSPARKY2_GYRORANGE_2000:
			PIOS_MPU9250_SetGyroRange(PIOS_MPU60X0_SCALE_2000_DEG);
			break;
	}

	uint8_t hw_accel_range;
	HwSparky2AccelRangeGet(&hw_accel_range);
	switch(hw_accel_range) {
		case HWSPARKY2_ACCELRANGE_2G:
			PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_2G);
			break;
		case HWSPARKY2_ACCELRANGE_4G:
			PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_4G);
			break;
		case HWSPARKY2_ACCELRANGE_8G:
			PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_8G);
			break;
		case HWSPARKY2_ACCELRANGE_16G:
			PIOS_MPU9250_SetAccelRange(PIOS_MPU60X0_ACCEL_16G);
			break;
	}

	// the filter has to be set before rate else divisor calculation will fail
	uint8_t hw_mpu9250_dlpf;
	HwSparky2MPU9250GyroLPFGet(&hw_mpu9250_dlpf);
	enum pios_mpu9250_gyro_filter mpu9250_gyro_lpf = \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250GYROLPF_184) ? PIOS_MPU9250_GYRO_LOWPASS_184_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250GYROLPF_92) ? PIOS_MPU9250_GYRO_LOWPASS_92_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250GYROLPF_41) ? PIOS_MPU9250_GYRO_LOWPASS_41_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250GYROLPF_20) ? PIOS_MPU9250_GYRO_LOWPASS_20_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250GYROLPF_10) ? PIOS_MPU9250_GYRO_LOWPASS_10_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250GYROLPF_5) ? PIOS_MPU9250_GYRO_LOWPASS_5_HZ : \
	    pios_mpu9250_cfg.default_gyro_filter;
	PIOS_MPU9250_SetGyroLPF(mpu9250_gyro_lpf);

	HwSparky2MPU9250AccelLPFGet(&hw_mpu9250_dlpf);
	enum pios_mpu9250_accel_filter mpu9250_accel_lpf = \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250ACCELLPF_460) ? PIOS_MPU9250_ACCEL_LOWPASS_460_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250ACCELLPF_184) ? PIOS_MPU9250_ACCEL_LOWPASS_184_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250ACCELLPF_92) ? PIOS_MPU9250_ACCEL_LOWPASS_92_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250ACCELLPF_41) ? PIOS_MPU9250_ACCEL_LOWPASS_41_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250ACCELLPF_20) ? PIOS_MPU9250_ACCEL_LOWPASS_20_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250ACCELLPF_10) ? PIOS_MPU9250_ACCEL_LOWPASS_10_HZ : \
	    (hw_mpu9250_dlpf == HWSPARKY2_MPU9250ACCELLPF_5) ? PIOS_MPU9250_ACCEL_LOWPASS_5_HZ : \
	    pios_mpu9250_cfg.default_accel_filter;
	PIOS_MPU9250_SetAccelLPF(mpu9250_accel_lpf);
#endif /* PIOS_INCLUDE_MPU9250_SPI */


	PIOS_WDG_Clear();

#if defined(PIOS_INCLUDE_I2C)
	uint8_t hw_magnetometer;
	HwSparky2MagnetometerGet(&hw_magnetometer);

	switch (hw_magnetometer) {
		case HWSPARKY2_MAGNETOMETER_NONE:
		case HWSPARKY2_MAGNETOMETER_INTERNAL:
			/* internal mag is handled by MPU code above */
			break;

		/* default external mags and handle them in PiOS HAL rather than maintaining list here */
		default:
		{
			HwSparky2ExtMagPortOptions hw_mag_port;
			HwSparky2ExtMagPortGet(&hw_mag_port);

			uint32_t i2c_mag_id = 0; /* TODO change to a real pointer */
			const void *i2c_mag_cfg = NULL;
			switch (hw_mag_port) {
			case HWSPARKY2_EXTMAGPORT_FLEXIPORT:
				if (hw_flexiport == HWSHARED_PORTTYPES_I2C) {
					i2c_mag_id = pios_i2c_flexiport_adapter_id;
					i2c_mag_cfg = &pios_i2c_flexiport_adapter_cfg;
				}
				break;
			case HWSPARKY2_EXTMAGPORT_AUXI2C:
				i2c_mag_id = pios_i2c_mag_pressure_adapter_id;
				i2c_mag_cfg = &pios_i2c_mag_pressure_adapter_cfg;
				break;
			}

			if (i2c_mag_id && i2c_mag_cfg) {
				uint8_t hw_orientation;
				HwSparky2ExtMagOrientationGet(&hw_orientation);

				PIOS_HAL_ConfigureExternalMag(hw_magnetometer, hw_orientation,
					&i2c_mag_id, i2c_mag_cfg);
			} else {
				PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
			}
		}
			break;
	}

	/* I2C is slow, sensor init as well, reset watchdog to prevent reset here */
	PIOS_WDG_Clear();

#endif /* PIOS_INCLUDE_I2C */

	switch (bdinfo->board_rev) {
	case BRUSHEDSPARKY_V0_2:
		{
			HwSparky2VTX_ChOptions channel;
			HwSparky2VTX_ChGet(&channel);
			set_vtx_channel(channel);
		}
		break;
	}

}

/**
 * @}
 * @}
 */

