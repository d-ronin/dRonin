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

struct lpfilter_state {

	struct lpfilter_first_order *first_order;
	struct lpfilter_biquad *biquad[4];
	uint8_t order;
	uint8_t width;

};

void lpfilter_create(struct lpfilter_state *filter, float cutoff, float dT, uint8_t order, uint8_t width);
float lpfilter_run_single(struct lpfilter_state *filter, uint8_t axis, float sample);
void lpfilter_run(struct lpfilter_state *filter, float *sample);

#endif // FILTER_H
