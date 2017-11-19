/**
 ******************************************************************************
 * @file       pios_dmashot.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DMAShot PiOS DMA-driven DShot driver
 * @{
 * @brief Generates DShot signal by updating timer CC registers via DMA bursts.
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

#include <pios.h>
#include "pios_dmashot.h"

#define MAX_TIMERS                              8

// This is to do half-word writes where appropriate. TIM2 and TIM5 are 32-bit, the rest
// are 16-bit. Can do full word writes to 16bit regardless, but it doubles DMA buffer size
// unnecessarily.
union dma_buffer {
	uint32_t ptr;
	uint32_t *fw;
	uint16_t *hw;
};

// Internal structure for timer and DShot configuration.
struct servo_timer {
	uint32_t sysclock;                                                              // Flava Flav! Timer's clock frequency.

	const struct pios_dmashot_timer_cfg *dma;                                       // DMA config
	const struct pios_tim_channel *servo_channels[4];                               // DMAShot configured channels

	// Tracking used TIM channels to allocate least memory.
	uint8_t low_channel;                                                            // Lowest registered TIM channel
	uint8_t high_channel;                                                           // Duh

	uint32_t dshot_freq;                                                            // Stores the desired frequency for
	                                                                                // timer initialization
	uint16_t duty_cycle_0;                                                          // Calculated duty cycle for 0-bit
	uint16_t duty_cycle_1;                                                          // And for 1-bit

	union dma_buffer buffer;                                                        // DMA buffer
	uint8_t dma_started;                                                            // Whether DMA transfers have been initiated

};

// DShot signal is 16-bit. Use a pause before and after to delimit signal and quell the timer CC
// when the message ends.
#define DMASHOT_MESSAGE_WIDTH                   16
#define DMASHOT_MESSAGE_PAUSE                   1
#define DMASHOT_STM32_BUFFER                    (DMASHOT_MESSAGE_PAUSE + DMASHOT_MESSAGE_WIDTH + DMASHOT_MESSAGE_PAUSE)

// Those values are in percent. These represent the Betaflight/BLHeli_s timing,
// instead of the spec one. Works with KISS RE 24A though.
#define DSHOT_DUTY_CYCLE_0                      36
#define DSHOT_DUTY_CYCLE_1                      74

#define TIMC_TO_INDEX(c)                        ((c)>>2)

const struct pios_dmashot_cfg *dmashot_cfg;
struct servo_timer **servo_timers;

// Whether a timer is 16- or 32-bit wide.
static inline bool PIOS_DMAShot_HalfWord(struct servo_timer *s_timer)
{
	// In STM32F4 and STM32L4, these two are 32-bit, the rest is 16-bit.
	return (s_timer->dma->timer == TIM2 || s_timer->dma->timer == TIM5 ? false : true);
}

static int PIOS_DMAShot_GetNumChannels(struct servo_timer *timer)
{
	int channels = timer->high_channel - timer->low_channel;
	if (channels < 0)
		return 0;
	return (channels >> 2) + 1;
}

void PIOS_DMAShot_Init(const struct pios_dmashot_cfg *config)
{
	dmashot_cfg = config;
}

/**
 * @brief Makes sure the DMAShot driver has allocated all internal structs.
 */
void PIOS_DMAShot_Prepare()
{
	if (dmashot_cfg) {
		if (!servo_timers) {
			// Allocate memory
			servo_timers = PIOS_malloc_no_dma(sizeof(servo_timers) * MAX_TIMERS);
			PIOS_Assert(servo_timers);

			memset(servo_timers, 0, sizeof(servo_timers) * MAX_TIMERS);
		}
		for (int i = 0; i < MAX_TIMERS; i++) {
			if (servo_timers[i]) {
				// Force sysclock to zero to consider this timer disabled,
				// so that DMAShot doesn't hijack it when an user configures from DMAShot
				// to something else.
				servo_timers[i]->sysclock = 0;
			}
		}
	}
}

// Returns the internal timer config. struct for a servo, if there is one.
static struct servo_timer *PIOS_DMAShot_GetServoTimer(const struct pios_tim_channel *servo_channel)
{
	if (!servo_timers)
		return NULL;

