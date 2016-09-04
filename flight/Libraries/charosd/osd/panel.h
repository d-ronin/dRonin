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
#include "../lib/pgmspace.h"

#define OSD_PANEL_ALT                 0
#define OSD_PANEL_VARIO               1
#define OSD_PANEL_FLIGHT_MODE         2
#define OSD_PANEL_ARMING_STATE        3
#define OSD_PANEL_CONNECTION_STATE    4
#define OSD_PANEL_FLIGHT_TIME         5
#define OSD_PANEL_ROLL                6
#define OSD_PANEL_PITCH               7
#define OSD_PANEL_GPS_STATE           8
#define OSD_PANEL_GPS_LAT             9
#define OSD_PANEL_GPS_LON             10
#define OSD_PANEL_HORIZON             11
#define OSD_PANEL_THROTTLE            12
#define OSD_PANEL_GROUND_SPEED        13
#define OSD_PANEL_BATTERY1_VOLTAGE    14
#define OSD_PANEL_BATTERY2_VOLTAGE    15
#define OSD_PANEL_BATTERY1_CURRENT    16
#define OSD_PANEL_BATTERY2_CURRENT    17
#define OSD_PANEL_BATTERY1_CONSUMED   18
#define OSD_PANEL_BATTERY2_CONSUMED   19
#define OSD_PANEL_RSSI_FLAG           20
#define OSD_PANEL_HOME_DISTANCE       21
#define OSD_PANEL_HOME_DIRECTION      22
#define OSD_PANEL_CALLSIGN            23
#define OSD_PANEL_TEMPERATURE         24
#define OSD_PANEL_RSSI                25
#define OSD_PANEL_COMPASS             26
#define OSD_PANEL_AIRSPEED            27

namespace osd
{

namespace panel
{

	struct panel_t
	{
		typedef void (*update_t) ();
		typedef void (*draw_t) (uint8_t x, uint8_t y);

		const char *name_p; // progmem!
		update_t update;
		draw_t draw;
	};

	// panels collection
	extern const panel_t panels [] PROGMEM;
	extern const uint8_t count;

	inline const char *name_p (uint8_t panel)
	{
		return (const char *) pgm_read_ptr (&panels [panel].name_p);
	}

	inline void update (uint8_t panel)
	{
		((panel_t::update_t) pgm_read_ptr (&panels [panel].update)) ();
	}

	inline void draw (uint8_t panel, uint8_t x, uint8_t y)
	{
		((panel_t::draw_t) pgm_read_ptr (&panels [panel].draw)) (x, y);
	}

}  // namespace panel

namespace draw
{

	void rect (uint8_t l, uint8_t t, uint8_t w, uint8_t h, bool filled = false, uint8_t attr = 0);
	void arrow (uint8_t x, uint8_t y, uint16_t direction, uint8_t attr = 0);

}  // namespace draw

}  // namespace osd


#endif /* OSD_PANEL_H_ */

