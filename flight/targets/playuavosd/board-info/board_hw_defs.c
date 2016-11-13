/**
 ******************************************************************************
 * @file       playuavosd/board-info/board_hw_defs.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup PlayUavOsd
 * @{
 * @brief Board support for PlayUAVOSD board
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

#if defined(PIOS_INCLUDE_ANNUNC)

#include <pios_annunc_priv.h>

static const struct pios_annunc pios_annuncs[] = {
	[PIOS_LED_HEARTBEAT] = {
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_4,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_UP
			},
		},
	},
	[PIOS_LED_ALARM] = {
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_5,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_UP
			},
		},
	},
};

static const struct pios_annunc_cfg pios_annunc_cfg = {
	.annunciators     = pios_annuncs,
	.num_annunciators = NELEMENTS(pios_annuncs),
};

const struct pios_annunc_cfg * PIOS_BOARD_HW_DEFS_GetLedCfg (uint32_t board_revision)
{
	(void)board_revision;
	return &pios_annunc_cfg;
}

#endif	/* PIOS_INCLUDE_ANNUNC */


#if defined(PIOS_INCLUDE_FLASH)
#include "pios_flashfs_logfs_priv.h"

static const struct flashfs_logfs_cfg flashfs_settings_internal_cfg = {
	.fs_magic      = 0x99abcedf,
	.arena_size    = 0x00004000, /* 256 * slot size */
	.slot_size     = 0x00000100, /* 256 bytes */
};

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
		.last_sector = 11,
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

static const struct pios_flash_partition pios_flash_partition_table_internal[] = {
#if defined(PIOS_INCLUDE_FLASH_INTERNAL)
	{
		.label        = FLASH_PARTITION_LABEL_BL,
		.chip_desc    = &pios_flash_chip_internal,
		.first_sector = 0,
		.last_sector  = 1,
		.chip_offset  = 0,
		.size         = (1 - 0 + 1) * FLASH_SECTOR_16KB,
	},

	{
		.label        = FLASH_PARTITION_LABEL_SETTINGS,
		.chip_desc    = &pios_flash_chip_internal,
		.first_sector = 2,
		.last_sector  = 3,
		.chip_offset  = (2 * FLASH_SECTOR_16KB),
		.size         = (3 - 2 + 1) * FLASH_SECTOR_16KB,
	},

	{
		.label        = FLASH_PARTITION_LABEL_FW,
		.chip_desc    = &pios_flash_chip_internal,
		.first_sector = 5,
		.last_sector  = 7,
		.chip_offset  = (4 * FLASH_SECTOR_16KB) + (1 * FLASH_SECTOR_64KB),
		.size         = (7 - 5 + 1) * FLASH_SECTOR_128KB,
	},

	/* NOTE: sectors 8-11 of the internal flash are currently unallocated */

#endif /* PIOS_INCLUDE_FLASH_INTERNAL */
};

//! Get the partition table
const struct pios_flash_partition *PIOS_BOARD_HW_DEFS_GetPartitionTable (uint32_t board_revision, uint32_t *num_partitions)
{
	(void)board_revision;
	PIOS_Assert(num_partitions);
	*num_partitions = NELEMENTS(pios_flash_partition_table_internal);
	return pios_flash_partition_table_internal;
}

#endif	/* PIOS_INCLUDE_FLASH */

#include <pios_usart_priv.h>

#ifdef PIOS_INCLUDE_COM_TELEM

/*
 * MAIN USART
 */
static const struct pios_usart_cfg pios_usart_main_cfg = {
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
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_11,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
	},
	.tx = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_10,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
	},
};
#endif /* PIOS_INCLUDE_COM_TELEM */


#if defined(PIOS_INCLUDE_COM)

#include <pios_com_priv.h>

#endif /* PIOS_INCLUDE_COM */


#if defined(PIOS_INCLUDE_RTC)
/*
 * Realtime Clock (RTC)
 */
#include <pios_rtc_priv.h>

