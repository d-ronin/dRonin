/**
 ******************************************************************************
 * @file       main.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org, Copyright (C) 2016
 * @brief      Start the RTOS and the Modules.
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


/* OpenPilot Includes */
#define PIOS_INCLUDE_SPI
#include "openpilot.h"
#include "uavobjectsinit.h"
#include "systemmod.h"

#include "pios_spi_priv.h"

#include "flyingpio_messages.h"

void PIOS_Board_Init(void);

/* SPI command link interface
 *
 * NOTE: Leave this declared as const data so that it ends up in the
 * .rodata section (ie. Flash) rather than in the .bss section (RAM).
 */
static const struct pios_spi_cfg pios_spi_control_cfg = {
	.regs   = SPI1,
	.init   = {
		.SPI_Mode              = SPI_Mode_Slave,
		.SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
		.SPI_DataSize          = SPI_DataSize_8b,
		.SPI_NSS               = SPI_NSS_Hard,
		.SPI_FirstBit          = SPI_FirstBit_MSB,
		.SPI_CPOL              = SPI_CPOL_Low,
		.SPI_CPHA              = SPI_CPHA_1Edge,
		/* Next two not used */
		.SPI_CRCPolynomial     = 7,
		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
	},
	.sclk = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_3,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
	},
	.miso = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_4,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
	},
	.mosi = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_5,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
	},
	.slave_count = 1,
	.ssel = {{
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_4,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_UP
		}
	}}
};

struct flyingpi_msg rx_buf;
struct flyingpi_msg tx_buf;

DMA_InitTypeDef tx_dma = {
	.DMA_PeripheralBaseAddr = (uint32_t) &(SPI1->DR),
	.DMA_PeripheralInc = DMA_PeripheralInc_Disable,
	.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
	.DMA_MemoryBaseAddr = (uint32_t) &tx_buf,
	.DMA_MemoryInc = DMA_MemoryInc_Enable,
	.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
	.DMA_DIR = DMA_DIR_PeripheralDST,
	.DMA_Priority = DMA_Priority_High,
	.DMA_Mode = DMA_Mode_Normal,
	.DMA_M2M = DMA_M2M_Disable,
};

DMA_InitTypeDef rx_dma = {
	.DMA_PeripheralBaseAddr = (uint32_t) &(SPI1->DR),
	.DMA_PeripheralInc = DMA_PeripheralInc_Disable,
	.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
	.DMA_MemoryBaseAddr = (uint32_t) &rx_buf,
	.DMA_MemoryInc = DMA_MemoryInc_Enable,
	.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
	.DMA_DIR = DMA_DIR_PeripheralSRC,
	.DMA_Priority = DMA_Priority_High,
	.DMA_Mode = DMA_Mode_Normal,
	.DMA_M2M = DMA_M2M_Disable,
};

void dma_setup_spi_dma(uint32_t len)
{
	static bool first=true;

	SPI_RxFIFOThresholdConfig(SPI1, SPI_RxFIFOThreshold_QF);

	DMA_Cmd(DMA1_Channel3, DISABLE);
	DMA_Cmd(DMA1_Channel2, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);

	if (first) {
		DMA_Init(DMA1_Channel3, &tx_dma);
		DMA_Init(DMA1_Channel2, &rx_dma);
		first = false;
	}

	DMA_SetCurrDataCounter(DMA1_Channel3, len);
	DMA_SetCurrDataCounter(DMA1_Channel2, sizeof(rx_buf));

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);
	DMA_Cmd(DMA1_Channel3, ENABLE);
	DMA_Cmd(DMA1_Channel2, ENABLE);
}

void process_message(struct flyingpi_msg *msg) {
	if (!flyingpi_calc_crc(msg, false, NULL)) {
		return;
	}

	if (msg->id == FLYINGPICMD_ACTUATOR) {
		if (msg->body.actuator_fc.led_status) {
			PIOS_LED_On(PIOS_LED_HEARTBEAT);
		} else {
			PIOS_LED_Off(PIOS_LED_HEARTBEAT);
		}
	}
	/* If we get an edge and no clocking.. make sure the message is
	 * invalidated */
	msg->id = 0;
}

int main()
{
	PIOS_heap_initialize_blocks();

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();
	PIOS_Board_Init();

	GPIO_PinAFConfig(pios_spi_control_cfg.sclk.gpio,
			__builtin_ctz(pios_spi_control_cfg.sclk.init.GPIO_Pin),
			pios_spi_control_cfg.remap);
	GPIO_PinAFConfig(pios_spi_control_cfg.mosi.gpio,
			__builtin_ctz(pios_spi_control_cfg.mosi.init.GPIO_Pin),
			pios_spi_control_cfg.remap);
	GPIO_PinAFConfig(pios_spi_control_cfg.miso.gpio,
			__builtin_ctz(pios_spi_control_cfg.miso.init.GPIO_Pin),
			pios_spi_control_cfg.remap);
	GPIO_PinAFConfig(pios_spi_control_cfg.ssel[0].gpio,
			__builtin_ctz(pios_spi_control_cfg.ssel[0].init.GPIO_Pin),
			pios_spi_control_cfg.remap);

	GPIO_Init(pios_spi_control_cfg.sclk.gpio, (GPIO_InitTypeDef *) & (pios_spi_control_cfg.sclk.init));
	GPIO_Init(pios_spi_control_cfg.mosi.gpio, (GPIO_InitTypeDef *) & (pios_spi_control_cfg.mosi.init));
	GPIO_Init(pios_spi_control_cfg.miso.gpio, (GPIO_InitTypeDef *) & (pios_spi_control_cfg.miso.init));
	GPIO_SetBits(pios_spi_control_cfg.ssel[0].gpio, pios_spi_control_cfg.ssel[0].init.GPIO_Pin);
	GPIO_Init(pios_spi_control_cfg.ssel[0].gpio, (GPIO_InitTypeDef *) & (pios_spi_control_cfg.ssel[0].init));

        /* Initialize the SPI block */
        SPI_I2S_DeInit(pios_spi_control_cfg.regs);
        SPI_Init(pios_spi_control_cfg.regs, (SPI_InitTypeDef *) & (pios_spi_control_cfg.init));
        SPI_CalculateCRC(pios_spi_control_cfg.regs, DISABLE);

	SPI_Cmd(SPI1, ENABLE);
	dma_setup_spi_dma(4);

	(void) pios_spi_control_cfg;

	uint8_t last_status = Bit_SET;

	while (1) {
		uint8_t pin_status = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4);

		if (last_status != pin_status) {
			last_status = pin_status;

			if (pin_status == Bit_SET) {
				process_message(&rx_buf);

				dma_setup_spi_dma(1);
			}
		}
	}

	return 0;
}