	for (int i = 0; i < MAX_TIMERS; i++) {
		struct servo_timer *s_timer = servo_timers[i];
		if (!s_timer || !s_timer->sysclock)
			continue;

		if (s_timer->dma->timer == servo_channel->timer)
			return s_timer;
	}

	return NULL;
}

void PIOS_DMAShot_WriteValue(const struct pios_tim_channel *servo_channel, uint16_t throttle)
{
	// Fail hard if trying to write values without configured DMAShot. We shouldn't be getting to
	// this point in that case.
	PIOS_Assert(dmashot_cfg);

	// If we can't find a timer for this, something's wrong. We shouldn't be getting to this point.
	// Fail hard.
	struct servo_timer *s_timer = PIOS_DMAShot_GetServoTimer(servo_channel);
	PIOS_Assert(s_timer);

	int shift = (servo_channel->timer_chan - s_timer->low_channel) >> 2;
	int channels = PIOS_DMAShot_GetNumChannels(s_timer);

	bool telem_bit = false;

	/* Set telem bit on commands */
	if ((throttle > 0) && (throttle < 48)) {
		telem_bit = true;
	}

	if (throttle > 2047)
		throttle = 2047;

	throttle <<= 5;

	if (telem_bit) {
		throttle |= 16;
	}

	throttle |=
			((throttle >> 4 ) & 0xf) ^
			((throttle >> 8 ) & 0xf) ^
			((throttle >> 12) & 0xf);

	// Leading zero, trailing zero.
	for (int i = DMASHOT_MESSAGE_PAUSE; i < DMASHOT_MESSAGE_WIDTH+DMASHOT_MESSAGE_PAUSE; i++) {
		int addr = i * channels + shift;
		if (PIOS_DMAShot_HalfWord(s_timer)) {
			s_timer->buffer.hw[addr] = throttle & 0x8000 ? s_timer->duty_cycle_1 : s_timer->duty_cycle_0;
		} else {
			s_timer->buffer.fw[addr] = throttle & 0x8000 ? s_timer->duty_cycle_1 : s_timer->duty_cycle_0;
		}
		throttle <<= 1;
	}
}

bool PIOS_DMAShot_RegisterTimer(TIM_TypeDef *timer, uint32_t clockrate, uint32_t dshot_freq)
{
	// No configuration, push for GPIO in upstream.
	if (!dmashot_cfg || !servo_timers)
		return false;

	// Check whether the timer is configured for DMA first. If not, bail and
	// tell upstairs that DMA DShot is a no-go.
	const struct pios_dmashot_timer_cfg *dma_config = NULL;

	for (int i = 0; i < dmashot_cfg->num_timers; i++) {
		if (dmashot_cfg->timer_cfg[i].timer == timer) {
			dma_config = &dmashot_cfg->timer_cfg[i];
		}
	}

	if (!dma_config)
		return false;

	// Check whether there's an existing config from a previous registration,
	// since we can't clean up due to a lack of free(). Timer/pin stuff is
	// static anyway.
	struct servo_timer *s_timer = NULL;

	bool found = false;
	for (int i = 0; i < MAX_TIMERS; i++) {
		if (servo_timers[i] && servo_timers[i]->dma->timer == timer) {
			s_timer = servo_timers[i];
			found = true;
			break;
		}
	}

	// Nothing found? Allocate a new slot for the timer.
	if (!found) {
		for (int i = 0; i < MAX_TIMERS; i++) {
			if (!servo_timers[i]) {
				s_timer = PIOS_malloc_no_dma(sizeof(*s_timer));
				// No point in returning false and falling back to GPIO, because that
				// also needs to allocate stuff later on, so we might just fail here.
				PIOS_Assert(s_timer);
				memset(s_timer, 0, sizeof(*s_timer));
				s_timer->low_channel = TIM_Channel_4;
				s_timer->high_channel = TIM_Channel_1;
				servo_timers[i] = s_timer;
				break;
			}
		}
	}

	// Why are we trying to register more than MAX_TIMERS, anyway?
	PIOS_Assert(s_timer);

	s_timer->sysclock = clockrate;
	s_timer->dshot_freq = dshot_freq;
	s_timer->dma = dma_config;

	return true;
}