void PIOS_RTC_IRQ_Handler (void);
void RTC_WKUP_IRQHandler() __attribute__ ((alias ("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
	.clksrc = RCC_RTCCLKSource_HSE_Div8, // Divide 8 Mhz crystal down to 1
	// This clock is then divided by another 16 to give a nominal 62.5 khz clock
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

#endif

#include "pios_tim_priv.h"

// Set up timers on APB1
// TIM2,3,4,5,6,7,12,13,14
static const TIM_TimeBaseInitTypeDef tim_apb1_time_base = {
	.TIM_Prescaler = (PIOS_PERIPHERAL_APB1_COUNTER_CLOCK / 1000000) - 1,
	.TIM_ClockDivision = TIM_CKD_DIV1,
	.TIM_CounterMode = TIM_CounterMode_Up,
	.TIM_Period = 0xFFFF,
	.TIM_RepetitionCounter = 0x0000,
};


// Set up timers on APB2
// TIM1,8,9,10,11
static const TIM_TimeBaseInitTypeDef tim_apb2_time_base = {
	.TIM_Prescaler = (PIOS_PERIPHERAL_APB2_COUNTER_CLOCK  / 1000000) - 1,
	.TIM_ClockDivision = TIM_CKD_DIV1,
	.TIM_CounterMode = TIM_CounterMode_Up,
	.TIM_Period = 0xFFFF,
	.TIM_RepetitionCounter = 0x0000,
};


static const struct pios_tim_clock_cfg tim_3_cfg = {
	.timer = TIM3,
	.time_base_init = &tim_apb1_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM3_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
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
	.time_base_init = &tim_apb2_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM8_CC_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_clock_cfg tim_9_cfg = {
	.timer = TIM9,
	.time_base_init = &tim_apb2_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM1_BRK_TIM9_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_clock_cfg tim_12_cfg = {
	.timer = TIM12,
	.time_base_init = &tim_apb1_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM8_BRK_TIM12_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};


#if defined(PIOS_INCLUDE_USB)
#include "pios_usb_priv.h"

static const struct pios_usb_cfg pios_usb_main_cfg = {
	.irq = {
		.init    = {
			.NVIC_IRQChannel                   = OTG_FS_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority        = 3,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

const struct pios_usb_cfg *PIOS_BOARD_HW_DEFS_GetUsbCfg (uint32_t board_revision)
{
	(void)board_revision;
	return &pios_usb_main_cfg;
}

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


#if defined(PIOS_INCLUDE_CAN)
#include "pios_can_priv.h"
struct pios_can_cfg pios_can_cfg = {
	.regs = CAN1,
	.init = {
		// To make it easy to use both F3 and F4 use the other APB1 bus rate
		// divided by 2. This matches the baud rate across devices
		.CAN_Prescaler = 21-1,   /*!< Specifies the length of a time quantum. 
								 It ranges from 1 to 1024. */
		.CAN_Mode = CAN_Mode_Normal,         /*!< Specifies the CAN operating mode.
								 This parameter can be a value of @ref CAN_operating_mode */
		.CAN_SJW = CAN_SJW_1tq,          /*!< Specifies the maximum number of time quanta 
								 the CAN hardware is allowed to lengthen or 
								 shorten a bit to perform resynchronization.
								 This parameter can be a value of @ref CAN_synchronisation_jump_width */
		.CAN_BS1 = CAN_BS1_9tq,          /*!< Specifies the number of time quanta in Bit 
								 Segment 1. This parameter can be a value of 
								 @ref CAN_time_quantum_in_bit_segment_1 */
		.CAN_BS2 = CAN_BS2_8tq,          /*!< Specifies the number of time quanta in Bit Segment 2.
								 This parameter can be a value of @ref CAN_time_quantum_in_bit_segment_2 */
		.CAN_TTCM = DISABLE, /*!< Enable or disable the time triggered communication mode.
								This parameter can be set either to ENABLE or DISABLE. */
		.CAN_ABOM = DISABLE,  /*!< Enable or disable the automatic bus-off management.
								  This parameter can be set either to ENABLE or DISABLE. */
		.CAN_AWUM = DISABLE,  /*!< Enable or disable the automatic wake-up mode. 
								  This parameter can be set either to ENABLE or DISABLE. */
		.CAN_NART = ENABLE,  /*!< Enable or disable the non-automatic retransmission mode.
								  This parameter can be set either to ENABLE or DISABLE. */
		.CAN_RFLM = DISABLE,  /*!< Enable or disable the Receive FIFO Locked mode.
								  This parameter can be set either to ENABLE or DISABLE. */
		.CAN_TXFP = DISABLE,  /*!< Enable or disable the transmit FIFO priority.
								  This parameter can be set either to ENABLE or DISABLE. */
	},
	.remap = GPIO_AF_CAN1,
	.tx = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_9,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource9,
	},
	.rx = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_8,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource8,
	},
	.rx_irq = {
		.init = {
			.NVIC_IRQChannel = CAN1_RX1_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.tx_irq = {
		.init = {
			.NVIC_IRQChannel = CAN1_TX_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
};

#endif /* PIOS_INCLUDE_CAN */

#if defined(PIOS_INCLUDE_VIDEO)
#include <pios_video.h>

void set_bw_levels(uint8_t black, uint8_t white)
{
	TIM1->CCR1 = black;
	TIM1->CCR3 = white;
}

static const struct pios_exti_cfg pios_exti_vsync_cfg __exti_config = {
	.vector = PIOS_Vsync_ISR,
	.line   = EXTI_Line1,
	.pin    = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_1,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL,
		},
	},
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = EXTI1_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.exti                                      = {
		.init                                  = {
			.EXTI_Line    = EXTI_Line1, // matches above GPIO pin
			.EXTI_Mode    = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Falling,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

const struct pios_video_cfg pios_video_cfg = {
	.mask_dma = DMA2,
	.mask                                              = {
		.regs  = SPI1,
		.remap = GPIO_AF_SPI1,
		.init  = {
			.SPI_Mode              = SPI_Mode_Slave,
			.SPI_Direction         = SPI_Direction_1Line_Tx,
			.SPI_DataSize          = SPI_DataSize_8b,
			.SPI_NSS               = SPI_NSS_Soft,
			.SPI_FirstBit          = SPI_FirstBit_MSB,
			.SPI_CRCPolynomial     = 7,
			.SPI_CPOL              = SPI_CPOL_Low,
			.SPI_CPHA              = SPI_CPHA_2Edge,
			.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
		},
		.dma     = {
			.irq                                       = {
				.flags = (DMA_IT_TCIF3),
				.init  = {
					.NVIC_IRQChannel    = DMA2_Stream3_IRQn,
					.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
					.NVIC_IRQChannelSubPriority        = 0,
					.NVIC_IRQChannelCmd = ENABLE,
				},
			},
			/*.rx = {},*/
			.tx                                        = {
				.channel = DMA2_Stream3,
				.init    = {
					.DMA_Channel            = DMA_Channel_3,
					.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR),
					.DMA_DIR                = DMA_DIR_MemoryToPeripheral,
					.DMA_BufferSize         = BUFFER_WIDTH,
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
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_5,
				.GPIO_Speed = GPIO_Speed_100MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
		},
		.miso                                          = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_6,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
		},
		.slave_count                                   = 1,
	},
	.level_dma = DMA1,
	.level                                             = {
		.regs  = SPI2,
		.remap = GPIO_AF_SPI2,
		.init  = {
			.SPI_Mode              = SPI_Mode_Slave,
			.SPI_Direction         = SPI_Direction_1Line_Tx,
			.SPI_DataSize          = SPI_DataSize_8b,
			.SPI_NSS               = SPI_NSS_Soft,
			.SPI_FirstBit          = SPI_FirstBit_MSB,
			.SPI_CRCPolynomial     = 7,
			.SPI_CPOL              = SPI_CPOL_Low,
			.SPI_CPHA              = SPI_CPHA_2Edge,
			.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
		},
		.dma     = {
			.irq                                       = {
				.flags = (DMA_IT_TCIF4),
				.init  = {
					.NVIC_IRQChannel    = DMA1_Stream4_IRQn,
					.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
					.NVIC_IRQChannelSubPriority        = 0,
					.NVIC_IRQChannelCmd = ENABLE,
				},
			},
			/*.rx = {},*/
			.tx                                        = {
				.channel = DMA1_Stream4,
				.init    = {
					.DMA_Channel            = DMA_Channel_0,
					.DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR),
					.DMA_DIR                = DMA_DIR_MemoryToPeripheral,
					.DMA_BufferSize         = BUFFER_WIDTH,
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
				.GPIO_Pin   = GPIO_Pin_13,
				.GPIO_Speed = GPIO_Speed_100MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
		},
		.miso                                          = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_2,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
		},
		.slave_count                                   = 1,
	},

	.vsync = &pios_exti_vsync_cfg,

	.hsync_capture                                     = {
		.timer = TIM2,
		.timer_chan                                    = TIM_Channel_2,
		.pin   = {
			.gpio = GPIOB,
			.init = {
				.GPIO_Pin   = GPIO_Pin_3,
				.GPIO_Speed = GPIO_Speed_100MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source                                = GPIO_PinSource3,
		},
		.remap                                         = GPIO_AF_TIM2,
	},

	.pixel_timer                                       = {
		.timer = TIM3,
		.timer_chan                                    = TIM_Channel_1,
		.pin   = {
			.gpio = GPIOB,
			.init = {
				.GPIO_Pin   = GPIO_Pin_4,
				.GPIO_Speed = GPIO_Speed_100MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_UP
			},
			.pin_source                                = GPIO_PinSource4,
		},
		.remap                                         = GPIO_AF_TIM3,
	},

	.line_counter = TIM4,

	.tim_oc_init                                       = {
		.TIM_OCMode       = TIM_OCMode_PWM1,
		.TIM_OutputState  = TIM_OutputState_Enable,
		.TIM_OutputNState = TIM_OutputNState_Disable,
		.TIM_Pulse        = 1,
		.TIM_OCPolarity   = TIM_OCPolarity_High,
		.TIM_OCNPolarity  = TIM_OCPolarity_High,
		.TIM_OCIdleState  = TIM_OCIdleState_Reset,
		.TIM_OCNIdleState = TIM_OCNIdleState_Reset,
	},
	.set_bw_levels = set_bw_levels,
};

#endif /* if defined(PIOS_INCLUDE_VIDEO) */

#if defined(PIOS_INCLUDE_SPI)
#include <pios_spi_priv.h>

/*
 * SPI3 Interface
 * Used for MAX7456
 */
void PIOS_SPI_telem_flash_irq_handler(void);
void DMA1_Stream0_IRQHandler(void) __attribute__((alias("PIOS_SPI_max7456_irq_handler")));
void DMA1_Stream5_IRQHandler(void) __attribute__((alias("PIOS_SPI_max7456_irq_handler")));
static const struct pios_spi_cfg pios_spi_max7456_cfg = {
	.regs = SPI3,
	.remap = GPIO_AF_SPI3,
	.init = {
		.SPI_Mode              = SPI_Mode_Master,
		.SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
		.SPI_DataSize          = SPI_DataSize_8b,
		.SPI_NSS               = SPI_NSS_Soft,
		.SPI_FirstBit          = SPI_FirstBit_MSB,
		.SPI_CRCPolynomial     = 7,
		.SPI_CPOL              = SPI_CPOL_Low,
		.SPI_CPHA              = SPI_CPHA_1Edge,
		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8,
	},
	.sclk = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin = GPIO_Pin_10,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
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
	},
	.slave_count = 1,
	.ssel = { 
		{ // MAX7456
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin = GPIO_Pin_15,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_UP
			}
		},
	},
};

uint32_t pios_spi_max7456_id;
void PIOS_SPI_max7456_irq_handler(void)
{
	/* Call into the generic code to handle the IRQ for this specific device */
	PIOS_SPI_IRQ_Handler(pios_spi_max7456_id);
}

#endif // defined(PIOS_INCLUDE_SPI)

/**
 * @}
 * @}
 */
