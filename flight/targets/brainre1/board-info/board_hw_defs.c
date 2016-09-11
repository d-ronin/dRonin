/**
 ******************************************************************************
 * @addtogroup dRonin Targets
 * @{
 * @addtogroup BrainRE1 support files
 * @{
 *
 * @file       board_hw_defs.c 
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     BrainFPV, Copyright (C) 2015
 * @brief      Defines board specific static initializers for hardware for the
 *             BrainFPV RE1 board.
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
 
#include <pios_config.h>
#include <pios_board_info.h>

#if defined(PIOS_INCLUDE_LED)

#include <pios_led_priv.h>
static const struct pios_led pios_leds[] = {
	[PIOS_LED_HEARTBEAT] = {
		.pin = {
			.gpio = GPIOB,
			.init = {
				.GPIO_Pin   = GPIO_Pin_13,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_DOWN
			},
		},
		.remap = 0,
		.active_high = true,
	},
	[PIOS_LED_ALARM] = {
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_8,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_DOWN
			},
		},
		.remap = 0,
		.active_high = true,
	}
};

static const struct pios_led_cfg pios_led_cfg = {
	.leds     = pios_leds,
	.num_leds = NELEMENTS(pios_leds),
};

const struct pios_led_cfg * PIOS_BOARD_HW_DEFS_GetLedCfg (uint32_t board_revision)
{
	return &pios_led_cfg;
}
#endif	/* PIOS_INCLUDE_LED */


#if defined(PIOS_INCLUDE_FLASH)
#include "pios_flashfs_logfs_priv.h"

static const struct flashfs_logfs_cfg flashfs_settings_cfg = {
	.fs_magic      = 0x3bb141cf,
	.arena_size    = 0x00010000, /* 256 * slot size */
	.slot_size     = 0x00000100, /* 256 bytes */
};

static const struct flashfs_logfs_cfg flashfs_waypoints_cfg = {
	.fs_magic      = 0x9a365a64,
	.arena_size    = 0x00010000, /* 2048 * slot size */
	.slot_size     = 0x00000040, /* 64 bytes */
};

#if defined(PIOS_INCLUDE_FLASH_JEDEC)
#include "pios_flash_jedec_priv.h"

static const struct pios_flash_jedec_cfg flash_s25fl127_cfg = {
	.expect_manufacturer = JEDEC_MANUFACTURER_SPANSION,
	.expect_memorytype   = 0x20,
	.expect_capacity     = 0x18,
	.sector_erase        = 0xd8,
};
#endif	/* PIOS_INCLUDE_FLASH_JEDEC */

#if defined(PIOS_INCLUDE_FLASH_INTERNAL)
#include "pios_flash_internal_priv.h"

static const struct pios_flash_internal_cfg flash_internal_cfg = {
};
#endif	/* PIOS_INCLUDE_FLASH_INTERNAL */

#include "pios_flash_priv.h"

#if defined(PIOS_INCLUDE_FLASH_INTERNAL)
static const struct pios_flash_sector_range stm32f4_sectors[] = {
	{
		.base_sector = 0,
		.last_sector = 3,
		.sector_size = FLASH_SECTOR_16KB,
	},
	{
		.base_sector = 4,
		.last_sector = 4,
		.sector_size = FLASH_SECTOR_64KB,
	},
	{
		.base_sector = 5,
		.last_sector = 7,
		.sector_size = FLASH_SECTOR_128KB,
	},

};

uintptr_t pios_internal_flash_id;
static const struct pios_flash_chip pios_flash_chip_internal = {
	.driver        = &pios_internal_flash_driver,
	.chip_id       = &pios_internal_flash_id,
	.page_size     = 16, /* 128-bit rows */
	.sector_blocks = stm32f4_sectors,
	.num_blocks    = NELEMENTS(stm32f4_sectors),
};
#endif	/* PIOS_INCLUDE_FLASH_INTERNAL */

