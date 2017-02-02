/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SPI_SLAVE SPI Slave Functions
 * @brief PIOS interface for subsidiary microcontrollers
 * @{
 *
 * @file       pios_spislave.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org, Copyright (C) 2016
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

#ifndef PIOS_SPISLAVE_PRIV_H
#define PIOS_SPISLAVE_PRIV_H

#include <pios.h>
#include <pios_stm32.h>
#include "pios_semaphore.h"

typedef struct pios_spislave_dev * spislave_t;

struct pios_spislave_cfg {
	SPI_TypeDef *regs;
	uint32_t remap;				/* GPIO_Remap_* or GPIO_AF_* */
	SPI_InitTypeDef init;
	struct stm32_gpio sclk, miso, mosi, ssel;

	struct stm32_dma_chan rx_dma, tx_dma;

	uint8_t max_rx_len;

	void (*process_message)(void *ctx, int len, int *resp_len);

	void *ctx;
};

int32_t PIOS_SPISLAVE_Init(spislave_t *spislave_id,
		const struct pios_spislave_cfg *cfg,
		int initial_tx_size);
int32_t PIOS_SPISLAVE_PollSS(spislave_t spislave_dev);

#endif /* PIOS_SPISLAVE_PRIV_H */

/**
  * @}
  * @}
  */
