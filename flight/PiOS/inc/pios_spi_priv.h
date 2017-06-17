/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SPI SPI Functions
 * @brief PIOS interface to read and write from SPI ports
 * @{
 *
 * @file       pios_spi_priv.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @brief      SPI private definitions.
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

#ifndef PIOS_SPI_PRIV_H
#define PIOS_SPI_PRIV_H

#include <pios.h>
#include <pios_stm32.h>
#include "pios_semaphore.h"

#if defined(STM32F4XX)
#include "stm32f4xx_dma.h"
#include "pios_quickdma.h"
#endif

struct pios_spi_dev {
	const struct pios_spi_cfg *cfg;
	struct pios_semaphore *busy;
#if defined(STM32F4XX)
	quickdma_transfer_t dma_send;
	quickdma_transfer_t dma_recv;

	uint8_t dummy_send;
	uint8_t dummy_recv;
#endif
};

#if defined(STM32F4XX)

struct pios_spi_dma {
	struct quickdma_config send;
	struct quickdma_config recv;
};
#endif

struct pios_spi_cfg {
	SPI_TypeDef *regs;
	uint32_t remap;				/* GPIO_Remap_* or GPIO_AF_* */
	SPI_InitTypeDef init;
	struct stm32_gpio sclk;
	struct stm32_gpio miso;
	struct stm32_gpio mosi;
	uint32_t slave_count;
#if defined(STM32F4XX)
	struct pios_spi_dma spi_dma;
#endif
#ifdef PIOS_INCLUDE_VIDEO
	// XXX Hack: pios_video uses pios_spi's config structure and expects the
	// DMA information to be filled in properly
	struct stm32_dma dma;
#endif
	struct stm32_gpio ssel[];
};

#endif /* PIOS_SPI_PRIV_H */

/**
  * @}
  * @}
  */