#if defined(PIOS_INCLUDE_FLASH_JEDEC)
static const struct pios_flash_sector_range s25fl127_sectors[] = {
	{
		.base_sector = 0,
		.last_sector = 15,
		.sector_size = FLASH_SECTOR_4KB,
	},
	// NOTE: The 4kB sectors are currently unused, as they need a different erase command
	{
		.base_sector = 16,
		.last_sector = 270,
		.sector_size = FLASH_SECTOR_64KB,
	}
};

uintptr_t pios_external_flash_id;
static const struct pios_flash_chip pios_flash_chip_external = {
	.driver        = &pios_jedec_flash_driver,
	.chip_id       = &pios_external_flash_id,
	.page_size     = 256,
	.sector_blocks = s25fl127_sectors,
	.num_blocks    = NELEMENTS(s25fl127_sectors),
};
#endif /* PIOS_INCLUDE_FLASH_JEDEC */

static const struct pios_flash_partition pios_flash_partition_table[] = {
#if defined(PIOS_INCLUDE_FLASH_INTERNAL)
	{
		.label        = FLASH_PARTITION_LABEL_BL,
		.chip_desc    = &pios_flash_chip_internal,
		.first_sector = 0,
		.last_sector  = 1,
		.chip_offset  = 0,
		.size         = (1 - 0 + 1) * FLASH_SECTOR_16KB,
	},

	/* NOTE: sectors 2-4 of the internal flash are currently unallocated */

	{
		.label        = FLASH_PARTITION_LABEL_FW,
		.chip_desc    = &pios_flash_chip_internal,
		.first_sector = 5,
		.last_sector  = 7,
		.chip_offset  = (4 * FLASH_SECTOR_16KB) + (1 * FLASH_SECTOR_64KB),
		.size         = (7 - 5 + 1) * FLASH_SECTOR_128KB,
	},

#endif /* PIOS_INCLUDE_FLASH_INTERNAL */

#if defined(PIOS_INCLUDE_FLASH_JEDEC)
	{
		.label        = FLASH_PARTITION_LABEL_SETTINGS,
		.chip_desc    = &pios_flash_chip_external,
		.first_sector = 16,
		.last_sector  = 18,
		.chip_offset  = (16 * FLASH_SECTOR_4KB),
		.size         = (18 - 16 + 1) * FLASH_SECTOR_64KB,
	},

	{
		.label        = FLASH_PARTITION_LABEL_WAYPOINTS,
		.chip_desc    = &pios_flash_chip_external,
		.first_sector = 19,
		.last_sector  = 20,
		.chip_offset  = (16 * FLASH_SECTOR_4KB) + (3 * FLASH_SECTOR_64KB),
		.size         = (20 - 19 + 1) * FLASH_SECTOR_64KB,
	},

	{
		.label        = FLASH_PARTITION_LABEL_LOG,
		.chip_desc    = &pios_flash_chip_external,
		.first_sector = 21,
		.last_sector  = 270,
		.chip_offset  = (16 * FLASH_SECTOR_4KB) + (5 * FLASH_SECTOR_64KB),
		.size         = (270 - 21 + 1) * FLASH_SECTOR_64KB,
	},
#endif	/* PIOS_INCLUDE_FLASH_JEDEC */
};

const struct pios_flash_partition * PIOS_BOARD_HW_DEFS_GetPartitionTable (uint32_t board_revision, uint32_t * num_partitions)
{
	PIOS_Assert(num_partitions);

	*num_partitions = NELEMENTS(pios_flash_partition_table);
	return pios_flash_partition_table;
}


#endif	/* PIOS_INCLUDE_FLASH */


#if defined(PIOS_INCLUDE_SPI)
#include <pios_spi_priv.h>

/*
 * SPI1 Interface
 * Used for BMI160 gyro / accelerometer
 */
