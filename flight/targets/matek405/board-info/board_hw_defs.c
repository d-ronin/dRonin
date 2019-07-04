/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup MATEK405CTR Matek MATEK405 * @{
 *
 * @file       matek405/board-info/board_hw_defs.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      Defines board specific static initializers for hardware for the
 *             MATEK405 board.
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

#include <pios_config.h>
#include <pios_board_info.h>

#if defined(PIOS_INCLUDE_ANNUNC)

#include <pios_annunc_priv.h>
static const struct pios_annunc pios_annuncs[] = {
	[PIOS_LED_HEARTBEAT] = {
		.pin = {
			.gpio = GPIOB,
			.init = {
				.GPIO_Pin   = GPIO_Pin_9,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL,
			},
		},
		.active_high = false,
	},
	[PIOS_LED_ALARM] = {
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_14,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL,
			},
		},
		.active_high = false,
	},
	[PIOS_ANNUNCIATOR_BUZZER] = {
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_13,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_NOPULL
			},
		},
		.active_high = true,
	},
};

static const struct pios_annunc_cfg pios_annunc_cfg = {
	.annunciators     = pios_annuncs,
	.num_annunciators = NELEMENTS(pios_annuncs),
};

const struct pios_annunc_cfg * PIOS_BOARD_HW_DEFS_GetLedCfg (uint32_t board_revision)
{
	return &pios_annunc_cfg;
}

#endif	/* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_SPI)
#include <pios_spi_priv.h>

/* SPI2 Interface
 *      - Used for OSD
 */

static const struct pios_spi_cfg pios_spi_osd_cfg = {
	.regs = SPI2,
	.remap = GPIO_AF_SPI2,
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
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_13,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
		.pin_source = GPIO_PinSource13,
	},
	.miso = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_14,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
		.pin_source = GPIO_PinSource14,
	},
	.mosi = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_15,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
		.pin_source = GPIO_PinSource15,
	},
	.slave_count = 1,
	.ssel = { {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_10,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	} },
};

pios_spi_t pios_spi_osd_id;

/* SPI3 Interface
 *      - Used for gyro communications
 */

static const struct pios_spi_cfg pios_spi_gyro_cfg = {
	.regs = SPI1,
	.remap = GPIO_AF_SPI1,
	.init = {
		.SPI_Mode              = SPI_Mode_Master,
		.SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
		.SPI_DataSize          = SPI_DataSize_8b,
		.SPI_NSS               = SPI_NSS_Soft,
		.SPI_FirstBit          = SPI_FirstBit_MSB,
		.SPI_CRCPolynomial     = 7,
		.SPI_CPOL              = SPI_CPOL_High,
		.SPI_CPHA              = SPI_CPHA_2Edge,
		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32,		//@ APB2 PCLK1 82MHz / 32 == 2.6MHz
	},
	.sclk = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_5,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
		.pin_source = GPIO_PinSource5,
	},
	.miso = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_6,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
		.pin_source = GPIO_PinSource6,
	},
	.mosi = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_7,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
		.pin_source = GPIO_PinSource7,
	},
	.slave_count = 1,
	.ssel = { {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_2,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	} },
};

pios_spi_t pios_spi_gyro_id;

#endif	/* PIOS_INCLUDE_SPI */

#if defined(PIOS_INCLUDE_I2C)

#include <pios_i2c_priv.h>

void PIOS_I2C_internal_ev_irq_handler(void);
void PIOS_I2C_internal_er_irq_handler(void);

