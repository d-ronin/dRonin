/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DAC DAC Functions
 *
 * @file       pios_dac.h
 * @author     dRonin, http://dronin.org Copyright (C) 2017
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

#ifndef _PIOS_DAC_H
#define _PIOS_DAC_H

#include <stdint.h>
#include <stdio.h>

#include "stm32f4xx_dac.h"

typedef struct dac_dev_s *dac_dev_t;

struct pios_dac_cfg {
	TIM_TypeDef *timer;
	TIM_TimeBaseInitTypeDef clock_cfg;

	NVIC_InitTypeDef interrupt;

	GPIO_TypeDef *gpio;
	uint16_t gpio_pin;

	DMA_Stream_TypeDef *dma_stream;
	uint32_t dma_channel;

	uint32_t dac_channel;
	uint32_t dac_trigger;
	uintptr_t dac_outreg;

	uint32_t dma_tcif;
};

/**
 * @brief Allocate and initialise DAC device
 * @param[out] dev_out Device handle, only valid when return value is success
 * @param[in] cfg Configuration structure
 * @retval 0 on success, else failure
 */
int PIOS_DAC_init(dac_dev_t *dev_out,
		const struct pios_dac_cfg *cfg);

/**
 * @brief Handles a DMA completion interrupt on dma_stream.
 * @param[in] dev DAC device handle
 */
void PIOS_DAC_dma_interrupt_handler(dac_dev_t dev);

typedef bool (*fill_dma_cb)(void *ctx, uint16_t *buf, int len);

/**
 * Installs a DAC sample-generating callback.  Also starts DMA if necessary.
 * @param[in] dev A DAC device
 * @param[in] priority The priority for this callback; higher numbers prevail.
 * @param[in] cb The callback function.
 * @param[in] ctx An optional context pointer provided to the callback function.
 * @retval true if the callback was installed, false if a higher prio is running.
 */
bool PIOS_DAC_install_callback(dac_dev_t dev, uint8_t priority, fill_dma_cb cb,
		void *ctx);

#endif /* LIB_MAX7456_MAX7456_H_ */
