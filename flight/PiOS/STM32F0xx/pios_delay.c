/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DELAY Delay Functions
 * @brief PiOS Delay functionality
 * @{
 *
 * @file       pios_delay.c
 * @author     Michael Smith Copyright (C) 2012
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      Delay Functions 
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

/* Project Includes */
#include <pios.h>

/* This will result in a value of 32000 -- about 15 bits */
#define SYSTICK_HZ 1250
#define RTC_DIVIDER 2	/* Must be power of 2; this results in 625Hz "RTC" */

#define PIOS_RTC_MAX_CALLBACKS 3

struct rtc_callback_entry {
	void (*fn)(uintptr_t);
	uintptr_t data;
};

static struct rtc_callback_entry rtc_callback_list[PIOS_RTC_MAX_CALLBACKS];

static void PIOS_DELAY_Systick_Handler(void) __attribute__((isr));
static void PIOS_RTC_Tick();

const void *_systick_vector __attribute((section(".systick_vector"))) = PIOS_DELAY_Systick_Handler;

static volatile uint32_t systick_cnt;

static void PIOS_DELAY_Systick_Handler(void)
{
	systick_cnt++;

	if ((systick_cnt & (RTC_DIVIDER - 1)) == 0) {
		PIOS_RTC_Tick();
	}
}

/**
 * Initialises the Timer used by PIOS_DELAY functions.
 *
 * \return always zero (success)
 */
int32_t PIOS_DELAY_Init(void)
{
	/* Count up at HCLK frequency .. */
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	/* Request ~1ms tickiness -- .977ms actually */

	SysTick_Config(SYSCLK_FREQ / SYSTICK_HZ);

	return 0;
}

/**
 * Waits for a specific number of uS
 *
 * Example:<BR>
 * \code
 *   // Wait for 500 uS
 *   PIOS_DELAY_Wait_uS(500);
 * \endcode
 * \param[in] uS delay
 * \return < 0 on errors
 */
int32_t PIOS_DELAY_WaituS(uint32_t us)
{
	PIOS_Assert(us < 0x80000000);

	uint32_t initial = PIOS_DELAY_GetuS();
	uint32_t expires = initial + us;
	uint32_t offset = 0;

	if (initial > expires) {
		offset = 0x80000000;
		expires += offset;
	}

	while ((PIOS_DELAY_GetuS() + offset) < expires);

	/* No error */
	return 0;
}

/**
 * Waits for a specific number of mS
 *
 * Example:<BR>
 * \code
 *   // Wait for 500 mS
 *   PIOS_DELAY_Wait_mS(500);
 * \endcode
 * \param[in] mS delay (1..65535 milliseconds)
 * \return < 0 on errors
 */
int32_t PIOS_DELAY_WaitmS(uint32_t ms)
{
	// ~ 2147 seconds is a long time to busywait.
	PIOS_Assert(ms < (0x8000000 / 1000));

	return PIOS_DELAY_WaituS(ms * 1000);
}

/**
 * @brief Query the Delay timer for the current uS 
 * @return A microsecond value
 */
uint32_t PIOS_DELAY_GetuS()
{
	/* XXX fixup to be more precise */
	return systick_cnt * SYSTICK_HZ;
}

/**
 * @brief Calculate time in microseconds since a previous time
 * @param[in] t previous time
 * @return time in us since previous time t.
 */
uint32_t PIOS_DELAY_GetuSSince(uint32_t t)
{
	return PIOS_DELAY_GetuS() - t;
}

/**
 * @brief Get the raw delay timer, useful for timing
 * @return Unitless value (uint32 wrap around)
 */
uint32_t PIOS_DELAY_GetRaw()
{
	return PIOS_DELAY_GetuS();
}

/**
 * @brief Subtract raw time from now and convert to us 
 * @return A microsecond value
 */
uint32_t PIOS_DELAY_DiffuS(uint32_t raw)
{
	return PIOS_DELAY_GetuSSince(raw);
}

/**
 * @brief Subrtact two raw times and convert to us.
 * @return Interval between raw times in microseconds
 */
uint32_t PIOS_DELAY_DiffuS2(uint32_t raw, uint32_t later) {
	return later - raw;
}

float PIOS_RTC_Rate()
{
	return ((float) SYSTICK_HZ) / RTC_DIVIDER;
}

float PIOS_RTC_MsPerTick()
{
	return 1000 / ((float) SYSTICK_HZ) * RTC_DIVIDER;
}

bool PIOS_RTC_RegisterTickCallback(void (*fn)(uintptr_t id), uintptr_t data)
{
	for (int i = 0; i < PIOS_RTC_MAX_CALLBACKS; i++) {
		if (!rtc_callback_list[i].fn) {
			struct rtc_callback_entry *cb;

			cb = &rtc_callback_list[i];

			cb->fn = fn;
			cb->data = data;

			return true;
		}
	}

	return false;
}

static void PIOS_RTC_Tick()
{
	for (int i = 0; i < PIOS_RTC_MAX_CALLBACKS; i++) {
		if (rtc_callback_list[i].fn) {
			rtc_callback_list[i].fn(rtc_callback_list[i].data);
		}
	}
}

/**
  * @}
  * @}
  */
