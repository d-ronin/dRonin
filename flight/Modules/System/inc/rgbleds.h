/**
 ******************************************************************************
 * @file       rgbleds.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup Modules Modules
 * @{
 * @addtogroup SystemModule System
 * @{
 * @brief Implementation of annunciation and simple color blending for RGB LEDs.
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

#ifndef _RGBLEDS_H
#define _RGBLEDS_H

#include <stdint.h>

void systemmod_process_rgb_leds(bool led_override, bool led_override_active,
		uint8_t blink_prio, bool is_armed, bool force_dim);

#endif

/**
 * @}
 * @}
 */
