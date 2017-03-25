/**
 ******************************************************************************
 *
 * @file       pios_delay.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Thorsten Klose (tk@midibox.org) (tk@midibox.org)
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      Delay Functions 
 *                 - Provides a micro-second granular delay using a TIM 
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   PIOS_DELAY Delay Functions
 * @{
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
#include "time.h"

#include <time.h>

/**
 * This is the value used as a base.  Strictly not required, as times
 * can be expected to wrap... But it makes sense to get sane numbers at
 * first that will agree with the PIOS_Thread ones etc.
 */
static uint32_t base_time;

#ifdef DRONIN_GETTIME
int clock_gettime(clockid_t clk_id, struct timespec *t)
{
	(void) clk_id;

	uint64_t tm = mach_absolute_time();
	static int numer = 0, denom = 0;

	if (!numer) {
		mach_timebase_info_data_t tb;

		kern_return_t ret = mach_timebase_info(&tb);

		if (ret != KERN_SUCCESS) abort();

		numer = tb.numer;
		denom = tb.denom;
	}

	tm *= numer;
	tm /= denom;

	t->tv_nsec = tm % 1000000000;
	t->tv_sec  = tm / 1000000000;

	return 0;
}

#endif /* CLOCK_MONOTONIC */
static uint32_t get_monotonic_us_time(void) {
	clockid_t id = CLOCK_MONOTONIC;

#ifdef CLOCK_BOOTTIME
	id = CLOCK_BOOTTIME;
#endif /* CLOCK_BOOTTIME; assumes it's a define */

	struct timespec tp;

	if (clock_gettime(id, &tp)) {
		perror("clock_gettime");
		abort();
	}

	uint32_t val = tp.tv_sec * 1000000 + tp.tv_nsec / 1000;

	return val;
}

/**
* Initialises the Timer used by PIOS_DELAY functions<BR>
* This is called from pios.c as part of the main() function
* at system start up.
* \return < 0 if initialisation failed
*/
int32_t PIOS_DELAY_Init(void)
{
	base_time = get_monotonic_us_time();

	/* No error */
	return 0;
}

/**
* Waits for a specific number of uS<BR>
* Example:<BR>
* \code
*   // Wait for 500 uS
*   PIOS_DELAY_Wait_uS(500);
* \endcode
* \param[in] uS delay (1..65535 microseconds)
* \return < 0 on errors
*/
int32_t PIOS_DELAY_WaituS(uint32_t uS)
{
	struct timespec wait,rest;
	wait.tv_sec=0;
	wait.tv_nsec=1000*uS;
	while (nanosleep(&wait,&rest)) {
		wait=rest;
	}

	/* No error */
	return 0;
}


/**
* Waits for a specific number of mS<BR>
* Example:<BR>
* \code
*   // Wait for 500 mS
*   PIOS_DELAY_Wait_mS(500);
* \endcode
* \param[in] mS delay (1..65535 milliseconds)
* \return < 0 on errors
*/
int32_t PIOS_DELAY_WaitmS(uint32_t mS)
{
	struct timespec wait,rest;
	wait.tv_sec=mS/1000;
	wait.tv_nsec=(mS%1000)*1000000;
	while (nanosleep(&wait, &rest)) {
		wait=rest;
	}

	/* No error */
	return 0;
}

uint32_t PIOS_DELAY_GetRaw()
{
	uint32_t raw_us = get_monotonic_us_time() - base_time;
	return raw_us;
}

uint32_t PIOS_DELAY_DiffuS(uint32_t ref)
{
	return PIOS_DELAY_DiffuS2(ref, PIOS_DELAY_GetRaw());
}

uint32_t PIOS_DELAY_DiffuS2(uint32_t raw, uint32_t later) {
	uint32_t diff = later - raw;
	return diff;
}

