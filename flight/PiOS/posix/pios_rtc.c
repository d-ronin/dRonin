/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_RTC Realtime clock (periodic calls) functions
 * @{
 *
 * @file       pios_rtc.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Calls callbacks at ~500Hz.
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
#include "pios.h"

#if defined(PIOS_INCLUDE_RTC)

#include <pios_thread.h>

struct rtc_callback_entry {
	void (*fn)(uintptr_t);
	uintptr_t data;
};

#define PIOS_RTC_MAX_CALLBACKS 10
struct rtc_callback_entry rtc_callback_list[PIOS_RTC_MAX_CALLBACKS];
static volatile uint32_t rtc_counter;
static int rtc_callback_next;

uint32_t PIOS_RTC_Counter()
{
	return rtc_counter;
}

float PIOS_RTC_Rate()
{
	return 500;
}

float PIOS_RTC_MsPerTick() 
{
	return 1000.0f / PIOS_RTC_Rate();
}

bool PIOS_RTC_RegisterTickCallback(void (*fn)(uintptr_t id), uintptr_t data)
{
	struct rtc_callback_entry * cb;
	if (rtc_callback_next >= PIOS_RTC_MAX_CALLBACKS) {
		return false;
	}

	cb = &rtc_callback_list[rtc_callback_next++];

	cb->fn   = fn;
	cb->data = data;

	return true;
}

static void rtc_do(void)
{
	rtc_counter++;

	/* Call all registered callbacks */
	for (uint8_t i = 0; i < rtc_callback_next; i++) {
		struct rtc_callback_entry * cb = &rtc_callback_list[i];
		if (cb->fn) {
			(cb->fn)(cb->data);
		}
	}
}

static void PIOS_RTC_Task(void *unused)
{
	(void) unused;

	uint32_t now = PIOS_Thread_Systime();

	while (true) {
		PIOS_Thread_Sleep_Until(&now, 1000 / PIOS_RTC_Rate());

		rtc_do();
	}
}

void PIOS_RTC_Init()
{
	struct pios_thread *rtc_task = PIOS_Thread_Create(PIOS_RTC_Task,
			"pios_rtc_task",
			PIOS_THREAD_STACK_SIZE_MIN,
			NULL, PIOS_THREAD_PRIO_HIGHEST);

	PIOS_Assert(rtc_task);	
}

#endif /* PIOS_INCLUDE_RTC */


/** 
 * @}
 * @}
 */
