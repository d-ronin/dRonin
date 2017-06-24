/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_WS2811 WS2811 Functions
 * @{
 *
 * @file       flight/PiOS/STM32F4xx/inc/pios_ws2811_target.h
 * @author     dRonin, http://dronin.org Copyright (C) 2016
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

#ifndef _PIOS_WS2811_TARGET_H
#define _PIOS_WS2811_TARGET_H

#include <stdint.h>
#include <stdio.h>

struct pios_ws2811_cfg {
	TIM_TypeDef *timer;
	TIM_TimeBaseInitTypeDef clock_cfg;

	uint8_t fall_time_l, fall_time_h;

	NVIC_InitTypeDef interrupt;

	GPIO_TypeDef *led_gpio;

	uint16_t gpio_pin;

	DMA_Stream_TypeDef *bit_set_dma_stream;
	uint32_t bit_set_dma_channel;

	DMA_Stream_TypeDef *bit_clear_dma_stream;
	uint32_t bit_clear_dma_channel;
	uint32_t bit_clear_dma_tcif;
};

/**
 * @}
 * @}
 */

#endif /* _PIOS_WS2811_TARGET_H */
