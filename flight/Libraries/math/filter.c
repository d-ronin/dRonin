/**
 ******************************************************************************
 * @addtogroup TauLabsLibraries Tau Labs Libraries
 * @{
 * @addtogroup TauLabsMath Filtering support libraries
 * @{
 *
 * @file       filter.c
 * @author     dRonin, http://dronin.org, Copyright (C) 2017
 * @brief      Signal filtering algorithms
 *
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "pios_heap.h"
#include "filter.h"

const float filter_butterworth_factors[16] = {
	// 2nd order
	1.4142f,

	// 3rd order
	1.0f,

	// 4th order
	0.7654f, 1.8478f,

	// 5th order
	0.6180f, 1.6180f,

	// 6th order
	0.5176f, 1.4142f, 1.9319f,

	// 7th order
	0.4450f, 1.2470f, 1.8019f,

	// 8th order
	0.3902f, 1.1111f, 1.6629f, 1.9616f
};

void filter_construct_single_biquad(struct filter_biquad *b, float cutoff, float dT, float q)
{
	float f = 1.0f / tanf((float)M_PI*cutoff*dT);

	// Skipping calculation of b1 and b2, since this only going to do Butterworth.
	// These terms are optimized away in the actual filtering calculation.
	b->b0 = 1.0f / (1.0f + q*f + f*f);
	b->a1 = 2.0f * (f*f - 1.0f) * b->b0;
	b->a2 = -(1.0f - q*f + f*f) * b->b0;

	memset((void*)b->s, 0, sizeof(struct filter_biquad_state)*MAX_AXES);
}

void filter_construct_biquads(struct filter_compound_state *filt, float cutoff, float dT, int o)
{
	// Amount of biquad filters needed.
	int len = o >> 1;

	// Calculate the address of coefficients in the look-up table.
	// There's probably a proper mathematical way for this. Let's just count
	// everything.
	int addr = 0;
	for(int i = 2; i < o; i++)
	{
		addr += i >> 1;
	}

	// Create all necessary biquads and allocate, too, if not yet done so.
	for(int i = 0; i < len; i++)
	{
		if(filt->biquad[i] == NULL)	{
			filt->biquad[i] = PIOS_malloc_no_dma(sizeof(struct filter_biquad));
		}
		filter_construct_single_biquad(filt->biquad[i], cutoff, dT, filter_butterworth_factors[addr+i]);
	}
}

void filter_create_lowpass(struct filter_compound_state *filter, float cutoff, float dT, uint8_t order)
{
	// Don't check whether filter is NULL or not. We're passing a static
	// struct in the code. If this ends up NULL for a reason, something
	// else is completely broken.

	// Clamp order count. If zero, this bypasses the filter.
	if(order == 0) {
		filter->order = 0;
		return;
	} else if(order > 8) order = 8;

	if(order & 0x1)	{
		// Filter is odd, allocate the first order filter.
		if(filter->first_order == NULL)	{
			filter->first_order = PIOS_malloc_no_dma(sizeof(struct filter_first_order));
		}

		filter->first_order->alpha = expf(-2.0f * (float)(M_PI) * cutoff * dT);
		memset((void*)filter->first_order->prev, 0, sizeof(float)*MAX_AXES);
	}

	filter->order = order;
	filter_construct_biquads(filter, cutoff, dT, order);
}

float filter_execute(struct filter_compound_state *filter, int axis, float sample)
{
	int order = filter->order;

	// Order at zero means bypass.
	if(order == 0) return sample;

	if(order & 0x1)	{
		// Odd order filter
		filter->first_order->prev[axis] *= filter->first_order->alpha;
		filter->first_order->prev[axis] += (1 - filter->first_order->alpha) * sample;
		sample = filter->first_order->prev[axis];
	}

	// Run all generated biquads.
	order >>= 1;
	for(int i = 0; i < order; i++)
	{
		struct filter_biquad *b = filter->biquad[i];
		struct filter_biquad_state *s = &b->s[axis];

		float y = b->b0 * (sample + 2.0f * s->x1 + s->x2) + b->a1 * s->y1 + b->a2 * s->y2;

		s->y2 = s->y1;
		s->y1 = y;

		s->x2 = s->x1;
		s->x1 = sample;

		sample = y;
	}

	return sample;
}
