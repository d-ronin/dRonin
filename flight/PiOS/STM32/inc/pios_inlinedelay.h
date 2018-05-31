/**
 ******************************************************************************
 * @file       pios_inlinedelay.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_INLINEDELAY Inline delay subsystem
 * @{
 * @brief Inline delay routines for fine-resolution spinloops.
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

#ifndef _PIOS_INLINEDELAY_H
#define _PIOS_INLINEDELAY_H

#include <pios.h>

/** Confirms that actual clock speed equals expected clockspeed */
static inline void PIOS_INLINEDELAY_AssertClockSpeed()
{
	RCC_ClocksTypeDef clocks;

	RCC_GetClocksFreq(&clocks);
	uint32_t us_ticks = clocks.SYSCLK_Frequency / 1000000;
	PIOS_DEBUG_Assert(us_ticks > 1);

	uint32_t other_us_ticks = PIOS_SYSCLK / 1000000;

	PIOS_Assert(us_ticks = other_us_ticks);
}

static inline uint32_t PIOS_INLINEDELAY_NsToCycles(uint32_t ns)
{
	/* assumptions: integral number of MHz. */

	/* compute the number of system clocks per microsecond */

	uint32_t ticks = PIOS_SYSCLK / 1000;

	ticks <<= 10;		/* 22.10 KHz */

	ticks += 500000;

	ticks /= 1000000;	/* 22.10 GHz */

	ticks *= ns;		/* 22.10 GHz * ns = 22.10 cycles */

	return (ticks + 512) >> 10;
}

static inline uint32_t PIOS_INLINEDELAY_GetCycleCnt()
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	return DWT->CYCCNT;
}

static inline void PIOS_INLINEDELAY_TillCycleCnt(uint32_t whence)
{
	uint32_t value;

	/* DWT access should have been enabled to get value already */

	do {
		value = whence - DWT->CYCCNT;
	} while (value < 0xe0000000);		// Windowed against overflow.
}

#endif /* _PIOS_INLINEDELAY_H */


/**
 * @}
 */