void I2C1_EV_IRQHandler() __attribute__ ((alias ("PIOS_I2C_internal_ev_irq_handler")));
void I2C1_ER_IRQHandler() __attribute__ ((alias ("PIOS_I2C_internal_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_internal_cfg = {
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
			.GPIO_Pin   = GPIO_Pin_6,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_UP,
		},
	.pin_source = GPIO_PinSource6,
	},
	.sda = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_7,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_UP,
		},
	.pin_source = GPIO_PinSource7,
	},
	.event = {
		.flags = 0,		/* FIXME: check this */
		.init = {
			.NVIC_IRQChannel                   = I2C1_EV_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.error = {
    .flags   = 0,		/* FIXME: check this */
    .init = {
			.NVIC_IRQChannel                   = I2C1_ER_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

pios_i2c_t pios_i2c_internal_id;
void PIOS_I2C_internal_ev_irq_handler(void)
{
  /* Call into the generic code to handle the IRQ for this specific device */
  PIOS_I2C_EV_IRQ_Handler(pios_i2c_internal_id);
}

void PIOS_I2C_internal_er_irq_handler(void)
{
	/* Call into the generic code to handle the IRQ for this specific device */
	PIOS_I2C_ER_IRQ_Handler(pios_i2c_internal_id);
}

void PIOS_I2C_external_ev_irq_handler(void);
void PIOS_I2C_external_er_irq_handler(void);

void I2C2_EV_IRQHandler() __attribute__ ((alias ("PIOS_I2C_external_ev_irq_handler")));
void I2C2_ER_IRQHandler() __attribute__ ((alias ("PIOS_I2C_external_er_irq_handler")));

static const struct pios_i2c_adapter_cfg pios_i2c_external_cfg = {
	.regs = I2C2,
	.remap = GPIO_AF_I2C2,
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
			.GPIO_Pin   = GPIO_Pin_10,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_UP,
		},
	.pin_source = GPIO_PinSource10,
	},
	.sda = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_11,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_UP,
	},
	.pin_source = GPIO_PinSource11,
	},
	.event = {
		.flags = 0,		/* FIXME: check this */
	.	init = {
			.NVIC_IRQChannel                   = I2C2_EV_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.error = {
		.flags   = 0,		/* FIXME: check this */
		.init = {
			.NVIC_IRQChannel                   = I2C2_ER_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

pios_i2c_t pios_i2c_external_id;
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

#if defined(PIOS_INCLUDE_FLASH)
#include "pios_flashfs_logfs_priv.h"

static const struct flashfs_logfs_cfg flashfs_settings_cfg = {
	.fs_magic      = 0x3b1b14cf,
	.arena_size    = 0x00004000, /* 64 * slot size = 16K bytes = 1 sector */
	.slot_size     = 0x00000100, /* 256 bytes */
};

#include "pios_flash_internal_priv.h"

static const struct pios_flash_internal_cfg flash_internal_cfg = {
};

#include "pios_flash_priv.h"

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

static const struct pios_flash_partition pios_flash_partition_table[] = {
	{
		.label        = FLASH_PARTITION_LABEL_BL,
		.chip_desc    = &pios_flash_chip_internal,
		.first_sector = 0,
		.last_sector  = 1,
		.chip_offset  = 0,                               // 0x0800 0000
		.size         = (1 - 0 + 1) * FLASH_SECTOR_16KB, // 32KB
	},

	{
		.label        = FLASH_PARTITION_LABEL_SETTINGS,
		.chip_desc    = &pios_flash_chip_internal,
		.first_sector = 2,
		.last_sector  = 3,
		.chip_offset  = (2 * FLASH_SECTOR_16KB),         // 0x0800 8000
		.size         = (3 - 2 + 1) * FLASH_SECTOR_16KB, // 32KB
	},

	/* NOTE: sector 4 of internal flash is currently unallocated */

	{
		.label        = FLASH_PARTITION_LABEL_FW,
		.chip_desc    = &pios_flash_chip_internal,
		.first_sector = 5,
		.last_sector  = 7,
		.chip_offset  = (4 * FLASH_SECTOR_16KB) + (1 * FLASH_SECTOR_64KB), // 0x0802 0000
		.size         = (7 - 5 + 1) * FLASH_SECTOR_128KB,                  // 384KB
	},

	/* NOTE: sectors 8-9 of the internal flash are currently unallocated */

	{
		.label        = FLASH_PARTITION_LABEL_AUTOTUNE,
		.chip_desc    = &pios_flash_chip_internal,
		.first_sector = 10,
		.last_sector  = 10,
		.chip_offset  = (4 * FLASH_SECTOR_16KB) + (1 * FLASH_SECTOR_64KB) + (5 * FLASH_SECTOR_128KB), // 0x080C 0000
		.size         = (10 - 10 + 1) * FLASH_SECTOR_128KB,                                           // 128KB
	},

	/* Sector 11 is unallocated, but in old bootloaders was allocated to
	 * waypoints/autotune.
	 */
};

const struct pios_flash_partition * PIOS_BOARD_HW_DEFS_GetPartitionTable (uint32_t board_revision, uint32_t * num_partitions)
{
	PIOS_Assert(num_partitions);

	*num_partitions = NELEMENTS(pios_flash_partition_table);
	return pios_flash_partition_table;
}

#endif	/* PIOS_INCLUDE_FLASH */

#if defined(PIOS_INCLUDE_USART)

#include "pios_usart_priv.h"

#if defined(PIOS_INCLUDE_DSM)
/*
 * Spektrum/JR DSM USART
 */
#include <pios_dsm_priv.h>

static const struct pios_dsm_cfg pios_usart1_dsm_aux_cfg = {
	.bind = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_10,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	},
};

static const struct pios_dsm_cfg pios_usart2_dsm_aux_cfg = {
	.bind = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_3,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	},
};

static const struct pios_dsm_cfg pios_usart3_dsm_aux_cfg = {
	.bind = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_11,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	},
};

static const struct pios_dsm_cfg pios_usart4_dsm_aux_cfg = {
	.bind = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_1,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	},
};
			
static const struct pios_dsm_cfg pios_usart5_dsm_aux_cfg = {
	.bind = {
		.gpio = GPIOD,
		.init = {
			.GPIO_Pin   = GPIO_Pin_2,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_OUT,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL
		},
	},
};

#endif	/* PIOS_INCLUDE_DSM */

static const struct pios_usart_cfg pios_usart1_cfg = {
	.regs = USART1,
	.remap = GPIO_AF_USART1,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = USART1_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.rx = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_10,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource10,
	},
	.tx = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_9,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource9,
	},
};

