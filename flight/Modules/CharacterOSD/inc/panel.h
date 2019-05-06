/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{
 * @addtogroup CharOSD Character OSD
 * @{
 *
 * @brief Process OSD information
 * @file       panel.h
 * @author     dRonin, http://dronin.org Copyright (C) 2016
 * @author     @UncleRus and MultiOSD
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

/* Imported from MultiOSD (https://github.com/UncleRus/MultiOSD/)
 * Altered for use on STM32 Flight controllers by dRonin
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

#ifndef _PANEL_H
#define _PANEL_H

#include <openpilot.h>
#include <math.h>
#include <string.h>
#include <misc_math.h>

#include "charosd.h"

#include "physical_constants.h"

#include "airspeedactual.h"
#include "flightstatus.h"
#include "systemalarms.h"
#include "modulesettings.h"

#define ERR_STR "---"

#define terminate_buffer() do { buffer [sizeof(buffer) - 1] = 0; } while (0)

#define STD_UPDATE(__name, n, fmt, ...) static void __name ## _update(charosd_state_t state, uint8_t x, uint8_t y) \
{ \
	char buffer[n]; \
	snprintf(buffer, sizeof (buffer), fmt, __VA_ARGS__); \
	terminate_buffer(); \
	PIOS_MAX7456_puts(state->dev, x, y, buffer, 0); \
}

#define STD_PANEL(__name, bs, fmt, ...) \
	STD_UPDATE(__name, bs, fmt, __VA_ARGS__);

#endif // _PANEL_H

/**
  * @}
  * @}
  */