bool PIOS_DMAShot_RegisterServo(const struct pios_tim_channel *servo_channel)
{
	// No configuration? kthxbye!
	if (!dmashot_cfg || !servo_timers)
		return false;

	struct servo_timer *s_timer = PIOS_DMAShot_GetServoTimer(servo_channel);

	// No timer found, tell upstream to GPIO!
	if (!s_timer)
		return false;

	if (s_timer->dma->master_timer && (PIOS_DMAShot_GetNumChannels(s_timer) > 0) &&
		(s_timer->low_channel != servo_channel->timer_chan)) {
		// I'm sorry, Dave, I'm afraid I cannot do that!

		// If DMA'ing directly to CCR, can only do one channel per timer.
		// Tell upstream to GPIO instead.
		return false;
	}

	if (servo_channel->timer_chan < s_timer->low_channel) s_timer->low_channel = servo_channel->timer_chan;
	if (servo_channel->timer_chan > s_timer->high_channel) s_timer->high_channel = servo_channel->timer_chan;

	s_timer->servo_channels[TIMC_TO_INDEX(servo_channel->timer_chan)] = servo_channel;

	return true;
}

void PIOS_DMAShot_Validate()
{
	if (!dmashot_cfg || !servo_timers)
		return;

	for (int i = 0; i < MAX_TIMERS; i++) {
		struct servo_timer *s_timer = servo_timers[i];
		if (!s_timer || !s_timer->sysclock)
			continue;

		// If after servo "registration", there's a low channel higher than the high
		// one, something went wrong, lets ignore the timer.
		if (s_timer->low_channel > s_timer->high_channel) {
			s_timer->sysclock = 0;
		}
	}
}

void PIOS_DMAShot_InitializeGPIOs()
{
	// If there's nothing setup, fail hard. We shouldn't be getting here.
	PIOS_Assert(dmashot_cfg && servo_timers);

	for (int i = 0; i < MAX_TIMERS; i++) {
		for (int j = 0; j < 4; j++) {

			struct servo_timer *s_timer = servo_timers[i];
			if (!s_timer || !s_timer->sysclock)
				continue;

			const struct pios_tim_channel *servo_channel = s_timer->servo_channels[j];
			if (servo_channel) {

				GPIO_InitTypeDef gpio_cfg = servo_channel->pin.init;
				gpio_cfg.GPIO_Speed = GPIO_High_Speed;
				GPIO_Init(servo_channel->pin.gpio, &gpio_cfg);

				GPIO_PinAFConfig(servo_channel->pin.gpio, servo_channel->pin.pin_source, servo_channel->remap);
			}
		}
	}
}

static void PIOS_DMAShot_TimerSetup(struct servo_timer *s_timer, uint32_t sysclock, uint32_t dshot_freq, TIM_OCInitTypeDef *ocinit, bool master)
{
	TIM_TypeDef *timer = master ? s_timer->dma->master_timer : s_timer->dma->timer;
	TIM_TimeBaseInitTypeDef timerdef;

	TIM_Cmd(timer, DISABLE);

	TIM_TimeBaseStructInit(&timerdef);

	timerdef.TIM_Prescaler = 0;
	timerdef.TIM_Period = sysclock / dshot_freq;
	timerdef.TIM_ClockDivision = TIM_CKD_DIV1;
	timerdef.TIM_RepetitionCounter = 0;
	timerdef.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(timer, &timerdef);

	// TIM_Channels are spread apart by 4 in stm32f4xx_tim.h
	for(int i = s_timer->low_channel; i <= s_timer->high_channel; i+=4)
	{
		switch(i)
		{
			case TIM_Channel_1:
				TIM_OC1Init(timer, ocinit);
				TIM_OC1PreloadConfig(timer, TIM_OCPreload_Enable);
				break;
			case TIM_Channel_2:
				TIM_OC2Init(timer, ocinit);
				TIM_OC2PreloadConfig(timer, TIM_OCPreload_Enable);
				break;
			case TIM_Channel_3:
				TIM_OC3Init(timer, ocinit);
				TIM_OC3PreloadConfig(timer, TIM_OCPreload_Enable);
				break;
			case TIM_Channel_4:
				TIM_OC4Init(timer, ocinit);
				TIM_OC4PreloadConfig(timer, TIM_OCPreload_Enable);
				break;
		}
	}

	// Do this, in case SyncPWM was configured before.
	TIM_SelectOnePulseMode(timer, TIM_OPMode_Repetitive);
}

