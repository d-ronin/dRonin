/**
 ******************************************************************************
 * @addtogroup TauLabsLibraries Tau Labs Libraries
 * @{
 * @addtogroup TauLabsMath Filtering support libraries
 * @{
 *
 * @file       filter.h
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

#ifndef FILTER_H
#define FILTER_H

enum {ROLL,PITCH,YAW,MAX_AXES};

struct filter_biquad_state {
	float x1, x2, y1, y2;
};

struct filter_biquad {
	float b0, a1, a2;
	struct filter_biquad_state s[MAX_AXES];
};

struct filter_first_order {
	float alpha;
	float prev[MAX_AXES];
};

struct filter_compound_state {

	struct filter_first_order *first_order;
	struct filter_biquad *biquad[4];
	uint8_t order;

};

void filter_create_lowpass(struct filter_compound_state *filter, float cutoff, float dT, uint8_t order);
float filter_execute(struct filter_compound_state *filter, int axis, float sample);

#endif // FILTER_H
