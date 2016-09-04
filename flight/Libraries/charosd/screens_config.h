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
#ifndef SCREENS_CONFIG_H_
#define SCREENS_CONFIG_H_

#include "config.h"

// default screens layout
// { PANEL, x, y }

#if !defined (TELEMETRY_MODULES_UBX)
#	define SCR_DEFAULT_0 \
		{OSD_PANEL_GPS_STATE, 1, 1}, \
		{OSD_PANEL_GPS_LAT, 8, 1}, \
		{OSD_PANEL_GPS_LON, 19, 1}, \
		{OSD_PANEL_CONNECTION_STATE, 0, 2}, \
		{OSD_PANEL_ARMING_STATE, 3, 2}, \
		{OSD_PANEL_RSSI, 21, 3}, \
		{OSD_PANEL_FLIGHT_MODE, 24, 2}, \
		{OSD_PANEL_PITCH, 1, 6}, \
		{OSD_PANEL_HORIZON, 8, 6}, \
		{OSD_PANEL_THROTTLE, 23, 6}, \
		{OSD_PANEL_GROUND_SPEED, 1, 8}, \
		{OSD_PANEL_ALT, 23, 8}, \
		{OSD_PANEL_ROLL, 1, 10}, \
		{OSD_PANEL_VARIO, 23, 10}, \
		{OSD_PANEL_FLIGHT_TIME, 1, 13}, \
		{OSD_PANEL_COMPASS, 9, 12}, \
		{OSD_PANEL_HOME_DISTANCE, 1, 14}, \
		{OSD_PANEL_HOME_DIRECTION, 8, 14}, \
		{OSD_PANEL_BATTERY2_VOLTAGE, 1, 12}, \
		{OSD_PANEL_BATTERY1_VOLTAGE, 22, 12}, \
		{OSD_PANEL_BATTERY1_CURRENT, 22, 13}, \
		{OSD_PANEL_BATTERY1_CONSUMED, 22, 14}
#	define SCR_DEFAULT_1 \
		{OSD_PANEL_CONNECTION_STATE, 0, 0}, \
		{OSD_PANEL_ARMING_STATE, 3, 0}, \
		{OSD_PANEL_CALLSIGN, 7, 1}, \
		{OSD_PANEL_RSSI, 21, 1}, \
		{OSD_PANEL_FLIGHT_MODE, 24, 0}, \
		{OSD_PANEL_ALT, 1, 8}, \
		{OSD_PANEL_HORIZON, 8, 6}, \
		{OSD_PANEL_FLIGHT_TIME, 1, 13}, \
		{OSD_PANEL_HOME_DISTANCE, 1, 14}, \
		{OSD_PANEL_BATTERY1_VOLTAGE, 22, 14}
#	define SCR_DEFAULT_2 \
		{OSD_PANEL_CONNECTION_STATE, 0, 0}, \
		{OSD_PANEL_ARMING_STATE, 3, 0}, \
		{OSD_PANEL_RSSI, 20, 1}, \
		{OSD_PANEL_FLIGHT_MODE, 24, 0}, \
		{OSD_PANEL_PITCH, 1, 6}, \
		{OSD_PANEL_ALT, 1, 8}, \
		{OSD_PANEL_ROLL, 1, 10}, \
		{OSD_PANEL_FLIGHT_TIME, 1, 13}, \
		{OSD_PANEL_BATTERY1_VOLTAGE, 22, 14}
#else
#	define SCR_DEFAULT_0 \
		{OSD_PANEL_GPS_STATE, 1, 1}, \
		{OSD_PANEL_GPS_LAT, 8, 1}, \
		{OSD_PANEL_GPS_LON, 19, 1}, \
		{OSD_PANEL_CONNECTION_STATE, 1, 2}, \
		{OSD_PANEL_RSSI, 25, 3}, \
		{OSD_PANEL_GROUND_SPEED, 1, 6}, \
		{OSD_PANEL_ALT, 1, 8}, \
		{OSD_PANEL_VARIO, 1, 10}, \
		{OSD_PANEL_FLIGHT_TIME, 1, 13}, \
		{OSD_PANEL_COMPASS, 9, 12}, \
		{OSD_PANEL_HOME_DISTANCE, 1, 14}, \
		{OSD_PANEL_HOME_DIRECTION, 8, 14}, \
		{OSD_PANEL_BATTERY1_VOLTAGE, 22, 12}, \
		{OSD_PANEL_BATTERY1_CURRENT, 22, 13}, \
		{OSD_PANEL_BATTERY1_CONSUMED, 22, 14}
#	define SCR_DEFAULT_1 \
		{OSD_PANEL_CONNECTION_STATE, 1, 0}, \
		{OSD_PANEL_CALLSIGN, 7, 1}, \
		{OSD_PANEL_RSSI, 25, 1}, \
		{OSD_PANEL_ALT, 1, 8}, \
		{OSD_PANEL_FLIGHT_TIME, 1, 13}, \
		{OSD_PANEL_HOME_DISTANCE, 1, 14}, \
		{OSD_PANEL_BATTERY1_VOLTAGE, 22, 14}
#	define SCR_DEFAULT_2 \
		{OSD_PANEL_CONNECTION_STATE, 1, 0}, \
		{OSD_PANEL_RSSI, 25, 1}, \
		{OSD_PANEL_ALT, 1, 8}, \
		{OSD_PANEL_FLIGHT_TIME, 1, 13}, \
		{OSD_PANEL_BATTERY1_VOLTAGE, 22, 14}
#endif


#endif /* SCREENS_CONFIG_H_ */