void PIOS_DMAShot_InitializeTimers(TIM_OCInitTypeDef *ocinit)
{
	// If there's nothing setup, fail hard. We shouldn't be getting here.
	PIOS_Assert(dmashot_cfg && servo_timers);

	for (int i = 0; i < MAX_TIMERS; i++) {

		struct servo_timer *s_timer = servo_timers[i];
		if (!s_timer || !s_timer->sysclock)
			continue;

		if(PIOS_DMAShot_GetNumChannels(s_timer) == 0) {
			// Servo's should have been registered before InitializeTimers. If there's none,
			// disable the timer.
			s_timer->sysclock = 0;
			continue;
		}

		PIOS_DMAShot_TimerSetup(s_timer, s_timer->sysclock, s_timer->dshot_freq, ocinit, false);

		int f = s_timer->sysclock / s_timer->dshot_freq;

		s_timer->duty_cycle_0 = (f * DSHOT_DUTY_CYCLE_0 + 50) / 100;
		s_timer->duty_cycle_1 = (f * DSHOT_DUTY_CYCLE_1 + 50) / 100;

		if (s_timer->dma->master_timer)
			PIOS_DMAShot_TimerSetup(s_timer, s_timer->sysclock, s_timer->dshot_freq, ocinit, true);

		s_timer->dma_started = 0;
	}
}

static void PIOS_DMAShot_DMASetup(struct servo_timer *s_timer)
{
	DMA_Cmd(s_timer->dma->stream, DISABLE);
	while (DMA_GetCmdStatus(s_timer->dma->stream) == ENABLE) ;

	DMA_DeInit(s_timer->dma->stream);

	DMA_InitTypeDef dma;
	DMA_StructInit(&dma);

	dma.DMA_Channel = s_timer->dma->channel;
	dma.DMA_Memory0BaseAddr = s_timer->buffer.ptr;
	if (s_timer->dma->master_timer) {
		// The DMABase shift is in count of 32-bit registers. 16-bit registers are padded with 16-bit reserved ones,
		// and thus defacto 32-bit.
		dma.DMA_PeripheralBaseAddr = ((uint32_t)&s_timer->dma->timer->CR1) + ((s_timer->dma->master_config & 0x00FF) << 2);
	} else {
		dma.DMA_PeripheralBaseAddr = (uint32_t)(&s_timer->dma->timer->DMAR);
	}

	if (PIOS_DMAShot_HalfWord(s_timer)) {
		dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
		dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	} else {
		dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
		dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
	}

	dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;

	dma.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	dma.DMA_Mode = DMA_Mode_Normal;

	dma.DMA_BufferSize = PIOS_DMAShot_GetNumChannels(s_timer) * DMASHOT_STM32_BUFFER;

	dma.DMA_Priority = DMA_Priority_VeryHigh;
	dma.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
	dma.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

	dma.DMA_FIFOMode = DMA_FIFOMode_Enable;
	dma.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;

	DMA_Init(s_timer->dma->stream, &dma);

	// Don't do interrupts.
	DMA_ITConfig(s_timer->dma->stream, DMA_IT_TC, DISABLE);
}

// Allocates an aligned buffer for DMA burst transfers. Returns uint32_t, since
// that's the pointer type for Mem0 base address in DMA_InitTypeDef.
static uint32_t PIOS_DMAShot_AllocateBuffer(uint16_t size)
{
	size += 12;
	uint32_t ptr = (uint32_t)PIOS_malloc(size);

	// Blow up if we didn't get a buffer.
	PIOS_Assert(ptr);

	memset((void*)ptr, 0, size);

	// Align to 16-byte boundary.
	uint32_t shift = 16 - (ptr % 16);
	if(shift != 16) ptr += shift;

	return ptr;
}

