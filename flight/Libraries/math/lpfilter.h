/**
 ******************************************************************************
 * @addtogroup Libraries Libraries
 * @{
 * @addtogroup FlightMath Filtering support libraries
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

typedef struct lpfilter_state* lpfilter_state_t;

void lpfilter_create(lpfilter_state_t *filter_ptr, float cutoff, float dT, uint8_t order, uint8_t width);
float lpfilter_run_single(lpfilter_state_t filter, uint8_t axis, float sample);
void lpfilter_run(lpfilter_state_t filter, float *sample);

#endif // FILTER_H
