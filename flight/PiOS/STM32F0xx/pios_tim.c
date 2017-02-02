/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_TIM Timer Functions
 * @brief PIOS Timer control code
 * @{
 *
 * @file       pios_tim.c  
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @brief      Sets up timers and ways to register callbacks on them
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


#include "pios.h"

#include "pios_tim.h"
#include "pios_tim_priv.h"

enum pios_tim_dev_magic {
	PIOS_TIM_DEV_MAGIC = 0x87654098,
};

struct pios_tim_dev {
	enum pios_tim_dev_magic     magic;

	const struct pios_tim_channel * channels;
	uint8_t num_channels;

	const struct pios_tim_callbacks * callbacks;
	uintptr_t context;
};

static struct pios_tim_dev pios_tim_devs[PIOS_TIM_MAX_DEVS];
static uint8_t pios_tim_num_devs;
static struct pios_tim_dev * PIOS_TIM_alloc(void)
{
	struct pios_tim_dev * tim_dev;

	if (pios_tim_num_devs >= PIOS_TIM_MAX_DEVS) {
		return (NULL);
	}

	tim_dev = &pios_tim_devs[pios_tim_num_devs++];
	tim_dev->magic = PIOS_TIM_DEV_MAGIC;

	return (tim_dev);
}

int32_t PIOS_TIM_InitClock(const struct pios_tim_clock_cfg * cfg)
{
	PIOS_DEBUG_Assert(cfg);

	/* Configure the dividers for this timer */
	TIM_TimeBaseInit(cfg->timer, (TIM_TimeBaseInitTypeDef*)cfg->time_base_init);

	/* Configure internal timer clocks */
	TIM_InternalClockConfig(cfg->timer);

	/* Enable timers */
	TIM_Cmd(cfg->timer, ENABLE);

	/* Enable Interrupts */
	NVIC_Init((NVIC_InitTypeDef*)&cfg->irq.init);

	/* Check for optional second vector (dirty hack)
	 * This is needed for timers 1 when requiring more than one event
	 * to generate an interrupt. 
	 */
	if (cfg->irq2.init.NVIC_IRQChannel != 0)
		NVIC_Init((NVIC_InitTypeDef*)&cfg->irq2.init);

	return 0;
}

void PIOS_TIM_InitAllTimerPins(uintptr_t tim_id)
{
	struct pios_tim_dev * tim_dev = (struct pios_tim_dev *) tim_id;

	/* Configure the pins */
	for (uint8_t i = 0; i < tim_dev->num_channels; i++) {
		const struct pios_tim_channel * chan = &tim_dev->channels[i];

		GPIO_Init(chan->pin.gpio, (GPIO_InitTypeDef*)&chan->pin.init);

		PIOS_DEBUG_Assert(chan->remap);

		GPIO_PinAFConfig(chan->pin.gpio, chan->pin.pin_source, chan->remap);
	}
}

void PIOS_TIM_SetBankToGPOut(uintptr_t tim_id, TIM_TypeDef *timer)
{
	struct pios_tim_dev * tim_dev = (struct pios_tim_dev *) tim_id;

	GPIO_InitTypeDef gpio_inf;
	gpio_inf.GPIO_Speed = GPIO_Speed_10MHz;
	gpio_inf.GPIO_Mode = GPIO_Mode_OUT;
	gpio_inf.GPIO_OType = GPIO_OType_PP;
	gpio_inf.GPIO_PuPd = GPIO_PuPd_DOWN;

	for (uint8_t i = 0; i < tim_dev->num_channels; i++) {
		const struct pios_tim_channel * chan = &tim_dev->channels[i];

		if (chan->timer != timer) {
			continue;
		}

		// Only steal the pin info from the matching pin
		gpio_inf.GPIO_Pin = chan->pin.init.GPIO_Pin;
		GPIO_Init(chan->pin.gpio, &gpio_inf);
	}
}

int32_t PIOS_TIM_InitChannels(uintptr_t * tim_id, const struct pios_tim_channel * channels, uint8_t num_channels, const struct pios_tim_callbacks * callbacks, uintptr_t context)
{
	PIOS_Assert(channels);
	PIOS_Assert(num_channels);

	struct pios_tim_dev * tim_dev;
	tim_dev = (struct pios_tim_dev *) PIOS_TIM_alloc();
	if (!tim_dev) goto out_fail;

	/* Bind the configuration to the device instance */
	tim_dev->channels = channels;
	tim_dev->num_channels = num_channels;
	tim_dev->callbacks = callbacks;
	tim_dev->context = context;

	*tim_id = (uintptr_t)tim_dev;

	PIOS_TIM_InitAllTimerPins(*tim_id);

	return(0);

out_fail:
	return(-1);
}

static void PIOS_TIM_generic_irq_handler(TIM_TypeDef * timer)
{
	/* Iterate over all registered clients of the TIM layer to find channels on this timer */
	for (uint8_t i = 0; i < pios_tim_num_devs; i++) {
		const struct pios_tim_dev * tim_dev = &pios_tim_devs[i];

		if (!tim_dev->channels || tim_dev->num_channels == 0) {
			/* No channels to process on this client */
			continue;
		}

		/* Check for an overflow event on this timer */
		bool overflow_event;
		uint16_t overflow_count;
		if (TIM_GetITStatus(timer, TIM_IT_Update) == SET) {
			TIM_ClearITPendingBit(timer, TIM_IT_Update);
			overflow_count = timer->ARR;
			overflow_event = true;
		} else {
			overflow_count = 0;
			overflow_event = false;
		}

		for (uint8_t j = 0; j < tim_dev->num_channels; j++) {
			const struct pios_tim_channel * chan = &tim_dev->channels[j];

			if (chan->timer != timer) {
				/* channel is not on this timer */
				continue;
			}

			/* Figure out which interrupt bit we should be looking at */
			uint16_t timer_it;
			switch (chan->timer_chan) {
			case TIM_Channel_1:
				timer_it = TIM_IT_CC1;
				break;
			case TIM_Channel_2:
				timer_it = TIM_IT_CC2;
				break;
			case TIM_Channel_3:
				timer_it = TIM_IT_CC3;
				break;
			case TIM_Channel_4:
				timer_it = TIM_IT_CC4;
				break;
			default:
				PIOS_Assert(0);
				break;
			}

			bool edge_event;
			uint16_t edge_count;
			if (TIM_GetITStatus(chan->timer, timer_it) == SET) {
				TIM_ClearITPendingBit(chan->timer, timer_it);

				/* Read the current counter */
				switch(chan->timer_chan) {
				case TIM_Channel_1:
					edge_count = TIM_GetCapture1(chan->timer);
					break;
				case TIM_Channel_2:
					edge_count = TIM_GetCapture2(chan->timer);
					break;
				case TIM_Channel_3:
					edge_count = TIM_GetCapture3(chan->timer);
					break;
				case TIM_Channel_4:
					edge_count = TIM_GetCapture4(chan->timer);
					break;
				default:
					PIOS_Assert(0);
					break;
				}
				edge_event = true;
			} else {
				edge_event = false;
				edge_count = 0;
			}

			if (!tim_dev->callbacks) {
				/* No callbacks registered, we're done with this channel */
				continue;
			}

			/* Generate the appropriate callbacks */
			if (overflow_event & edge_event) {
				/*
				 * When both edge and overflow happen in the same interrupt, we
				 * need a heuristic to determine the order of the edge and overflow
				 * events so that the callbacks happen in the right order.  If we
				 * get the order wrong, our pulse width calculations could be off by up
				 * to ARR ticks.  That could be bad.
				 *
				 * Heuristic: If the edge_count is < 16 ticks above zero then we assume the
				 *            edge happened just after the overflow.
				 */

				if (edge_count < 16) {
					/* Call the overflow callback first */
					if (tim_dev->callbacks->overflow) {
						(*tim_dev->callbacks->overflow)((uintptr_t)tim_dev,
									tim_dev->context,
									j,
									overflow_count);
					}
					/* Call the edge callback second */
					if (tim_dev->callbacks->edge) {
						(*tim_dev->callbacks->edge)((uintptr_t)tim_dev,
									tim_dev->context,
									j,
									edge_count);
					}
				} else {
					/* Call the edge callback first */
					if (tim_dev->callbacks->edge) {
						(*tim_dev->callbacks->edge)((uintptr_t)tim_dev,
									tim_dev->context,
									j,
									edge_count);
					}
					/* Call the overflow callback second */
					if (tim_dev->callbacks->overflow) {
						(*tim_dev->callbacks->overflow)((uintptr_t)tim_dev,
									tim_dev->context,
									j,
									overflow_count);
					}
				}
			} else if (overflow_event && tim_dev->callbacks->overflow) {
				(*tim_dev->callbacks->overflow)((uintptr_t)tim_dev,
								tim_dev->context,
								j,
								overflow_count);
			} else if (edge_event && tim_dev->callbacks->edge) {
				(*tim_dev->callbacks->edge)((uintptr_t)tim_dev,
							tim_dev->context,
							j,
							edge_count);
			}
		}
	}
}

/* Bind Interrupt Handlers
 *
 * Map all valid TIM IRQs to the common interrupt handler
 * and give it enough context to properly demux the various timers
 */
void TIM1_CC_IRQHandler(void) __attribute__ ((alias ("PIOS_TIM_1_irq_handler")));
void TIM1_BRK_UP_TRG_COM_IRQHandler(void) __attribute__ ((alias ("PIOS_TIM_1_irq_handler")));
static void PIOS_TIM_1_irq_handler (void)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	PIOS_TIM_generic_irq_handler (TIM1);

#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
}