static const struct pios_usart_cfg pios_usart2_cfg = {
	.regs = USART2,
	.remap = GPIO_AF_USART2,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = USART2_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.rx = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_3,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource3,
	},
	.tx = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_2,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource2,
	},
};

static const struct pios_usart_cfg pios_usart3_cfg = {
	.regs = USART3,
	.remap = GPIO_AF_USART3,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = USART3_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.rx = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_11,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource11,
	},
	.tx = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_10,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource10,
	},
};

static const struct pios_usart_cfg pios_usart4_cfg = {
	.regs = UART4,
	.remap = GPIO_AF_UART4,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = UART4_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.rx = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_1,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource1,
	},
	.tx = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_0,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource0,
	},
};

static const struct pios_usart_cfg pios_usart5_cfg = {
	.regs = UART5,
	.remap = GPIO_AF_UART5,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = UART5_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.rx = {
		.gpio = GPIOD,
		.init = {
			.GPIO_Pin   = GPIO_Pin_2,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource2,
	},
	.tx = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_12,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
		.pin_source = GPIO_PinSource12,
	},
};

#endif  /* PIOS_INCLUDE_USART */

#if defined(PIOS_INCLUDE_COM)

#include "pios_com_priv.h"

#endif	/* PIOS_INCLUDE_COM */

#if defined(PIOS_INCLUDE_RTC)
/*
 * Realtime Clock (RTC)
 */
#include <pios_rtc_priv.h>