void PIOS_DMAShot_InitializeDMAs()
{
	// If there's nothing setup, fail hard. We shouldn't be getting here.
	PIOS_Assert(dmashot_cfg && servo_timers);

	for (int i = 0; i < MAX_TIMERS; i++) {
		struct servo_timer *s_timer = servo_timers[i];
		if (!s_timer || !s_timer->sysclock)
			continue;

		if (!s_timer->buffer.ptr) {
			// Allocate buffer at timer resolution.
			s_timer->buffer.ptr = PIOS_DMAShot_AllocateBuffer(
					PIOS_DMAShot_GetNumChannels(s_timer) * DMASHOT_STM32_BUFFER *
					(PIOS_DMAShot_HalfWord(s_timer) ? sizeof(uint16_t) : sizeof(uint32_t))
				);
		}

		PIOS_DMAShot_DMASetup(s_timer);
	}
}

void PIOS_DMAShot_TriggerUpdate()
{
	// If there's nothing setup, fail hard. We shouldn't be getting here.
	PIOS_Assert(dmashot_cfg && servo_timers);

	for (int i = 0; i < MAX_TIMERS; i++) {
		struct servo_timer *s_timer = servo_timers[i];
		if (!s_timer || !s_timer->sysclock)
			continue;

		// Wait for DMA to finish.
		if(s_timer->dma_started) {
			while(DMA_GetFlagStatus(s_timer->dma->stream, s_timer->dma->tcif) != SET) ;
		}

		DMA_Cmd(s_timer->dma->stream, DISABLE);
		while (DMA_GetCmdStatus(s_timer->dma->stream) == ENABLE) ;

		if (s_timer->dma->master_timer) {
			TIM_DMACmd(s_timer->dma->master_timer,
					// This results in a typical event source value
					s_timer->dma->master_config & 0xFF00,
					DISABLE);
			TIM_Cmd(s_timer->dma->master_timer, DISABLE);
			TIM_SetCounter(s_timer->dma->master_timer, 0);
		} else {
			TIM_DMACmd(s_timer->dma->timer, TIM_DMA_Update, DISABLE);
		}

		TIM_Cmd(s_timer->dma->timer, DISABLE);
		TIM_SetCounter(s_timer->dma->timer, 0);

		DMA_ClearFlag(s_timer->dma->stream, s_timer->dma->tcif);
		DMA_SetCurrDataCounter(s_timer->dma->stream, PIOS_DMAShot_GetNumChannels(s_timer) * DMASHOT_STM32_BUFFER);
	}

	// Re-enable the timers and DMA in a separate loop, to make the signals line up better.
	for (int i = 0; i < MAX_TIMERS; i++) {
		struct servo_timer *s_timer = servo_timers[i];
		if (!s_timer || !s_timer->sysclock)
			continue;

		if (s_timer->dma->master_timer) {
			TIM_DMACmd(s_timer->dma->master_timer,
					// This results in a typical event source value
					s_timer->dma->master_config & 0xFF00,
					ENABLE);
			TIM_Cmd(s_timer->dma->timer, ENABLE);
			TIM_Cmd(s_timer->dma->master_timer, ENABLE);
		} else {
			int offset = s_timer->low_channel >> 2;
			int transfers = (s_timer->high_channel - s_timer->low_channel) >> 2;
			TIM_DMAConfig(s_timer->dma->timer, TIM_DMABase_CCR1 + offset, transfers << 8);

			TIM_DMACmd(s_timer->dma->timer, TIM_DMA_Update, ENABLE);
			TIM_Cmd(s_timer->dma->timer, ENABLE);
		}

		DMA_Cmd(s_timer->dma->stream, ENABLE);
		s_timer->dma_started = 1;
	}
}

bool PIOS_DMAShot_IsReady()
{
	if (!dmashot_cfg || !servo_timers)
		return false;

	for (int i = 0; i < MAX_TIMERS; i++) {
		struct servo_timer *s_timer = servo_timers[i];
		if (s_timer && s_timer->sysclock) return true;
	}

	return false;
}

bool PIOS_DMAShot_IsConfigured()
{
	return dmashot_cfg != NULL;
}
