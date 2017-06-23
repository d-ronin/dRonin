/**
 ******************************************************************************
 *
 * @file       pios_ws2811.c
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

#include "pios.h"
#include "pios_ws2811.h"

#include "pios_dma.h"

#define WS2811_BITS_PER_LED        24
// for 50us delay
#define WS2811_DELAY_BUFFER_LENGTH 42

#define WS2811_TIMER_HZ            24000000
#define WS2811_TIMER_PERIOD        29
// timer compare value for logical 1
#define BIT_COMPARE_1              17
// timer compare value for logical 0
#define BIT_COMPARE_0              9

#define PIOS_WS2811_MAGIC 0x00281100

struct ws2811_dev_s {
	uint32_t magic;
	const struct pios_ws2811_cfg *config;

	bool dma_active;

	int max_leds;
	int buffer_size;

	uint8_t dma_buffer[];
};

int PIOS_WS2811_init(ws2811_dev_t *dev_out,
		const struct pios_ws2811_cfg *ws2811_cfg, int max_leds)
{
	if (max_leds <= 0) {
		return -2;
	}

	int buffer_size = WS2811_BITS_PER_LED * max_leds +
			WS2811_DELAY_BUFFER_LENGTH;

	ws2811_dev_t ws2811_dev = PIOS_malloc(sizeof(*ws2811_dev) + buffer_size);

	if (!ws2811_dev) {
		return -3;
	}

	memset(ws2811_dev, 0, sizeof(*ws2811_dev) + buffer_size);

	ws2811_dev->magic = PIOS_WS2811_MAGIC;
	ws2811_dev->config = ws2811_cfg;
	ws2811_dev->buffer_size = buffer_size;
	ws2811_dev->max_leds = max_leds;

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	DMA_InitTypeDef DMA_InitStructure;

	GPIO_InitTypeDef gpio_cfg = {
		.GPIO_Pin = ws2811_cfg->gpio_pin,
		.GPIO_Mode = GPIO_Mode_AF,
		/* Drive hard-- the pin is loaded on F3E, and there could be
		 * a big bus
		 */
		.GPIO_Speed = GPIO_Speed_50MHz,
		.GPIO_OType = GPIO_OType_PP,
		.GPIO_PuPd = GPIO_PuPd_NOPULL,
	};

	GPIO_Init(ws2811_cfg->led_gpio, &gpio_cfg);

	GPIO_PinAFConfig(ws2811_cfg->led_gpio,
			__builtin_ctz(ws2811_cfg->gpio_pin), ws2811_cfg->remap);

	/* Compute the prescaler value */
	uint16_t prescalerValue = (PIOS_SYSCLK / WS2811_TIMER_HZ) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period        = WS2811_TIMER_PERIOD;// 800kHz
	TIM_TimeBaseStructure.TIM_Prescaler     = prescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
	TIM_TimeBaseInit(ws2811_cfg->timer, &TIM_TimeBaseStructure);

	/* PWM1 Mode configuration: Channel1 */
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse       = 5;
	TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;

	switch (ws2811_cfg->timer_chan) {
	case TIM_Channel_1:
		TIM_OC1Init(ws2811_cfg->timer, &TIM_OCInitStructure);
		TIM_OC1PreloadConfig(ws2811_cfg->timer, TIM_OCPreload_Enable);
		break;
	case TIM_Channel_2:
		TIM_OC2Init(ws2811_cfg->timer, &TIM_OCInitStructure);
		TIM_OC2PreloadConfig(ws2811_cfg->timer, TIM_OCPreload_Enable);
		break;
	case TIM_Channel_3:
		TIM_OC3Init(ws2811_cfg->timer, &TIM_OCInitStructure);
		TIM_OC3PreloadConfig(ws2811_cfg->timer, TIM_OCPreload_Enable);
		break;
	case TIM_Channel_4:
		TIM_OC4Init(ws2811_cfg->timer, &TIM_OCInitStructure);
		TIM_OC4PreloadConfig(ws2811_cfg->timer, TIM_OCPreload_Enable);
		break;
	}

	TIM_CtrlPWMOutputs(ws2811_cfg->timer, ENABLE);

	DMA_DeInit(ws2811_cfg->dma_chan);

	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ws2811_cfg->timer->CCR1 + ws2811_cfg->timer_chan;
	DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)ws2811_dev->dma_buffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize         = ws2811_dev->buffer_size;
	DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
	DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode     = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	DMA_Init(ws2811_cfg->dma_chan, &DMA_InitStructure);

	TIM_DMACmd(ws2811_cfg->timer, ws2811_cfg->timer_dma_source, ENABLE);

	TIM_Cmd(ws2811_cfg->timer, ENABLE);

	PIOS_WS2811_set_all(ws2811_dev, 0, 0, 0);

	*dev_out = ws2811_dev;

	return 0;
}

void PIOS_WS2811_set(ws2811_dev_t dev, int led,
		uint8_t r, uint8_t g, uint8_t b)
{
	if (!dev) {
		return;
	}

	if (led >= dev->max_leds) {
		return;
	}

	int offset = led * WS2811_BITS_PER_LED;

	uint32_t grb = (g << 16) | (r << 8) | (b);

	for (int bit = (WS2811_BITS_PER_LED - 1); bit >= 0; --bit) {
		dev->dma_buffer[offset++] =
				(grb & (1 << bit)) ? BIT_COMPARE_1 : BIT_COMPARE_0;
	}
}

void PIOS_WS2811_trigger_update(ws2811_dev_t dev)
{
	if (!dev) {
		return;
	}

	if (dev->dma_active) {
		if (!DMA_GetFlagStatus(dev->config->dma_tcif)) {
			return;
		}

		dev->dma_active = false;	/* LOL */
		DMA_ClearFlag(dev->config->dma_tcif);
		DMA_Cmd(dev->config->dma_chan, DISABLE);
	}

	dev->dma_active = true;

	DMA_SetCurrDataCounter(dev->config->dma_chan, dev->buffer_size);
	DMA_Cmd(dev->config->dma_chan, ENABLE);
}

void PIOS_WS2811_set_all(ws2811_dev_t dev, uint8_t r, uint8_t g,
		uint8_t b)
{
	for (int i = 0; i < dev->max_leds; i++) {
		PIOS_WS2811_set(dev, i, r, g, b);
	}
}

int PIOS_WS2811_get_num_leds(ws2811_dev_t dev)
{
	return dev->max_leds;
}