void TIM2_IRQHandler(void) __attribute__ ((alias ("PIOS_TIM_2_irq_handler")));
static void PIOS_TIM_2_irq_handler (void)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	PIOS_TIM_generic_irq_handler (TIM2);

#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
}

void TIM3_IRQHandler(void) __attribute__ ((alias ("PIOS_TIM_3_irq_handler")));
static void PIOS_TIM_3_irq_handler (void)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	PIOS_TIM_generic_irq_handler (TIM3);

#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
}

void TIM6_IRQHandler(void) __attribute__ ((alias ("PIOS_TIM_6_irq_handler")));
static void PIOS_TIM_6_irq_handler (void)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	PIOS_TIM_generic_irq_handler (TIM6);

#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
}

void TIM14_IRQHandler(void) __attribute__ ((alias ("PIOS_TIM_14_irq_handler")));
static void PIOS_TIM_14_irq_handler (void)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	PIOS_TIM_generic_irq_handler (TIM14);

#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
}

void TIM15_IRQHandler(void) __attribute__ ((alias ("PIOS_TIM_15_irq_handler")));
static void PIOS_TIM_15_irq_handler (void)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	PIOS_TIM_generic_irq_handler (TIM15);

#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
}

void TIM16_IRQHandler(void) __attribute__ ((alias ("PIOS_TIM_16_irq_handler")));
static void PIOS_TIM_16_irq_handler (void)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	PIOS_TIM_generic_irq_handler (TIM16);

#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
}

void TIM17_IRQHandler(void) __attribute__ ((alias ("PIOS_TIM_17_irq_handler")));
static void PIOS_TIM_17_irq_handler (void)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	PIOS_TIM_generic_irq_handler (TIM17);

#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
}
/**
 * @}
 * @}
 */