static const struct pios_spi_cfg pios_spi_gyro_cfg = {
	.regs = SPI1,
	.remap = GPIO_AF_SPI1,
	.init   = {
		.SPI_Mode              = SPI_Mode_Master,
		.SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
		.SPI_DataSize          = SPI_DataSize_8b,
		.SPI_NSS               = SPI_NSS_Soft,
		.SPI_FirstBit          = SPI_FirstBit_MSB,
		.SPI_CRCPolynomial     = 7,
		.SPI_CPOL              = SPI_CPOL_High,
		.SPI_CPHA              = SPI_CPHA_2Edge,
		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16, // 90MHz / 16 = 5.6MHz
	},
	.sclk = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_5,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_UP
		},
	},
	.miso = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_6,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_UP
		},
	},
	.mosi = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_7,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_UP
		},
	},
	.slave_count = 1,
	.ssel = { {
				  .gpio = GPIOC,
				  .init = {
					  .GPIO_Pin   = GPIO_Pin_14,
					  .GPIO_Speed = GPIO_Speed_2MHz,
					  .GPIO_Mode  = GPIO_Mode_OUT,
					  .GPIO_OType = GPIO_OType_PP,
					  .GPIO_PuPd = GPIO_PuPd_UP
				  } }
			}
};

/* SPI3 Interface
 *      - Used for flash and RE1 FPGA communication
 */
static const struct pios_spi_cfg pios_spi_flash_cfg = {
	.regs = SPI3,
	.remap = GPIO_AF_SPI3,
	.init = {
		.SPI_Mode              = SPI_Mode_Master,
		.SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
		.SPI_DataSize          = SPI_DataSize_8b,
		.SPI_NSS               = SPI_NSS_Soft,
		.SPI_FirstBit          = SPI_FirstBit_MSB,
		.SPI_CRCPolynomial     = 7,
		.SPI_CPOL              = SPI_CPOL_High,
		.SPI_CPHA              = SPI_CPHA_2Edge,
		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8,	//@ APB1 PCLK1 45MHz / 8 == 5.6MHz
	},
	.sclk = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin = GPIO_Pin_3,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
		.pin_source = GPIO_PinSource3,
	},
	.miso = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin = GPIO_Pin_11,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
		.pin_source = GPIO_PinSource11,
	},
	.mosi = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin = GPIO_Pin_12,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
		.pin_source = GPIO_PinSource12,
	},
	.slave_count = 2,
	.ssel = {
		{
			.gpio = GPIOB,
			.init = {
				.GPIO_Pin = GPIO_Pin_15,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource15,
		},
		{
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin = GPIO_Pin_15,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource15,
		},
	},

};

uint32_t pios_spi_flash_id;

#endif	/* PIOS_INCLUDE_SPI */

#if defined(PIOS_INCLUDE_I2C)
#include <pios_i2c_priv.h>

/*
 * I2C Adapters
 */
