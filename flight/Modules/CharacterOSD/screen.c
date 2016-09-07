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
#include "screen.h"
#include "pios_max7456.h"
#include "panel.h"

typedef struct
{
	uint8_t panel;
	uint8_t x;
	uint8_t y;
} panel_pos_t;

panel_pos_t records [];
uint8_t count = 0;
volatile bool updated = false;

void screen_load (uint8_t num)
{
	const panel_pos_t _default_screen_0 [] = { SCR_DEFAULT_0 };

	if (num >= OSD_MAX_SCREENS) num = 0;

	const panel_pos_t *screen = _default_screen_0;

	uint8_t i = 0;
	for (; i < OSD_SCREEN_PANELS; i ++)
	{
		records [i].panel = screen[i].panel;
		if (records [i].panel >= osd::panel::count)
			break;
		records [i].x = screen[i].x;
		records [i].y = screen[i].y;
	}
	count = i;

	PIOS_MAX7456_clear (dev);
}

void screen_update ()
{
	for (uint8_t i = 0; i < count; i ++)
		panel_update(records [i].panel);
	updated = true;
}

void screen_draw ()
{
	if (!updated) return;
	PIOS_MAX7456_clear (dev);

	for (uint8_t i = 0; i < count; i ++)
		panel_draw(records[i].panel, records[i].x, records[i].y);

	updated = false;
}
