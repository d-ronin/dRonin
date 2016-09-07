/* Imported from MultiOSD (https://github.com/UncleRus/MultiOSD/)
 * Altered for use on STM32 Flight controllers by dRonin
 * Copyright (C) dRonin 2016
 */


/*
 * This file is part of MultiOSD <https://github.com/UncleRus/MultiOSD>
 *
 * MultiOSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef OSD_PANEL_H_
#define OSD_PANEL_H_

#include <stdint.h>
#include <stdbool.h>

#include <openpilot.h>
#include <pios_max7456.h>
#include "charosd.h"

typedef void (*update_t) (charosd_state_t state, uint8_t x, uint8_t y);

typedef struct 
{
	const char *name;
	update_t update;
} panel_t;

// panels collection
extern const panel_t panels [];
extern const uint8_t panels_count;

const char *panel_name (uint8_t panel)
{
	return panels [panel].name;
}

void panel_draw (charosd_state_t state, uint8_t panel, uint8_t x, uint8_t y)
{
	panels [panel].update(state, x, y);
}

#endif /* OSD_PANEL_H_ */

