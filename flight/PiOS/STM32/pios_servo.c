/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SERVO RC Servo Functions
 * @brief Code to do set RC servo output
 * @{
 *
 * @file       pios_servo.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014-2015
 * @author     dRonin, http://dRonin.org, Copyright (C) 2016
 * @brief      RC Servo routines (STM32 dependent)
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

/* Project Includes */
#include "pios.h"
#include "pios_servo_priv.h"
#include "pios_tim_priv.h"

#include <pios_dio.h>

#ifndef STM32F0XX	// No high-res delay on F0 yet
#include "pios_inlinedelay.h"
#include "pios_irq.h"
#endif

#if defined(PIOS_INCLUDE_DMASHOT)
#include "pios_dmashot.h"
#endif

#include "misc_math.h"

/* Private variables */
static const struct pios_servo_cfg *servo_cfg;

static uint32_t channel_mask = 0;

//! The counter rate for the channel, used to calculate compare values.

enum dshot_gpio {
	DS_GPIOA = 0,
	DS_GPIOB,
	DS_GPIOC,
	DS_GPIO_NUMVALS
} __attribute__((packed));

enum channel_mode {
	UNCONFIGURED = 0,
	REGULAR_PWM,
	REGULAR_INVERTED_PWM,
	SYNC_PWM,
	SYNC_DSHOT_300,
	SYNC_DSHOT_600,
	SYNC_DSHOT_1200,
	SYNC_DSHOT_DMA
} __attribute__((packed));

struct dshot_info {
	uint16_t value;
	uint16_t pin;
	enum dshot_gpio gpio;
};

struct output_channel {
	enum channel_mode mode;

	union {
		uint32_t pwm_res;
		struct dshot_info dshot;
	} i;
} __attribute__((packed));

static struct output_channel *output_channels;

static uintptr_t servo_tim_id;

static bool resetting;

static bool dshot_in_use;

/* Private function prototypes */
static uint32_t timer_apb_clock(TIM_TypeDef *timer);
static uint32_t max_timer_clock(TIM_TypeDef *timer);

/**
* Initialise Servos
*/
int32_t PIOS_Servo_Init(const struct pios_servo_cfg *cfg)
{
	PIOS_Assert(!servo_tim_id);		// Make sure not inited already

	if (PIOS_TIM_InitChannels(&servo_tim_id, cfg->channels, cfg->num_channels, NULL, 0)) {
		return -1;
	}

	/* Store away the requested configuration */
	servo_cfg = cfg;

	/* Configure the channels to be in output compare mode */
	for (uint8_t i = 0; i < cfg->num_channels; i++) {
		const struct pios_tim_channel *chan = &cfg->channels[i];

		/* Set up for output compare function */
		switch(chan->timer_chan) {
			case TIM_Channel_1:
				TIM_OC1Init(chan->timer, (TIM_OCInitTypeDef *)&cfg->tim_oc_init);
				TIM_OC1PreloadConfig(chan->timer, TIM_OCPreload_Enable);
				break;
			case TIM_Channel_2:
				TIM_OC2Init(chan->timer, (TIM_OCInitTypeDef *)&cfg->tim_oc_init);
				TIM_OC2PreloadConfig(chan->timer, TIM_OCPreload_Enable);
				break;
			case TIM_Channel_3:
				TIM_OC3Init(chan->timer, (TIM_OCInitTypeDef *)&cfg->tim_oc_init);
				TIM_OC3PreloadConfig(chan->timer, TIM_OCPreload_Enable);
				break;
			case TIM_Channel_4:
				TIM_OC4Init(chan->timer, (TIM_OCInitTypeDef *)&cfg->tim_oc_init);
				TIM_OC4PreloadConfig(chan->timer, TIM_OCPreload_Enable);
				break;
		}

		TIM_ARRPreloadConfig(chan->timer, ENABLE);
		TIM_CtrlPWMOutputs(chan->timer, ENABLE);
		TIM_Cmd(chan->timer, ENABLE);
	}

	output_channels = PIOS_malloc(servo_cfg->num_channels * sizeof(*output_channels));
	if (output_channels == NULL) {
		return -1;
	}
	memset(output_channels, 0, servo_cfg->num_channels * sizeof(*output_channels));

	return 0;
}

void PIOS_Servo_DisableChannel(int channel)
{
	channel_mask |= 1 << channel;
}

int PIOS_Servo_GetPins(dio_tag_t *dios, int max_dio)
{
	int i;

	for (i = 0; (i < servo_cfg->num_channels) && (i < max_dio); i++) {
		if (channel_mask & (1 << i)) {
			dios[i] = DIO_NULL;
			continue;
		}

		dios[i] = DIO_MAKE_TAG(servo_cfg->channels[i].pin.gpio,
				servo_cfg->channels[i].pin.init.GPIO_Pin);
	}

	return i;
}

struct timer_bank {
	TIM_TypeDef *timer;
	uint32_t clk_rate;
	uint16_t max_pulse;
	uint16_t prescaler;
	uint16_t period;
};

static void ChannelSetup_DShot(int j, uint16_t rate)
{
	switch (rate) {
		case SHOT_DSHOT300:
			output_channels[j].mode = SYNC_DSHOT_300;
			break;
		case SHOT_DSHOT600:
			output_channels[j].mode = SYNC_DSHOT_600;
			break;
		case SHOT_DSHOT1200:
			output_channels[j].mode = SYNC_DSHOT_1200;
			break;
		default:
			PIOS_Assert(false);
	};

	struct dshot_info *ds_info = &output_channels[j].i.dshot;

	ds_info->value = 0;

	switch ((uintptr_t) servo_cfg->channels[j].pin.gpio) {
		case (uintptr_t) GPIOA:
			ds_info->gpio = DS_GPIOA;
			break;
		case (uintptr_t) GPIOB:
			ds_info->gpio = DS_GPIOB;
			break;
		case (uintptr_t) GPIOC:
			ds_info->gpio = DS_GPIOC;
			break;
		default:
			PIOS_Assert(false);
	}

	ds_info->pin = servo_cfg->channels[j].pin.init.GPIO_Pin;
}

static int BankSetup_DShot(struct timer_bank *bank)
{
	/*
	 * Relatively simple configuration here.  Just seize it!
	 */
	PIOS_TIM_SetBankToGPOut(servo_tim_id, bank->timer);

	return 0;
}

static int BankSetup_OneShot(struct timer_bank *bank, int32_t max_tim_clock)
{
	/* 
	 * Ensure a dead time of 2% + 10 us, e.g. 15us for 250us
	 * long pulses, 50 us for 2000us long pulses
	 */
	float period = 1.02f * bank->max_pulse + 10.0f;
	float num_ticks = period / 1e6f * max_tim_clock;
	// assume 16-bit timer
	bank->prescaler = num_ticks / 0xffff + 0.5f;
	bank->clk_rate = max_tim_clock / (bank->prescaler + 1);
	bank->period = period / 1e6f * bank->clk_rate;

	// select one pulse mode for SyncPWM
	TIM_SelectOnePulseMode(bank->timer, TIM_OPMode_Single);
	// Need to invert output polarity for one-pulse mode
	uint16_t inverted_polarity = (servo_cfg->tim_oc_init.TIM_OCPolarity == TIM_OCPolarity_Low) ?
		TIM_OCPolarity_High : TIM_OCPolarity_Low;
	TIM_OC1PolarityConfig(bank->timer, inverted_polarity);
	TIM_OC2PolarityConfig(bank->timer, inverted_polarity);
	TIM_OC3PolarityConfig(bank->timer, inverted_polarity);
	TIM_OC4PolarityConfig(bank->timer, inverted_polarity);

	return 0;
}

static int BankSetup_PWM(struct timer_bank *bank, int32_t max_tim_clock,
		uint16_t rate)
{
	// assume 16-bit timer
	if (servo_cfg->force_1MHz) {
		bank->prescaler = max_tim_clock / 1000000 - 1;
	} else {
		float num_ticks = (float)max_tim_clock / (float)rate;

		bank->prescaler = num_ticks / 0xffff + 0.5f;
	}

	bank->clk_rate = max_tim_clock / (bank->prescaler + 1);
	bank->period = (float)bank->clk_rate / (float)rate;

	// de-select one pulse mode in case SyncPWM was previously used
	TIM_SelectOnePulseMode(bank->timer, TIM_OPMode_Repetitive);

	// and make sure the output is enabled
	TIM_Cmd(bank->timer, ENABLE);

	// restore polarity in case one-pulse was used
	TIM_OC1PolarityConfig(bank->timer, servo_cfg->tim_oc_init.TIM_OCPolarity);
	TIM_OC2PolarityConfig(bank->timer, servo_cfg->tim_oc_init.TIM_OCPolarity);
	TIM_OC3PolarityConfig(bank->timer, servo_cfg->tim_oc_init.TIM_OCPolarity);
	TIM_OC4PolarityConfig(bank->timer, servo_cfg->tim_oc_init.TIM_OCPolarity);

	return 0;
}

/**
 * @brief PIOS_Servo_SetMode Sets the PWM output frequency and resolution.
 * An output rate of 0 indicates Synchronous updates (e.g. SyncPWM/OneShot), otherwise
 * normal PWM at the specified output rate. SyncPWM uses hardware one-pulse mode.
 * Timer prescaler and related parameters are calculated based on the channel
 * with highest max pulse length on each timer. A deadtime is provided to
 * ensure SyncPWM pulses cannot merge.
 * The information required to convert from us to compare value is cached
 * for each channel (not timer) to facilitate PIOS_Servo_Set
 * @param out_rate array of output rate in Hz, banks elements
 * @param banks maximum number of banks
 * @param channel_max array of max pulse lengths, number of channels elements
 */
int PIOS_Servo_SetMode(const uint16_t *out_rate, const int banks, const uint16_t *channel_max, const uint16_t *channel_min)
{
	if (!servo_cfg || banks > PIOS_SERVO_MAX_BANKS) {
		return -10;
	}

#if defined(PIOS_INCLUDE_DMASHOT)
	if (PIOS_DMAShot_IsConfigured()){
		PIOS_DMAShot_Prepare();
	}
#endif

	dshot_in_use = false;

	struct timer_bank timer_banks[PIOS_SERVO_MAX_BANKS];

	memset(&timer_banks, 0, sizeof(timer_banks));
	int banks_found = 0;

	// find max pulse length for each bank
	for (int i = 0; i < servo_cfg->num_channels; i++) {
		if (channel_mask & (1 << i)) {
			continue;
		}

		PIOS_TIM_InitTimerPin(servo_tim_id, i);

		int bank = -1;
		const struct pios_tim_channel *chan = &servo_cfg->channels[i];

		for (int j = 0; j < banks_found; j++) {
			if (timer_banks[j].timer == chan->timer)
				bank = j;
		}

		if (bank < 0)
			bank = banks_found++;

		timer_banks[bank].timer = chan->timer;
		timer_banks[bank].max_pulse = MAX(timer_banks[bank].max_pulse, channel_max[i]);
		timer_banks[bank].max_pulse = MAX(timer_banks[bank].max_pulse, channel_min[i]);
	}

	// configure timers/banks
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = servo_cfg->tim_base_init;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	for (int i = 0; i < banks_found; i++) {
		// Skip the bank if no outputs are configured
		if (timer_banks[i].max_pulse == 0)
			continue;

		/* Calculate the maximum clock frequency for the timer */
		uint32_t max_tim_clock = max_timer_clock(timer_banks[i].timer);
		// check if valid timer
		if (!max_tim_clock) {
			return -20;
		}

		uint16_t rate = out_rate[i];

		if (servo_cfg->force_1MHz && (rate == SHOT_ONESHOT)) {
			/* We've been asked for syncPWM but are in a config
			 * where we can't do it.  This means CC3D + 333Hz.
			 * Getting fast output is functionally identical
			 * to oneshot on a target like this.  Pick a period
			 * that has a lot of deadtime and call it good.
			 */

			/* Works out to 2500Hz at normal 250us max pulse,
			 * 400Hz at 2000us.
			 *
			 * Put one more way, with "oneshot", 400us of variable
			 * delay on 3300us control period.
			 */

			rate = 1000000 / (timer_banks[i].max_pulse +
					timer_banks[i].max_pulse / 5 +
					100);
		}

		int ret;

		// output rate of 0 means SyncPWM
		switch (rate) {
		case SHOT_ONESHOT:
			ret = BankSetup_OneShot(&timer_banks[i], max_tim_clock);

			if (ret) {
				return ret;
			}

			TIM_TimeBaseStructure.TIM_Prescaler = timer_banks[i].prescaler;
			TIM_TimeBaseStructure.TIM_Period = timer_banks[i].period;

			// Configure this timer appropriately.
			TIM_TimeBaseInit(timer_banks[i].timer, &TIM_TimeBaseStructure);
			break;

		default:
			ret = BankSetup_PWM(&timer_banks[i], max_tim_clock, rate);

			if (ret) {
				return ret;
			}

			TIM_TimeBaseStructure.TIM_Prescaler = timer_banks[i].prescaler;
			TIM_TimeBaseStructure.TIM_Period = timer_banks[i].period;

			// Configure this timer appropriately.
			TIM_TimeBaseInit(timer_banks[i].timer, &TIM_TimeBaseStructure);
			break;

		case SHOT_DSHOT300:
		case SHOT_DSHOT600:
		case SHOT_DSHOT1200:
			ret = 1;
#if defined(PIOS_INCLUDE_DMASHOT)
			if (PIOS_DMAShot_IsConfigured()) {
				uint32_t freq;
				switch(rate) {
					default:
						// If for whatever reason new frequencies show up in the GPIO version,
						// we oughta know about it. So fail if that happens.
						PIOS_Assert(0);
					case SHOT_DSHOT300:
						freq = DMASHOT_300;
						break;
					case SHOT_DSHOT600:
						freq = DMASHOT_600;
						break;
					case SHOT_DSHOT1200:
						freq = DMASHOT_1200;
						break;
				}
				ret = PIOS_DMAShot_RegisterTimer(timer_banks[i].timer, max_tim_clock, freq) ? 0 : 1;
			}
#endif
			if (ret) {
				/* TODO: attempts to just fall back to
				 * bitbanging, which seems not ideal-- no
				 * benefit to doing DMA if we are bitbanging
				 * at the same time.
				 */
				ret = BankSetup_DShot(&timer_banks[i]);
			}

			if (ret) {
				return ret;
			}

			dshot_in_use = true;

			break;
		}


		/* Configure frequency scaler for all channels that use the same timer */
		for (uint8_t j = 0; j < servo_cfg->num_channels; j++) {
			if (channel_mask & (1 << j)) {
				continue;
			}

			const struct pios_tim_channel *chan = &servo_cfg->channels[j];
			if (timer_banks[i].timer == chan->timer) {
				/* save the frequency for these channels */
				switch (rate) {
				case SHOT_DSHOT300:
				case SHOT_DSHOT600:
				case SHOT_DSHOT1200:
#if defined(PIOS_INCLUDE_DMASHOT)
					if (PIOS_DMAShot_IsConfigured() && PIOS_DMAShot_RegisterServo(chan)) {
						output_channels[j].mode = SYNC_DSHOT_DMA;
					} else {
						// If RegisterServo fails, the relevant timer wasn't registered, or DMAShot
						// isn't configured, so we expect the bank to be set up already higher up in this
						// section.
						ChannelSetup_DShot(j, rate);
					}
#else
					ChannelSetup_DShot(j, rate);
#endif
					break;
				case SHOT_ONESHOT:
					output_channels[j].i.pwm_res = timer_banks[i].clk_rate;
					output_channels[j].mode = SYNC_PWM;
					break;
				default:
					output_channels[j].i.pwm_res = timer_banks[i].clk_rate;
					if (channel_max[j] >= channel_min[j]) {
						output_channels[j].mode = REGULAR_PWM;
					} else {
						output_channels[j].mode = REGULAR_INVERTED_PWM;
					}
					break;
				}
			}
		}
	}

#if defined (PIOS_INCLUDE_DMASHOT)
	if (PIOS_DMAShot_IsConfigured()) {
		PIOS_DMAShot_Validate();
		PIOS_DMAShot_InitializeTimers((TIM_OCInitTypeDef *)&servo_cfg->tim_oc_init);
		PIOS_DMAShot_InitializeGPIOs();
		PIOS_DMAShot_InitializeDMAs();
	}
#endif

	return 0;
}

static void PIOS_Servo_SetRaw(uint8_t servo, uint32_t raw_position) {
	const struct pios_tim_channel *chan = &servo_cfg->channels[servo];

	// in one-pulse mode, the pulse length is ARR - CCR
	if (output_channels[servo].mode == SYNC_PWM) {
		raw_position = chan->timer->ARR - raw_position;
	}

	/* Update the raw_position */
	switch(chan->timer_chan) {
		case TIM_Channel_1:
			TIM_SetCompare1(chan->timer, raw_position);
			break;
		case TIM_Channel_2:
			TIM_SetCompare2(chan->timer, raw_position);
			break;
		case TIM_Channel_3:
			TIM_SetCompare3(chan->timer, raw_position);
			break;
		case TIM_Channel_4:
			TIM_SetCompare4(chan->timer, raw_position);
			break;
	}
}

// Fraction: 0.16
// max_val, min_val: microseconds (seconds * 1000000)
void PIOS_Servo_SetFraction(uint8_t servo, uint16_t fraction,
		uint16_t max_val, uint16_t min_val)
{
	PIOS_Assert(max_val >= min_val);

	/* Make sure servo exists */
	if (!servo_cfg || servo >= servo_cfg->num_channels ||
			output_channels[servo].mode == UNCONFIGURED) {
		return;
	}

	if (channel_mask & (1 << servo)) {
		return;
	}

	switch (output_channels[servo].mode) {
		case SYNC_DSHOT_300:
		case SYNC_DSHOT_600:
		case SYNC_DSHOT_1200:
		case SYNC_DSHOT_DMA:
			/* Expect a 0 to 2047 range!
			 * Don't bother with min/max tomfoolery here.
			 */
			output_channels[servo].i.dshot.value = fraction >> 5;
			return;
		default:
			break;
	};

	// Seconds * 1000000 : 16.0
	uint16_t spread = max_val - min_val;

	uint64_t val = min_val;
	val = val << 16;
	// Seconds * 1000000 : 16.16

	val += spread * fraction;

	// Multiply by ticks/second to get: Ticks * 1000000: 36.16
	val *= output_channels[servo].i.pwm_res;

	// Ticks: 16.16
	val /= 1000000;

	// Round to nearest, 16.0
	val += 32767;
	val = val >> 16;

	if (resetting && (fraction < 0xff00)) {
		/* If we're resetting and the value isn't driven to the maximum
		 * value, drive low with no pulses.  If it's at the max value,
		 * it could be driving brushed motors and forced low is bad.
		 */
		val = 0;
	}

	PIOS_Servo_SetRaw(servo, val);
}

bool PIOS_Servo_IsDshot(uint8_t servo) {
	switch (output_channels[servo].mode) {
		case SYNC_DSHOT_300:
		case SYNC_DSHOT_600:
		case SYNC_DSHOT_1200:
		case SYNC_DSHOT_DMA:
			return true;
		default:
			break;
	};

	return false;
}

/**
* Set servo position
* \param[in] Servo Servo number (0->num_channels-1)
* \param[in] Position Servo position in microseconds
*/
void PIOS_Servo_Set(uint8_t servo, float position)
{
	/* Make sure servo exists */
	if (!servo_cfg || servo >= servo_cfg->num_channels ||
			output_channels[servo].mode == UNCONFIGURED) {
		return;
	}

	if (channel_mask & (1 << servo)) {
		return;
	}

	switch (output_channels[servo].mode) {
		case SYNC_DSHOT_300:
		case SYNC_DSHOT_600:
		case SYNC_DSHOT_1200:
		case SYNC_DSHOT_DMA:
			if (position > 2047)
				position = 2047;
			else if (position < 0)
				position = 0;

			output_channels[servo].i.dshot.value = position;
			return;
		default:
			break;
	};


	/* recalculate the position value based on timer clock rate */
	/* position is in us. */
	float us_to_count = output_channels[servo].i.pwm_res / 1000000.0f;
	position = position * us_to_count;

	if (resetting && (output_channels[servo].mode != REGULAR_INVERTED_PWM)) {
		/* Don't nail inverted channels low during reset, because they
		 * could be driving brushed motors.
		 */
		position = 0;
	}

	PIOS_Servo_SetRaw(servo, position);
}

#ifdef STM32F0XX	// No high-res delay on F0 yet
static int DSHOT_Update()
{
	return -1;
}
#else
static int DSHOT_Update()
{
	bool triggered = false;

	uint16_t dshot_bits[DS_GPIO_NUMVALS] = { 0 };
	uint16_t dshot_msg_zeroes[16][DS_GPIO_NUMVALS] = { { 0 } };

	enum channel_mode dshot_mode = UNCONFIGURED;

	for (int i = 0; i < servo_cfg->num_channels; i++) {
		struct output_channel *chan = &output_channels[i];
		struct dshot_info *info = &chan->i.dshot;

		if (channel_mask & (1 << i)) {
			continue;
		}

		switch (chan->mode) {
			case SYNC_DSHOT_300:
			case SYNC_DSHOT_600:
			case SYNC_DSHOT_1200:
				break;
			case SYNC_DSHOT_DMA:
#if defined(PIOS_INCLUDE_DMASHOT)
				PIOS_DMAShot_WriteValue(&servo_cfg->channels[i],
						info->value);
				continue;
#endif
			default:
				continue;	// Continue enclosing loop if not ds
		}

		if (dshot_mode == UNCONFIGURED) {
			dshot_mode = chan->mode;
		} else if (dshot_mode != chan->mode) {
			/* Multiple dshot rates not supported */
			return -1;
		}

		PIOS_Assert(info->gpio >= 0);
		PIOS_Assert(info->gpio < DS_GPIO_NUMVALS);

		dshot_bits[info->gpio] |= info->pin;

		PIOS_Assert(info->value < 2048);

		uint16_t message = info->value << 5;

		/* Set telem bit on commands */
		if ((info->value > 0) && (info->value < 48)) {
			message |= 16;
		}

		message |= 
			((message >> 4 ) & 0xf) ^
			((message >> 8 ) & 0xf) ^
			((message >> 12) & 0xf);

		for (int j = 0; j < 16; j++) {
			if (! (message & 0x8000)) {
				dshot_msg_zeroes[j][info->gpio] |= info->pin;
			}

			message <<= 1;
		}

		info->value = 0;	/* turn off motor if not updated */
	}

#if defined(PIOS_INCLUDE_DMASHOT)
	// Do it before bitbanging, so the updates line up +/-.
	// XXX: what's the use case for doing both at once?
	if (PIOS_DMAShot_IsReady()) {
		PIOS_DMAShot_TriggerUpdate();
		triggered = true;
	}
#endif
	uint16_t time_0, time_1, time_tot;

	/* As specified by original document:
	 * DShot 300 - 1250ns 0, 2500ns 1, 3333ns total
	 * DShot 600 -  625ns 0, 1250ns 1, 1667ns total
	 * DShot 1200-  312ns 0,  625ns 1,  833ns total
	 *
	 * Below timings are as used by Betaflight, which blheli_s seems to
	 * demand.  Meh.
	 * DShot 300 - 1000ns 0, 2166ns 1, 3333ns total
	 * DShot 600 -  500ns 0, 1083ns 1, 1667ns total
	 * DShot 1200-  250ns 0,  541ns 1,  833ns total
	 */

	switch (dshot_mode) {
		case UNCONFIGURED:
			return !triggered;
		case SYNC_DSHOT_300:
			time_0   = PIOS_INLINEDELAY_NsToCycles(1000);
			time_1   = PIOS_INLINEDELAY_NsToCycles(2166);
			time_tot = PIOS_INLINEDELAY_NsToCycles(3333);
			break;
		case SYNC_DSHOT_600:
			time_0   = PIOS_INLINEDELAY_NsToCycles(500);
			time_1   = PIOS_INLINEDELAY_NsToCycles(1083);
			time_tot = PIOS_INLINEDELAY_NsToCycles(1667);
			break;
		case SYNC_DSHOT_1200:
			time_0   = PIOS_INLINEDELAY_NsToCycles(250);
			time_1   = PIOS_INLINEDELAY_NsToCycles(541);
			time_tot = PIOS_INLINEDELAY_NsToCycles(833);
			break;
		default:
			PIOS_Assert(0);
			break;
	}

	if (dshot_mode == UNCONFIGURED) {
		return -1;
	}

	PIOS_IRQ_Disable();	// Necessary for critical timing below

	register uint16_t a = dshot_bits[DS_GPIOA];
	register uint16_t b = dshot_bits[DS_GPIOB];
	register uint16_t c = dshot_bits[DS_GPIOC];

	uint32_t start_cycle = PIOS_INLINEDELAY_GetCycleCnt() + 30;

	/* Time to get down to business of bitbanging this out. */
	for (int i = 0; i < 16; i++) {
		// Always delay at the beginning for consistent timing
		PIOS_INLINEDELAY_TillCycleCnt(start_cycle);

#if defined(STM32F40_41xxx) || defined(STM32F446xx)
#define CLEAR_BITS(gpio, bits) do { (gpio)->BSRRH = bits; } while (0)
#define SET_BITS(gpio, bits) do { (gpio)->BSRRL = bits; } while (0)
#else
		// Many STM32F* do not allow halfword writes to BSRR (or have
		// BSRRL/H definitions), so do it this way:

#define CLEAR_BITS(gpio, bits) do { (gpio)->BSRR = ((uint32_t) bits) << 16; } while (0)
#define SET_BITS(gpio, bits) do { (gpio)->BSRR = bits; } while (0)
#endif

		// Start pulse by writing to BSRRL's
		SET_BITS(GPIOA, a);
		SET_BITS(GPIOB, b);
		SET_BITS(GPIOC, c);

		// Calculate falling edge time for 0's
		uint32_t next_tick = start_cycle + time_0;

		// And preload next values before waiting
		a = dshot_msg_zeroes[i][DS_GPIOA];
		b = dshot_msg_zeroes[i][DS_GPIOB];
		c = dshot_msg_zeroes[i][DS_GPIOC];

		PIOS_INLINEDELAY_TillCycleCnt(next_tick);

		// Generate falling edges for zeroes (shorter pulses)
		CLEAR_BITS(GPIOA, a);
		CLEAR_BITS(GPIOB, b);
		CLEAR_BITS(GPIOC, c);

		// Calculate falling edge time for 1's
		next_tick = start_cycle + time_1;

		// Adjust next cycle time for when next bit should begin
		start_cycle += time_tot;

		// And next time, all bits will fall (no harm in doing twice for
		// the 0 bits)
		a = dshot_bits[DS_GPIOA];
		b = dshot_bits[DS_GPIOB];
		c = dshot_bits[DS_GPIOC];

		PIOS_INLINEDELAY_TillCycleCnt(next_tick);

		// Falling edges for all remaining bits.
		CLEAR_BITS(GPIOA, a);
		CLEAR_BITS(GPIOB, b);
		CLEAR_BITS(GPIOC, c);
	}

	PIOS_IRQ_Enable();

	return 0;
}
#endif /* !STM32F0xx */

/**
* Update the timer for HPWM/OneShot
*/
void PIOS_Servo_Update(void)
{
	if (!servo_cfg) {
		return;
	}

	// If some banks are oneshot and some are dshot (why would you do this?)
	// get the oneshots firing first.
	for (uint8_t i = 0; i < servo_cfg->num_channels; i++) {
		if (channel_mask & (1 << 1)) {
			continue;
		}

		if (output_channels[i].mode != SYNC_PWM) {
			continue;
		}

		const struct pios_tim_channel * chan = &servo_cfg->channels[i];

		/* Look for a disabled timer using synchronous output */
		if (!(chan->timer->CR1 & TIM_CR1_CEN)) {
			/* enable it again and reinitialize it */
			TIM_GenerateEvent(chan->timer, TIM_EventSource_Update);
			TIM_Cmd(chan->timer, ENABLE);
		}
	}

	if (dshot_in_use) {
		if (DSHOT_Update()) {
			dshot_in_use = false;
		}
	}
}

/**
 * @brief Determines the APB clock used by a given timer
 * @param[in] timer Pointer to the base register of the timer to check
 * @retval Clock frequency in Hz
 */

#if defined(STM32F0XX) /* F0 */
static uint32_t timer_apb_clock(TIM_TypeDef *timer)
{
	return PIOS_PERIPHERAL_APB1_CLOCK;
}

#elif defined(STM32F10X_MD) /* F1 */

static uint32_t timer_apb_clock(TIM_TypeDef *timer)
{
	if (timer == TIM1 || timer == TIM8)
		return PIOS_PERIPHERAL_APB2_CLOCK;
	else
		return PIOS_PERIPHERAL_APB1_CLOCK;
}

#elif defined(STM32F30X) /* F3 */

static uint32_t timer_apb_clock(TIM_TypeDef *timer)
{
	if (timer == TIM2 || timer == TIM3 || timer == TIM4)
		return PIOS_PERIPHERAL_APB1_CLOCK;
	else
		return PIOS_PERIPHERAL_APB2_CLOCK;
}

#elif defined(STM32F40_41xxx) || defined(STM32F446xx) /*  F4 */

static uint32_t timer_apb_clock(TIM_TypeDef *timer)
{
	if (timer == TIM1 || timer == TIM8 || timer == TIM9 || timer == TIM10 || timer == TIM11)
		return PIOS_PERIPHERAL_APB2_CLOCK;
	else
		return PIOS_PERIPHERAL_APB1_CLOCK;
}

#else
#error Unsupported microcontroller
#endif

/**
 * Prepare for IAP reset.  Stop PWM, so ESCs disarm.
 */
void PIOS_Servo_PrepareForReset() {
	resetting = true;
}

/**
 * @brief Determines the maximum clock rate for a given timer (i.e. no prescale)
 * @param[in] timer Pointer to the base register of the timer to check
 * @retval Clock frequency in Hz
 */
static uint32_t max_timer_clock(TIM_TypeDef *timer)
{
	if (timer == TIM6 || timer == TIM7) {
		// TIM6 and TIM7 cannot be used
		PIOS_Assert(0);
		return 0;
	}

	// "The timer clock frequencies are automatically fixed by hardware. There are two cases:
	//    1. if the APB prescaler is 1, the timer clock frequencies are set to the same frequency as
	//    that of the APB domain to which the timers are connected.
	//    2. otherwise, they are set to twice (*2) the frequency of the APB domain to which the
	//    timers are connected."
	uint32_t apb_clock = timer_apb_clock(timer);
	if (apb_clock == PIOS_SYSCLK)
		return apb_clock;
	else
		return apb_clock * 2;
}

/*
 * @}
 * @}
 */
