/**
 ******************************************************************************
 *
 * @file       pios_ws2811_cfg.h
 * @author     Cleanflight/Betaflight
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @brief      A driver for ws2811 rgb led controller.
 *             this is a port of the CleanFlight/BetaFlight implementation,
 *             and in turn ported from LibrePilot.
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

#ifndef _PIOS_WS2811_TARGET_H
#define _PIOS_WS2811_TARGET_H

#include <pios.h>

struct pios_ws2811_cfg {
	TIM_TypeDef *timer;
	uint8_t timer_chan;

	GPIO_TypeDef *led_gpio;
	uint16_t gpio_pin;

	uint32_t remap;

	uint16_t timer_dma_source;

	DMA_Channel_TypeDef *dma_chan;

	uint32_t dma_tcif;

	uint8_t dma_irqn;
};

#endif /* _PIOS_WS2811_TARGET_H */
