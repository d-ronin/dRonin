/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_WS2811 WS2811 Functions
 *
 * @file       pios_ws2811.h
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef _PIOS_WS2811_H
#define _PIOS_WS2811_H

#include <stdint.h>
#include <stdio.h>

typedef struct ws2811_dev_s *ws2811_dev_t;

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
 * @brief Allocate and initialise WS2811 device
 * @param[out] dev_out Device handle, only valid when return value is success
 * @param[in] cfg Configuration structure
 * @param[in] max_leds Number of LEDs to update each update cycle.
 * @retval 0 on success, else failure
 */
int PIOS_WS2811_init(ws2811_dev_t *dev_out,
		const struct pios_ws2811_cfg *cfg, int max_leds);

/**
 * @brief Trigger an update of the LED strand
 * @param[in] dev WS2811 device handle
 */
void PIOS_WS2811_trigger_update(ws2811_dev_t dev);

/**
 * @brief Set a given LED to a color value.
 * @param[in] dev WS2811 device handle
 * @param[in] idx index of the led to update
 * @param[in] r the red brightness value
 * @param[in] g the green brightness value
 * @param[in] b the blue brightness value
 */
void PIOS_WS2811_set(ws2811_dev_t dev, int idx, uint8_t r, uint8_t g,
		uint8_t b);

/**
 * @brief Sets all LEDs to a color value.
 * @param[in] dev WS2811 device handle
 * @param[in] r the red brightness value
 * @param[in] g the green brightness value
 * @param[in] b the blue brightness value
 */
void PIOS_WS2811_set_all(ws2811_dev_t dev, uint8_t r, uint8_t g,
		uint8_t b);

/**
 * @brief Handles a DMA completion interrupt on bit_clear_dma_stream.
 * @param[in] dev WS2811 device handle
 */
void PIOS_WS2811_dma_interrupt_handler(ws2811_dev_t dev);

/**
 * @brief Find out how many LEDs are configured on an interface.
 * @param[in] dev WS2811 device handle
 * @returns number of leds configured.
 */
int PIOS_WS2811_get_num_leds(ws2811_dev_t dev);

#endif /* LIB_MAX7456_MAX7456_H_ */