void PIOS_RTC_IRQ_Handler (void);
void RTC_WKUP_IRQHandler() __attribute__ ((alias ("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
	.clksrc = RCC_RTCCLKSource_HSE_Div8, // Divide 8 MHz HSE by 8 = 1 MHz
	// 1 MHz resulting clock is then divided
	// by another 16 to give a nominal 62.5 kHz clock
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

//Timers used for inputs (5)

// Set up timers that only have outputs on APB1
static const TIM_TimeBaseInitTypeDef tim_5_time_base = {
	.TIM_Prescaler = (PIOS_PERIPHERAL_APB1_COUNTER_CLOCK / 1000000) - 1,
	.TIM_ClockDivision = TIM_CKD_DIV1,
	.TIM_CounterMode = TIM_CounterMode_Up,
	.TIM_Period = 0xFFFF,
	.TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_5_cfg = {
	.timer = TIM5,
	.time_base_init = &tim_5_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM5_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

// Timers used for outputs (3,8)

// Set up timers that only have outputs on APB1
static const TIM_TimeBaseInitTypeDef tim_2_3_4_time_base = {
	.TIM_Prescaler         = (PIOS_PERIPHERAL_APB1_COUNTER_CLOCK / 1000000) - 1,
	.TIM_ClockDivision     = TIM_CKD_DIV1,
	.TIM_CounterMode       = TIM_CounterMode_Up,
	.TIM_Period            = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
	.TIM_RepetitionCounter = 0x0000,
};

// Set up timers that only have outputs on APB2
static const TIM_TimeBaseInitTypeDef tim_1_8_time_base = {
	.TIM_Prescaler         = (PIOS_PERIPHERAL_APB2_COUNTER_CLOCK / 1000000) - 1,
	.TIM_ClockDivision     = TIM_CKD_DIV1,
	.TIM_CounterMode       = TIM_CounterMode_Up,
	.TIM_Period            = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
	.TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_3_cfg = {
	.timer = TIM3,
	.time_base_init = &tim_2_3_4_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM3_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_clock_cfg tim_8_cfg = {
	.timer = TIM8,
	.time_base_init = &tim_1_8_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM8_CC_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_clock_cfg tim_2_cfg = {
	.timer = TIM2,
	.time_base_init = &tim_2_3_4_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM2_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_clock_cfg tim_1_cfg = {
	.timer = TIM1,
	.time_base_init = &tim_1_8_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM1_CC_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_clock_cfg tim_4_cfg = {
	.timer = TIM4,
	.time_base_init = &tim_2_3_4_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM4_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

/**
 * Pios servo configuration structures
 */

/*
 * Outputs
	1:  TIM3_CH1 (PC6)        DMA1 Stream 2 Channel 5
	2:  TIM8_CH2 (PC7)        DMA2 Stream 1 Channel 7
	3:  TIM8_CH3 (PC8)
	4:  TIM8_CH4 (PC9)
	5:  TIM2_CH1 (PA15)       DMA1 Stream 1 Channel 3
	6:  TIM1_CH1 (PA8)        DMA2 Stream 5 Channel 6
	7:  TIM4_CH3 (PB8)        DMA1 Stream 6 Channel 2
 */

static const struct pios_tim_channel pios_tim_outputs_pins_without_led_string[] = {
	{
		.timer = TIM3,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_TIM3,
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_6,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
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
				.GPIO_Pin   = GPIO_Pin_7,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource7,
		},
	},
	{
		.timer = TIM8,
		.timer_chan = TIM_Channel_3,
		.remap = GPIO_AF_TIM8,
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_8,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource8,
		},
	},
	{
		.timer = TIM8,
		.timer_chan = TIM_Channel_4,
		.remap = GPIO_AF_TIM8,
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_9,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource9,
		},
	},
	{
		.timer = TIM2,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_TIM2,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_15,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource15,
		},
	},
	{
		.timer = TIM1,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_TIM1,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_8,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource8,
		},
	},
	{
		.timer = TIM4,
		.timer_chan = TIM_Channel_3,
		.remap = GPIO_AF_TIM4,
		.pin = {
			.gpio = GPIOB,
			.init = {
				.GPIO_Pin   = GPIO_Pin_8,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource8,
		},
	},
};

static const struct pios_tim_channel pios_tim_outputs_pins_with_led_string[] = {
	{
		.timer = TIM3,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_TIM3,
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_6,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
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
				.GPIO_Pin   = GPIO_Pin_7,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource7,
		},
	},
	{
		.timer = TIM8,
		.timer_chan = TIM_Channel_3,
		.remap = GPIO_AF_TIM8,
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_8,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource8,
		},
	},
	{
		.timer = TIM8,
		.timer_chan = TIM_Channel_4,
		.remap = GPIO_AF_TIM8,
		.pin = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin   = GPIO_Pin_9,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource9,
		},
	},
	{
		.timer = TIM2,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_TIM2,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_15,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource15,
		},
	},
	{
		.timer = TIM4,
		.timer_chan = TIM_Channel_3,
		.remap = GPIO_AF_TIM4,
		.pin = {
			.gpio = GPIOB,
			.init = {
				.GPIO_Pin   = GPIO_Pin_8,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd  = GPIO_PuPd_NOPULL
			},
			.pin_source = GPIO_PinSource8,
		},
	},
};

#if defined(PIOS_INCLUDE_DMASHOT)

#include <pios_dmashot.h>

/*
 * Outputs
	1:  TIM3_CH1 (PC6)        DMA1 Stream 2 Channel 5
	2:  TIM8_CH2 (PC7)        DMA2 Stream 1 Channel 7
	3:  TIM8_CH3 (PC8)
	4:  TIM8_CH4 (PC9)
	5:  TIM2_CH1 (PA15)       DMA1 Stream 1 Channel 3
	6:  TIM1_CH1 (PA8)        DMA2 Stream 5 Channel 6
	7:  TIM4_CH3 (PB8)        DMA1 Stream 6 Channel 2
 */
  
 static const struct pios_dmashot_timer_cfg dmashot_tim_cfg_without_led_string[] = {
	{
		.timer   = TIM3,
		.stream  = DMA1_Stream2,
		.channel = DMA_Channel_5,
		.tcif    = DMA_FLAG_TCIF2
	},
	{
		.timer   = TIM8,
		.stream  = DMA2_Stream1,
		.channel = DMA_Channel_7,
		.tcif    = DMA_FLAG_TCIF1
	},
	{
		.timer   = TIM2,
		.stream  = DMA1_Stream1,
		.channel = DMA_Channel_3,
		.tcif    = DMA_FLAG_TCIF1
	},
	{
		.timer   = TIM1,
		.stream  = DMA2_Stream5,
		.channel = DMA_Channel_6,
		.tcif    = DMA_FLAG_TCIF5
	},
	{
		.timer   = TIM4,
		.stream  = DMA1_Stream6,
		.channel = DMA_Channel_2,
		.tcif    = DMA_FLAG_TCIF6
	},
};

 static const struct pios_dmashot_timer_cfg dmashot_tim_cfg_with_led_string[] = {
	{
		.timer   = TIM3,
		.stream  = DMA1_Stream2,
		.channel = DMA_Channel_5,
		.tcif    = DMA_FLAG_TCIF2
	},
	{
		.timer   = TIM8,
		.stream  = DMA2_Stream1,
		.channel = DMA_Channel_7,
		.tcif    = DMA_FLAG_TCIF1
	},
	{
		.timer   = TIM2,
		.stream  = DMA1_Stream1,
		.channel = DMA_Channel_3,
		.tcif    = DMA_FLAG_TCIF1
	},
	{
		.timer   = TIM4,
		.stream  = DMA1_Stream6,
		.channel = DMA_Channel_2,
		.tcif    = DMA_FLAG_TCIF6
	},
};

static const struct pios_dmashot_cfg dmashot_config_without_led_string = {
	.timer_cfg =  &dmashot_tim_cfg_without_led_string[0],
	.num_timers = NELEMENTS(dmashot_tim_cfg_without_led_string)
};

static const struct pios_dmashot_cfg dmashot_config_with_led_string = {
	.timer_cfg =  &dmashot_tim_cfg_with_led_string[0],
	.num_timers = NELEMENTS(dmashot_tim_cfg_with_led_string)
};

#endif // defined(PIOS_INCLUDE_DMASHOT)

#if defined(PIOS_INCLUDE_SERVO) && defined(PIOS_INCLUDE_TIM)
/*
 * Servo outputs
 */
#include <pios_servo_priv.h>

static const struct pios_servo_cfg pios_servo_cfg_without_led_string = {
	.tim_oc_init = {
		.TIM_OCMode       = TIM_OCMode_PWM1,
		.TIM_OutputState  = TIM_OutputState_Enable,
		.TIM_OutputNState = TIM_OutputNState_Disable,
		.TIM_Pulse        = PIOS_SERVOS_INITIAL_POSITION,
		.TIM_OCPolarity   = TIM_OCPolarity_High,
		.TIM_OCNPolarity  = TIM_OCPolarity_High,
		.TIM_OCIdleState  = TIM_OCIdleState_Reset,
		.TIM_OCNIdleState = TIM_OCNIdleState_Reset,
	},
	.channels = pios_tim_outputs_pins_without_led_string,
	.num_channels = NELEMENTS(pios_tim_outputs_pins_without_led_string),
};

static const struct pios_servo_cfg pios_servo_cfg_with_led_string = {
	.tim_oc_init = {
		.TIM_OCMode       = TIM_OCMode_PWM1,
		.TIM_OutputState  = TIM_OutputState_Enable,
		.TIM_OutputNState = TIM_OutputNState_Disable,
		.TIM_Pulse        = PIOS_SERVOS_INITIAL_POSITION,
		.TIM_OCPolarity   = TIM_OCPolarity_High,
		.TIM_OCNPolarity  = TIM_OCPolarity_High,
		.TIM_OCIdleState  = TIM_OCIdleState_Reset,
		.TIM_OCNIdleState = TIM_OCNIdleState_Reset,
	},
	.channels = pios_tim_outputs_pins_with_led_string,
	.num_channels = NELEMENTS(pios_tim_outputs_pins_with_led_string),
};

#endif	/* PIOS_INCLUDE_SERVO && PIOS_INCLUDE_TIM */

/*
 * 	PPM INPUT
	1: TIM5_CH4 (PA3)
 */
static const struct pios_tim_channel pios_tim_rcvrport_ppm[] = {
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
};

/*
 * PPM Input
 */
#if defined(PIOS_INCLUDE_PPM)
#include <pios_ppm_priv.h>

/*
 * PPM Input
 */
const struct pios_ppm_cfg pios_ppm_cfg = {
	.tim_ic_init = {
		.TIM_ICPolarity = TIM_ICPolarity_Rising,
		.TIM_ICSelection = TIM_ICSelection_DirectTI,
		.TIM_ICPrescaler = TIM_ICPSC_DIV1,
		.TIM_ICFilter = 0x0,
		.TIM_Channel = TIM_Channel_4,
	},
	.channels = &pios_tim_rcvrport_ppm[0],
	.num_channels = 1,
};

#endif

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
		.gpio = NULL,
	},
};

const struct pios_usb_cfg * PIOS_BOARD_HW_DEFS_GetUsbCfg (uint32_t board_revision)
{
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

#if defined(PIOS_INCLUDE_ADC)
#include "pios_adc_priv.h"
#include "pios_internal_adc_priv.h"

void PIOS_ADC_DMA_irq_handler(void);
void DMA2_Stream0_IRQHandler(void) __attribute__((alias("PIOS_ADC_DMA_irq_handler")));
struct pios_internal_adc_cfg pios_adc_cfg = {
	.adc_dev_master = ADC1,
	.dma = {
		.irq = {
			.flags = (DMA_FLAG_TCIF0 | DMA_FLAG_TEIF0 | DMA_FLAG_HTIF0),
			.init = {
				.NVIC_IRQChannel = DMA2_Stream0_IRQn,
				.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
				.NVIC_IRQChannelSubPriority = 0,
				.NVIC_IRQChannelCmd = ENABLE,
			},
		},
		.rx = {
			.channel = DMA2_Stream0,
			.init = {
				.DMA_Channel = DMA_Channel_0,
				.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR
			},
		}
	},
	.half_flag = DMA_IT_HTIF0,
	.full_flag = DMA_IT_TCIF0,
	.adc_pins =  {                                                                                   \
		{ GPIOC, GPIO_Pin_4, ADC_Channel_14 },  /* Current                                       */  \
		{ GPIOC, GPIO_Pin_5, ADC_Channel_15 },  /* Voltage                                       */  \
		{ GPIOB, GPIO_Pin_1, ADC_Channel_9  },  /* RSSI                                          */  \
	},
	.adc_pin_count = 3,
};

void PIOS_ADC_DMA_irq_handler(void)
{
	/* Call into the generic code to handle the IRQ for this specific device */
	PIOS_INTERNAL_ADC_DMA_Handler();
}

#endif /* PIOS_INCLUDE_ADC */

/**
 * Configuration for driving a WS2811 LED out S6 (PA8)
 */

#if defined(PIOS_INCLUDE_WS2811)
#include "pios_ws2811.h"

ws2811_dev_t pios_ws2811;

void DMA2_Stream6_IRQHandler() {
	PIOS_WS2811_dma_interrupt_handler(pios_ws2811);
}

static const struct pios_ws2811_cfg pios_ws2811_cfg = {
	.timer = TIM1,
	.clock_cfg = {
		.TIM_Prescaler = (PIOS_PERIPHERAL_APB2_COUNTER_CLOCK / 12000000) - 1,
		.TIM_ClockDivision = TIM_CKD_DIV1,
		.TIM_CounterMode = TIM_CounterMode_Up,
		.TIM_Period = 25,	/* 2.083us/bit */
	},
	.interrupt = {
		.NVIC_IRQChannel = DMA2_Stream6_IRQn,
		.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
		.NVIC_IRQChannelSubPriority = 0,
		.NVIC_IRQChannelCmd = ENABLE,
	},
	.bit_clear_dma_tcif = DMA_IT_TCIF6,
	.fall_time_l = 5,                       /* 333ns */
	.fall_time_h = 11,                      /* 833ns */
	.led_gpio = GPIOA,
	.gpio_pin = GPIO_Pin_8,                 /* PA8 */
	.bit_set_dma_stream = DMA2_Stream4,
	.bit_set_dma_channel = DMA_Channel_6,   /* 2/S4/C6: TIM1 CH4|TRIG|COM */
	.bit_clear_dma_stream = DMA2_Stream6,
	.bit_clear_dma_channel = DMA_Channel_0, /* 0/S6/C0: TIM1 CH1|CH2|CH3 */
};
#endif

#if defined(PIOS_INCLUDE_DAC)
#include "pios_dac.h"

dac_dev_t pios_dac;

void DMA1_Stream5_IRQHandler() {
	PIOS_DAC_dma_interrupt_handler(pios_dac);
}

typedef struct dac_dev_s *dac_dev_t;

struct pios_dac_cfg pios_dac_cfg = {
	.timer = TIM6,
	.clock_cfg = {
		.TIM_Prescaler = (PIOS_PERIPHERAL_APB1_COUNTER_CLOCK / 600000) - 1,
		.TIM_ClockDivision = TIM_CKD_DIV1,
		.TIM_CounterMode = TIM_CounterMode_Up,
		.TIM_Period = 25-1,	/* 600000 / 25 = 24000 samples per sec */
	},
	.interrupt = {
		.NVIC_IRQChannel = DMA1_Stream5_IRQn,
		.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
		.NVIC_IRQChannelSubPriority = 0,
		.NVIC_IRQChannelCmd = ENABLE,
	},
	.gpio = GPIOA,
	.gpio_pin = GPIO_Pin_4,
	.dma_stream = DMA1_Stream5,
	.dma_channel = DMA_Channel_7,
	.dac_channel = DAC_Channel_1,
	.dac_trigger = DAC_Trigger_T6_TRGO,
	.dac_outreg = (uintptr_t) (&DAC->DHR12L1),
	.dma_tcif = DMA_IT_TCIF5,
};

#endif /* PIOS_INCLUDE_DAC */

/**
 * Configuration for the MPU6000 chip
 */
#if defined(PIOS_INCLUDE_MPU)
#include "pios_mpu.h"
static const struct pios_exti_cfg pios_exti_mpu_cfg __exti_config = {
	.vector = PIOS_MPU_IRQHandler,
	.line = EXTI_Line3,
		.pin = {
		.gpio = GPIOC,
		.init = {
			.GPIO_Pin   = GPIO_Pin_3,
			.GPIO_Speed = GPIO_Speed_100MHz,
			.GPIO_Mode  = GPIO_Mode_IN,
			.GPIO_OType = GPIO_OType_OD,
			.GPIO_PuPd  = GPIO_PuPd_NOPULL,
		},
	},
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = EXTI3_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.exti = {
		.init = {
			.EXTI_Line    = EXTI_Line3, // matches above GPIO pin
			.EXTI_Mode    = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Rising,
			.EXTI_LineCmd = ENABLE,
		},
	},
};

static struct pios_mpu_cfg pios_mpu_cfg = {
	.exti_cfg            = &pios_exti_mpu_cfg,
	.default_samplerate  = 1000,
	.orientation         = PIOS_MPU_TOP_180DEG
};
#endif /* PIOS_INCLUDE_MPU */

/**
 * Configuration for the BMP280
 */
#ifdef PIOS_INCLUDE_BMP280
#include "pios_bmp280_priv.h"

static const struct pios_bmp280_cfg pios_bmp280_cfg = {
	.oversampling = BMP280_HIGH_RESOLUTION,
};
#endif /* PIOS_INCLUDE_BMP280 */

#ifdef PIOS_INCLUDE_HMC5883
#include "pios_hmc5883_priv.h"
static const struct pios_hmc5883_cfg pios_hmc5883_external_cfg = {
	.exti_cfg = NULL,
	.M_ODR = PIOS_HMC5883_ODR_75,
	.Meas_Conf = PIOS_HMC5883_MEASCONF_NORMAL,
	.Gain = PIOS_HMC5883_GAIN_1_9,
	.Mode = PIOS_HMC5883_MODE_CONTINUOUS,
	.Default_Orientation = PIOS_HMC5883_TOP_0DEG,
};
#endif

#ifdef PIOS_INCLUDE_HMC5983_I2C
#include "pios_hmc5983.h"

static const struct pios_hmc5983_cfg pios_hmc5983_external_cfg = {
	.exti_cfg            = NULL,
	.M_ODR               = PIOS_HMC5983_ODR_75,
	.Meas_Conf           = PIOS_HMC5983_MEASCONF_NORMAL,
	.Gain                = PIOS_HMC5983_GAIN_1_9,
	.Mode                = PIOS_HMC5983_MODE_CONTINUOUS,
	.Averaging           = PIOS_HMC5983_AVERAGING_1,
	.Orientation         = PIOS_HMC5983_TOP_0DEG,
};

#endif
/**
 * @}
 * @}
 */