void PIOS_I2C_internal_ev_irq_handler(void);
void PIOS_I2C_internal_er_irq_handler(void);
void I2C1_EV_IRQHandler() __attribute__ ((alias ("PIOS_I2C_external_ev_irq_handler")));
void I2C1_ER_IRQHandler() __attribute__ ((alias ("PIOS_I2C_external_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_external_cfg = {
	.regs = I2C1,
	.remap = GPIO_AF_I2C1,
	.init = {
		.I2C_Mode                = I2C_Mode_I2C,
		.I2C_OwnAddress1         = 0,
		.I2C_Ack                 = I2C_Ack_Enable,
		.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit,
		.I2C_DutyCycle           = I2C_DutyCycle_2,
		.I2C_ClockSpeed          = 400000,	/* bits/s */
	},
	.transfer_timeout_ms = 50,
	.scl = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin = GPIO_Pin_8,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL,
		},
		.pin_source = GPIO_PinSource8,
	},
	.sda = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin = GPIO_Pin_9,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL,
		},
		.pin_source = GPIO_PinSource9,
	},
	.event = {
		.flags   = 0,
		.init = {
			.NVIC_IRQChannel = I2C1_EV_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.error = {
		.flags   = 0,
		.init = {
			.NVIC_IRQChannel = I2C1_ER_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
};

uint32_t pios_i2c_external_id;
void PIOS_I2C_external_ev_irq_handler(void)
{
	/* Call into the generic code to handle the IRQ for this specific device */
	PIOS_I2C_EV_IRQ_Handler(pios_i2c_external_id);
}

void PIOS_I2C_external_er_irq_handler(void)
{
	/* Call into the generic code to handle the IRQ for this specific device */
	PIOS_I2C_ER_IRQ_Handler(pios_i2c_external_id);
}
#endif /* PIOS_INCLUDE_I2C */

#if defined(PIOS_INCLUDE_COM)
#include "pios_com_priv.h"
#endif	/* PIOS_INCLUDE_COM */

#if defined(PIOS_INCLUDE_GCSRCVR)
#include "pios_gcsrcvr_priv.h"
#endif	/* PIOS_INCLUDE_GCSRCVR */


#if defined(PIOS_INCLUDE_RCVR)
#include "pios_rcvr_priv.h"
#endif /* PIOS_INCLUDE_RCVR */


#if defined(PIOS_INCLUDE_USB)
#include "pios_usb_priv.h"

static const struct pios_usb_cfg pios_usb_main_cfg = {
	.irq = {
		.init    = {
			.NVIC_IRQChannel                   = OTG_FS_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.vsense = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_9,
			.GPIO_Speed = GPIO_Speed_25MHz,
			.GPIO_Mode  = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL,
		},
	}
};

#include "pios_usb_board_data_priv.h"
#include "pios_usb_desc_hid_cdc_priv.h"
#include "pios_usb_desc_hid_only_priv.h"
#include "pios_usbhook.h"

#endif	/* PIOS_INCLUDE_USB */

#if defined(PIOS_INCLUDE_COM_MSG)

#include <pios_com_msg_priv.h>

#endif /* PIOS_INCLUDE_COM_MSG */

#if defined(PIOS_INCLUDE_USB_HID) && !defined(PIOS_INCLUDE_USB_CDC)
#include <pios_usb_hid_priv.h>

const struct pios_usb_hid_cfg pios_usb_hid_cfg = {
	.data_if = 0,
	.data_rx_ep = 1,
	.data_tx_ep = 1,
};
#endif /* PIOS_INCLUDE_USB_HID && !PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_USB_HID) && defined(PIOS_INCLUDE_USB_CDC)
#include <pios_usb_cdc_priv.h>

const struct pios_usb_cdc_cfg pios_usb_cdc_cfg = {
	.ctrl_if = 0,
	.ctrl_tx_ep = 2,

	.data_if = 1,
	.data_rx_ep = 3,
	.data_tx_ep = 3,
};

#include <pios_usb_hid_priv.h>

const struct pios_usb_hid_cfg pios_usb_hid_cfg = {
	.data_if = 2,
	.data_rx_ep = 1,
	.data_tx_ep = 1,
};
#endif	/* PIOS_INCLUDE_USB_HID && PIOS_INCLUDE_USB_CDC */

#if defined(PIOS_INCLUDE_ADC)
#include "pios_adc_priv.h"
#include "pios_internal_adc_priv.h"

void PIOS_ADC_DMA_irq_handler(void);
void DMA2_Stream4_IRQHandler(void) __attribute__((alias("PIOS_ADC_DMA_irq_handler")));
struct pios_internal_adc_cfg pios_adc_cfg = {
	.adc_dev_master = ADC1,
	.dma = {
		.irq = {
			.flags = (DMA_FLAG_TCIF4 | DMA_FLAG_TEIF4 | DMA_FLAG_HTIF4),
			.init = {
				.NVIC_IRQChannel = DMA2_Stream4_IRQn,
				.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
				.NVIC_IRQChannelSubPriority = 0,
				.NVIC_IRQChannelCmd = ENABLE,
			},
		},
		.rx = {
			.channel = DMA2_Stream4,
			.init = {
				.DMA_Channel = DMA_Channel_0,
				.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR
			},
		}
	},
	.half_flag = DMA_IT_HTIF4,
	.full_flag = DMA_IT_TCIF4,
	.adc_pins = {                                                                             \
		{ GPIOC, GPIO_Pin_1, ADC_Channel_11 },                                                \
		{ GPIOC, GPIO_Pin_0, ADC_Channel_10 },                                                \
		{ GPIOC, GPIO_Pin_3, ADC_Channel_13 }                                                 \
	},
	.adc_pin_count = 3
};

void PIOS_ADC_DMA_irq_handler(void)
{
	/* Call into the generic code to handle the IRQ for this specific device */
	PIOS_INTERNAL_ADC_DMA_Handler();
}

#endif /* PIOS_INCLUDE_ADC */


#if defined(PIOS_INCLUDE_RTC)
/*
 * Realtime Clock (RTC)
 */
#include <pios_rtc_priv.h>

void PIOS_RTC_IRQ_Handler (void);
void RTC_WKUP_IRQHandler() __attribute__ ((alias ("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
	// FIXME for real board the clock is 16MHz
	.clksrc = RCC_RTCCLKSource_HSE_Div16, // Divide 8 Mhz crystal down to 1
	// For some reason it's acting like crystal is 16 Mhz.  This clock is then divided
	// by another 16 to give a nominal 62.5 khz clock
	.prescaler = 100, // Every 100 cycles gives 625 Hz
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = RTC_WKUP_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

void PIOS_RTC_IRQ_Handler (void)
{
	PIOS_RTC_irq_handler ();
}

#endif /* PIOS_INCLUDE_RTC */

#if defined(PIOS_INCLUDE_USART)

#include "pios_usart_priv.h"

static const struct pios_usart_cfg pios_usart1_cfg = {
	.regs = USART1,
	.remap = GPIO_AF_USART1,
	.irq = {
		.init = {
			.NVIC_IRQChannel = USART1_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.rx = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_7,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource7,
	},
	.tx = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_6,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource6,
	},
};

static const struct pios_usart_cfg pios_usart3_cfg = {
	.regs = USART3,
	.remap = GPIO_AF_USART3,
	.irq = {
		.init = {
			.NVIC_IRQChannel = USART3_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.rx = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_5,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource5,
	}
};

static const struct pios_usart_cfg pios_usart6_cfg = {
	.regs = USART6,
	.remap = GPIO_AF_USART6,
	.irq = {
		.init = {
			.NVIC_IRQChannel = USART6_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.rx = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_7,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource7,
	},
	.tx = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_6,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource6,
	},
};

#endif /* defined(PIOS_INCLUDE_USART) */

#if defined(PIOS_INCLUDE_PPM)
/*
 * PPM Input
 */
// Timer 12 used for PPM input
#include "pios_tim_priv.h"
#include <pios_ppm_priv.h>

static const TIM_TimeBaseInitTypeDef tim_12_time_base = {
	.TIM_Prescaler = (PIOS_PERIPHERAL_APB1_COUNTER_CLOCK / 1000000) - 1,
	.TIM_ClockDivision = TIM_CKD_DIV1,
	.TIM_CounterMode = TIM_CounterMode_Up,
	.TIM_Period = 0xFFFF,
	.TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_12_cfg = {
	.timer = TIM12,
	.time_base_init = &tim_12_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM8_BRK_TIM12_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_channel pios_tim_ppm_channel[] = {
	{
		.timer = TIM12,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_TIM12,
		.pin = {
			.gpio = GPIOB,
			.init = {
				.GPIO_Pin = GPIO_Pin_14,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource14,
		},
	},
};

#include <pios_ppm_priv.h>
static const struct pios_ppm_cfg pios_ppm_cfg = {
	.tim_ic_init = {
		.TIM_ICPolarity = TIM_ICPolarity_Rising,
		.TIM_ICSelection = TIM_ICSelection_DirectTI,
		.TIM_ICPrescaler = TIM_ICPSC_DIV1,
		.TIM_ICFilter = 0x0,
		.TIM_Channel = TIM_Channel_3,
	},
	.channels = &pios_tim_ppm_channel[0],
	.num_channels = 1,
};
#endif /* PIOS_INCLUDE_PPM */

#if defined(PIOS_INCLUDE_DSM)
/*
 * Spektrum/JR DSM USART
 */
#include <pios_dsm_priv.h>

static const struct pios_dsm_cfg pios_rxport_dsm_cfg = {
	.bind = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_14,  // same pin as PPM input
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	},
};
#endif /* defined(PIOS_INCLUDE_DSM) */

#if defined(PIOS_INCLUDE_SERVO) && defined(PIOS_INCLUDE_TIM)

// TIme base for timers on APB1 (45MHz)
static const TIM_TimeBaseInitTypeDef tim_apb1_time_base = {
	.TIM_Prescaler = (PIOS_PERIPHERAL_APB1_COUNTER_CLOCK / 1000000) - 1,
	.TIM_ClockDivision = TIM_CKD_DIV1,
	.TIM_CounterMode = TIM_CounterMode_Up,
	.TIM_Period = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
	.TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_5_cfg = {
	.timer = TIM5,
	.time_base_init = &tim_apb1_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM5_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_clock_cfg tim_8_cfg = {
	.timer = TIM8,
	.time_base_init = &tim_apb1_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM8_UP_TIM13_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

// TIme base for timers on APB2 (90MHz)
static const TIM_TimeBaseInitTypeDef tim_apb2_time_base = {
	.TIM_Prescaler = (PIOS_PERIPHERAL_APB2_COUNTER_CLOCK / 1000000) - 1,
	.TIM_ClockDivision = TIM_CKD_DIV1,
	.TIM_CounterMode = TIM_CounterMode_Up,
	.TIM_Period = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
	.TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_1_cfg = {
	.timer = TIM1,
	.time_base_init = &tim_apb2_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM1_UP_TIM10_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

/*
 *  OUTPUTS
	1: TIM5_CH1 (PA0)
	2: TIM5_CH2 (PA1)
	3: TIM5_CH3 (PA2)
	4: TIM5_CH4 (PA3)
	5: TIM1_CH3 (PA10)
	6: TIM2_CH1 (PA8)
	7: TIM8_CH1 (PC6)
	8: TIM8_CH2 (PC7)
 */

static const struct pios_tim_channel pios_tim_servoport_all_pins[] = {
	{
		.timer = TIM5,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_TIM5,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin = GPIO_Pin_0,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource0,
		},
	},
	{
		.timer = TIM5,
		.timer_chan = TIM_Channel_2,
		.remap = GPIO_AF_TIM5,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin = GPIO_Pin_1,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource1,
		},
	},
	{
		.timer = TIM5,
		.timer_chan = TIM_Channel_3,
		.remap = GPIO_AF_TIM5,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin = GPIO_Pin_2,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource2,
		},
	},
	{
		.timer = TIM5,
		.timer_chan = TIM_Channel_4,
		.remap = GPIO_AF_TIM5,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin = GPIO_Pin_3,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource3,
		},
	},
	{
		.timer = TIM1,
		.timer_chan = TIM_Channel_3,
		.remap = GPIO_AF_TIM1,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin = GPIO_Pin_10,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource10,
		},
	},
	{
		.timer = TIM2,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_TIM2,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin = GPIO_Pin_15,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource15,
		},
	},
	{
		.timer = TIM8,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_TIM8,
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin = GPIO_Pin_6,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource6,
		},
	},
	{
		.timer = TIM8,
		.timer_chan = TIM_Channel_2,
		.remap = GPIO_AF_TIM8,
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin = GPIO_Pin_7,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source = GPIO_PinSource7,
		},
	}
};

/*
 * Servo outputs
 */
#include <pios_servo_priv.h>

const struct pios_servo_cfg pios_servo_cfg = {
	.tim_oc_init = {
		.TIM_OCMode = TIM_OCMode_PWM1,
		.TIM_OutputState = TIM_OutputState_Enable,
		.TIM_OutputNState = TIM_OutputNState_Disable,
		.TIM_Pulse = PIOS_SERVOS_INITIAL_POSITION,
		.TIM_OCPolarity = TIM_OCPolarity_High,
		.TIM_OCNPolarity = TIM_OCPolarity_High,
		.TIM_OCIdleState = TIM_OCIdleState_Reset,
		.TIM_OCNIdleState = TIM_OCNIdleState_Reset,
	},
	.channels = pios_tim_servoport_all_pins,
	.num_channels = NELEMENTS(pios_tim_servoport_all_pins) - 2,
};

const struct pios_servo_cfg pios_servo_all_cfg = {
	.tim_oc_init = {
		.TIM_OCMode = TIM_OCMode_PWM1,
		.TIM_OutputState = TIM_OutputState_Enable,
		.TIM_OutputNState = TIM_OutputNState_Disable,
		.TIM_Pulse = PIOS_SERVOS_INITIAL_POSITION,
		.TIM_OCPolarity = TIM_OCPolarity_High,
		.TIM_OCNPolarity = TIM_OCPolarity_High,
		.TIM_OCIdleState = TIM_OCIdleState_Reset,
		.TIM_OCNIdleState = TIM_OCNIdleState_Reset,
	},
	.channels = pios_tim_servoport_all_pins,
	.num_channels = NELEMENTS(pios_tim_servoport_all_pins),
};
#endif /* defined(PIOS_INCLUDE_SERVO) && defined(PIOS_INCLUDE_TIM) */


#if defined(PIOS_INCLUDE_FRSKY_RSSI)
#include "pios_frsky_rssi_priv.h"
const TIM_TimeBaseInitTypeDef pios_frsky_rssi_time_base ={
	.TIM_Prescaler = 6,
	.TIM_ClockDivision = TIM_CKD_DIV1,
	.TIM_CounterMode = TIM_CounterMode_Up,
	.TIM_Period = 0xFFFF,
	.TIM_RepetitionCounter = 0x0000,
};


const struct pios_frsky_rssi_cfg pios_frsky_rssi_cfg = {
	.clock_cfg = {
		.timer = TIM8,
		.time_base_init = &pios_frsky_rssi_time_base,
	},
	.channels = {
		{
			.timer = TIM8,
			.timer_chan = TIM_Channel_2,
			.pin   = {
				.gpio = GPIOC,
				.init = {
					.GPIO_Pin   = GPIO_Pin_7,
					.GPIO_Speed = GPIO_Speed_100MHz,
					.GPIO_Mode  = GPIO_Mode_AF,
					.GPIO_OType = GPIO_OType_PP,
					.GPIO_PuPd  = GPIO_PuPd_UP
				},
				.pin_source = GPIO_PinSource7,
			},
			.remap = GPIO_AF_TIM8,
		},
		{
			.timer = TIM8,
			.timer_chan = TIM_Channel_1,
			.pin   = {
				.gpio = GPIOC,
				.init = {
					.GPIO_Pin   = GPIO_Pin_7,
					.GPIO_Speed = GPIO_Speed_100MHz,
					.GPIO_Mode  = GPIO_Mode_AF,
					.GPIO_OType = GPIO_OType_PP,
					.GPIO_PuPd  = GPIO_PuPd_UP
				},
				.pin_source = GPIO_PinSource7,
			},
			.remap = GPIO_AF_TIM8,
		},
	},
	.ic2 = {
		.TIM_ICPolarity = TIM_ICPolarity_Falling,
		.TIM_ICSelection = TIM_ICSelection_IndirectTI,
		.TIM_ICPrescaler = TIM_ICPSC_DIV1,
		.TIM_ICFilter = 0x0,
	},
	.ic1 = {
		.TIM_ICPolarity = TIM_ICPolarity_Rising,
		.TIM_ICSelection = TIM_ICSelection_DirectTI,
		.TIM_ICPrescaler = TIM_ICPSC_DIV1,
		.TIM_ICFilter = 0x0,
	}
};

#endif /* PIOS_INCLUDE_FRSKY_RSSI */


#if defined(PIOS_INCLUDE_VIDEO_QUADSPI)
#include <pios_video.h>
#include "fpga_drv.h"

static const struct pios_exti_cfg pios_exti_vsync_cfg __exti_config = {
	.vector = PIOS_Vsync_ISR,
	.line   = EXTI_Line5,
	.pin    = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_5,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL,
		},
	},
	.irq                                       = {
		.init                                  = {
			.NVIC_IRQChannel    =  EXTI9_5_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.exti                                      = {
		.init                                  = {
			.EXTI_Line    = EXTI_Line5, // matches above GPIO pin
			.EXTI_Mode    = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Falling,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

static const struct pios_exti_cfg pios_exti_hsync_cfg __exti_config = {
	.vector = PIOS_Hsync_ISR,
	.line   = EXTI_Line2,
	.pin    = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_2,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL,
		},
	},
	.irq                                       = {
		.init                                  = {
			.NVIC_IRQChannel    =  EXTI2_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.exti                                      = {
		.init                                  = {
			.EXTI_Line    = EXTI_Line2, // matches above GPIO pin
			.EXTI_Mode    = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Falling,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

const struct pios_video_cfg pios_video_cfg = {
	.qspi_init = {
		.QSPI_SShift     = QSPI_SShift_NoShift,
		.QSPI_Prescaler  = 12,  // 180MHz / 12 = 15MHz
		.QSPI_CKMode     = QSPI_CKMode_Mode0,
		.QSPI_CSHTime    = QSPI_CSHTime_1Cycle,
		.QSPI_FSize      = 0x1F,
		.QSPI_FSelect    = QSPI_FSelect_1,
		.QSPI_DFlash     = QSPI_DFlash_Disable,
	},
	.pixel_dma = DMA2,
	.dma     = {
		.irq                                       = {
			.flags = (DMA_IT_TCIF7),
			.init  = {
				.NVIC_IRQChannel    = DMA2_Stream7_IRQn,
				.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
				.NVIC_IRQChannelSubPriority        = 0,
				.NVIC_IRQChannelCmd = ENABLE,
			},
		},
		/*.rx = {},*/
		.tx                                        = {
			.channel = DMA2_Stream7,
			.init    = {
				.DMA_Channel            = DMA_Channel_3,
				.DMA_PeripheralBaseAddr = (uint32_t)&(QUADSPI->DR),
				.DMA_DIR                = DMA_DIR_MemoryToPeripheral,
				.DMA_BufferSize         = 400,
				.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
				.DMA_MemoryInc          = DMA_MemoryInc_Enable,
				.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
				.DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
				.DMA_Mode               = DMA_Mode_Normal,
				.DMA_Priority           = DMA_Priority_VeryHigh,
				.DMA_FIFOMode           = DMA_FIFOMode_Enable,
				.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
				.DMA_MemoryBurst        = DMA_MemoryBurst_INC4,
				.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
			},
		},
	},
	.sclk                                          = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_2,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	},
	.bk1_io0                                       = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_9,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	},
	.bk1_io1                                       = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_10,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	},

	.vsync = &pios_exti_vsync_cfg,
	.hsync = &pios_exti_hsync_cfg,
	.set_bw_levels = PIOS_RE1FPGA_SetBwLevels,
	.set_x_offset = PIOS_RE1FPGA_SetXOffset,
	.set_x_scale = PIOS_RE1FPGA_SetXScale,
	.set_3d_config = PIOS_RE1FPGA_Set3DConfig,
};
#endif /* if defined(PIOS_INCLUDE_VIDEO) */

#if defined(PIOS_INCLUDE_RE1_FPGA)
#include "fpga_drv.h"

const struct pios_re1fpga_cfg pios_re1fpga_cfg = {
	.mco_pin = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_8,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
	},
	.rst_pin = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_10,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_DOWN
		},
	},
	.cdone_pin = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_0,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
	},
	.cresetb_pin = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_1,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
	},
};
#endif /* defined(PIOS_INCLUDE_RE1_FPGA) */

/**
 * @}
 * @}
 */

