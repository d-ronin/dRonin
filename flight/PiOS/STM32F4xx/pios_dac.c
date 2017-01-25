/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DAC Driver for WS2811 LEDs.
 * @brief Driver for DAC output.  Uses a timer (usually TIM6), a DMA channel,
 * and a DAC.
 * @{
 *
 * @file       pios_dac.c
 * @author     dRonin, http://dRonin.org, Copyright (C) 2017
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

#include "pios_config.h"

#if defined(PIOS_INCLUDE_DAC)

#include "pios.h"
#include <pios_stm32.h>
#include "stm32f4xx_tim.h"
#include "pios_tim_priv.h"

#include "misc_math.h"

#include "pios_dac.h"

// Sample rate we'll generally use is 24KHz; this is both good enough for audio,
// and results in 4800bps smartaudio being 5 samples per bit.  1200 BFSK
// telemetry is a luxurious 20 samples/bit.
//
// Samples are 16 bits wide, so a 120 sample buffer is 240 bytes long (480b
// double buffered), carries 6 bits, and results in an interrupt rate of 200Hz
#define DAC_DMA_SAMPLE_COUNT 120

typedef bool (*fill_dma_cb)(void *ctx, uint16_t *buf, int len);

struct dac_dev_s {
#define DAC_MAGIC 0x61434144		/* 'DACa' */
	uint32_t magic;

	const struct pios_dac_cfg *cfg;

	// These are the double buffered bufs.  
	uint16_t dma_buf_0[DAC_DMA_SAMPLE_COUNT];
	uint16_t dma_buf_1[DAC_DMA_SAMPLE_COUNT];

	// Updated from, but only read from, interrupt handler.
	bool cur_buf;

	volatile bool in_progress;

	volatile uint8_t priority;
	volatile fill_dma_cb dma_cb;
	volatile void *ctx;

	uint16_t phase_accum;	// XXX tmp
};

static void dac_cue_dma(dac_dev_t dev);

int PIOS_DAC_init(dac_dev_t *dev_out, const struct pios_dac_cfg *cfg) 
{
	dac_dev_t dev = PIOS_malloc(sizeof(*dev));

	if (!dev) {
		return -1;
	}

	*dev = (struct dac_dev_s) {
		.magic = DAC_MAGIC,
		.cfg = cfg,
	};

	TIM_DeInit(cfg->timer);
	TIM_Cmd(cfg->timer, DISABLE);

	TIM_TimeBaseInit(cfg->timer,
			(TIM_TimeBaseInitTypeDef *) &cfg->clock_cfg);

	// Disable all timer interrupts, in case someone inited this timer
	// before.
	TIM_ITConfig(cfg->timer, TIM_IT_Update | TIM_IT_CC1 | TIM_IT_CC2 |
			TIM_IT_CC3 | TIM_IT_CC4 | TIM_IT_COM | TIM_IT_Trigger |
			TIM_IT_Break, DISABLE);

	// Setup a trigger output, get the timer spinning
	TIM_SelectOutputTrigger(cfg->timer, TIM_TRGOSource_Update);

	TIM_Cmd(cfg->timer, ENABLE);

	// Set up the DMA interrupt
	NVIC_Init((NVIC_InitTypeDef *)(&dev->cfg->interrupt));

	*dev_out = dev;

	return 0;
}

bool PIOS_DAC_install_callback(dac_dev_t dev, uint8_t priority, fill_dma_cb cb,
		void *ctx) {
	PIOS_Assert(dev->magic == DAC_MAGIC);

	PIOS_IRQ_Disable();

	if (dev->in_progress) {
		bool ret_val = false;

		if (dev->priority < priority) {
			// Preempt existing transmitter

			dev->dma_cb = cb;
			dev->ctx = ctx;
			dev->priority = priority;
			ret_val = true;
		}

		PIOS_IRQ_Enable();

		return ret_val;
	}

	dev->dma_cb = cb;
	dev->ctx = ctx;
	dev->priority = priority;

	/* Fill initial sample buffers with half-scale */
	for (int i = 0; i < DAC_DMA_SAMPLE_COUNT; i++) {
		dev->dma_buf_0[i] = 32767;
		dev->dma_buf_1[i] = 32767;
	}

	/* Establish DMA */
	dac_cue_dma(dev);

	dev->in_progress = true;

	PIOS_IRQ_Enable();

	return true;
}

static void dac_cue_dma(dac_dev_t dev) {
	/* We init / seize the GPIO here, because it's possible someone else has
	 * been running the port the other way for bidir types of things. */
	GPIO_InitTypeDef gpio_cfg = {
		.GPIO_Pin = dev->cfg->gpio_pin,
		.GPIO_Mode = GPIO_Mode_AN,
		.GPIO_PuPd = GPIO_PuPd_NOPULL,
	};

	GPIO_Init(dev->cfg->gpio, &gpio_cfg);

	DAC_InitTypeDef dac_cfg = {
		.DAC_Trigger = dev->cfg->dac_trigger,
		.DAC_WaveGeneration = DAC_WaveGeneration_None,
		.DAC_OutputBuffer = DAC_OutputBuffer_Enable,
	};

	DAC_Init(dev->cfg->dac_channel, &dac_cfg);
	DAC_Cmd(dev->cfg->dac_channel, ENABLE);
	DAC_DMACmd(dev->cfg->dac_channel, ENABLE);

	dev->cur_buf = false;
	dev->in_progress = true;

	DMA_Cmd(dev->cfg->dma_stream, DISABLE);

	DMA_DeInit(dev->cfg->dma_stream);

	DMA_InitTypeDef dma_init;

	DMA_StructInit(&dma_init);

	dma_init.DMA_Channel = dev->cfg->dma_channel;
	dma_init.DMA_PeripheralBaseAddr = (uintptr_t) dev->cfg->dac_outreg;
	dma_init.DMA_Memory0BaseAddr = (uintptr_t) dev->dma_buf_0;
	dma_init.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;

	dma_init.DMA_BufferSize = DAC_DMA_SAMPLE_COUNT;
	dma_init.DMA_Mode = DMA_Mode_Circular;
	dma_init.DMA_Priority = DMA_Priority_High;
	dma_init.DMA_FIFOMode = DMA_FIFOMode_Enable;
	dma_init.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	dma_init.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
	dma_init.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

	/* Now the tricky one-- set up the double-buffered DMA, to blit to
	 * BSRRH and clear bits at the appropriate time */

	DMA_Init(dev->cfg->dma_stream, &dma_init);

	DMA_DoubleBufferModeConfig(dev->cfg->dma_stream,
			(uintptr_t) dev->dma_buf_1,
			DMA_Memory_0);

	DMA_DoubleBufferModeCmd(dev->cfg->dma_stream, ENABLE);

	DMA_ClearITPendingBit(dev->cfg->dma_stream,
			dev->cfg->dma_tcif);

	DMA_ITConfig(dev->cfg->dma_stream,
			DMA_IT_TC, ENABLE);

	DMA_Cmd(dev->cfg->dma_stream, ENABLE);
}

void PIOS_DAC_dma_interrupt_handler(dac_dev_t dev)
{
	DMA_ClearITPendingBit(dev->cfg->dma_stream, dev->cfg->dma_tcif);
	dev->cur_buf = DMA_GetCurrentMemoryTarget(dev->cfg->dma_stream);

	uint16_t *buf = dev->cur_buf ? dev->dma_buf_0 : dev->dma_buf_1;

	bool in_prog;

	in_prog = dev->dma_cb((void *)dev->ctx, buf, DAC_DMA_SAMPLE_COUNT);

	if (!in_prog) {
		// When a callback declares completion, something between the
		// last 2 to last 0 buffers is lost / not blitted
		DMA_Cmd(dev->cfg->dma_stream, DISABLE);
		dev->in_progress = false;
	}
}

#endif /* PIOS_INCLUDE_DAC */
