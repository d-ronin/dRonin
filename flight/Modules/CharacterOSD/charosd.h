/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{
 * @addtogroup CharOSD Character OSD
 * @{
 *
 * @brief Process OSD information
 * @file       charosd.h
 * @author     dRonin, http://dronin.org Copyright (C) 2016
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

#ifndef _CHAROSD_H
#define _CHAROSD_H

#include "pios_max7456.h"
#include "charonscreendisplaysettings.h"

#include "attitudeactual.h"
#include "flightbatterystate.h"
#include "gpsposition.h"
#include "positionactual.h"
#include "velocityactual.h"

typedef struct {
	FlightBatteryStateData battery;
	AttitudeActualData attitude_actual;
	PositionActualData position_actual;
	VelocityActualData velocity_actual;
	GPSPositionData gps_position;
	struct {
		int16_t rssi;
		float thrust;
	} manual;
	struct {
		uint8_t arm_status;
		uint8_t mode;
	} flight_status;
	struct {
		uint32_t flight_time;
	} system;
} telemetry_t;

typedef struct {
	max7456_dev_t dev;

	telemetry_t telemetry;
	char *custom_text;
	uint8_t prev_font;
	uint8_t video_standard;

	int available;
} * charosd_state_t;

typedef void (*update_t) (charosd_state_t state, uint8_t x, uint8_t y);

typedef struct
{
	update_t update;
	int requirements;
} panel_t;

#define HAS_GPS		(1 << 0)
#define HAS_BATT	(1 << 1)
#define HAS_PITOT	(1 << 2)
#define HAS_ALT		(1 << 3)
#define HAS_TEMP	(1 << 4)
#define HAS_RSSI	(1 << 5)
#define HAS_NAV		(1 << 6)
#define HAS_COMPASS	(1 << 7)

#define HAS_SENSOR(available, required) ((available & required) == required)

// panels collection
extern const panel_t panels [];
extern const uint8_t panels_count;

#define CHAROSD_CHAR_GPS_2D		0x01
#define CHAROSD_CHAR_GPS_3D		0x02
#define CHAROSD_CHAR_VARIO_DOWN1	0x03
#define CHAROSD_CHAR_VARIO_DOWN2	0x04
#define CHAROSD_CHAR_VARIO_DOWN3	0x05
#define CHAROSD_CHAR_VARIO_UP1		0x06
#define CHAROSD_CHAR_VARIO_UP2		0x07
#define CHAROSD_CHAR_VARIO_UP3		0x08
#define CHAROSD_CHAR_BULLET		0x09
#define CHAROSD_CHAR_CROSSHAIR		0x0a
#define CHAROSD_CHAR_GPS_SATL		0x10
#define CHAROSD_CHAR_GPS_SATR		0x11
#define CHAROSD_CHAR_HOME		0x12
#define CHAROSD_CHAR_BAR0		0x16
#define CHAROSD_CHAR_BAR1		0x17
#define CHAROSD_CHAR_BAR2		0x18
#define CHAROSD_CHAR_BAR3		0x19
#define CHAROSD_CHAR_BAR4		0x1a
#define CHAROSD_CHAR_BAR5		0x1b
#define CHAROSD_CHAR_BAR6		0x1c
#define CHAROSD_CHAR_BAR7		0x1d
#define CHAROSD_CHAR_BAR8		0x1e
#define CHAROSD_CHAR_FLAG		0x7f
#define CHAROSD_CHAR_GS			0x80
#define CHAROSD_CHAR_KMH		0x81
#define CHAROSD_CHAR_MAH		0x82
#define CHAROSD_CHAR_LAT		0x83
#define CHAROSD_CHAR_LON		0x84
#define CHAROSD_CHAR_ALT		0x85
#define CHAROSD_CHAR_VEL		0x86
#define CHAROSD_CHAR_THR		0x87
#define CHAROSD_CHAR_AIR		0x88
#define CHAROSD_CHAR_DEGC		0x8a
#define CHAROSD_CHAR_KM			0x8b
#define CHAROSD_CHAR_MS			0x8c
#define CHAROSD_CHAR_M			0x8d
#define CHAROSD_CHAR_V			0x8e
#define CHAROSD_CHAR_A			0x8f
#define CHAROSD_CHAR_ARROW_NL		0x90
#define CHAROSD_CHAR_ARROW_NR		0x91
#define CHAROSD_CHAR_ARROW_NNEL		0x92
#define CHAROSD_CHAR_ARROW_NNER		0x93
#define CHAROSD_CHAR_ARROW_NEL		0x94
#define CHAROSD_CHAR_ARROW_NER		0x95
#define CHAROSD_CHAR_ARROW_ENEL		0x96
#define CHAROSD_CHAR_ARROW_ENER		0x97
#define CHAROSD_CHAR_ARROW_EL		0x98
#define CHAROSD_CHAR_ARROW_ER		0x99
#define CHAROSD_CHAR_ARROW_ESEL		0x9a
#define CHAROSD_CHAR_ARROW_ESER		0x9b
#define CHAROSD_CHAR_ARROW_SEL		0x9c
#define CHAROSD_CHAR_ARROW_SER		0x9d
#define CHAROSD_CHAR_ARROW_SSEL		0x9e
#define CHAROSD_CHAR_ARROW_SSER		0x9f
#define CHAROSD_CHAR_ARROW_SL		0xa0
#define CHAROSD_CHAR_ARROW_SR		0xa1
#define CHAROSD_CHAR_ARROW_SSWL		0xa2
#define CHAROSD_CHAR_ARROW_SSWR		0xa3
#define CHAROSD_CHAR_ARROW_SWL		0xa4
#define CHAROSD_CHAR_ARROW_SWR		0xa5
#define CHAROSD_CHAR_ARROW_WSWL		0xa6
#define CHAROSD_CHAR_ARROW_WSWR		0xa7
#define CHAROSD_CHAR_ARROW_WL		0xa8
#define CHAROSD_CHAR_ARROW_WR		0xa9
#define CHAROSD_CHAR_ARROW_WNWL		0xaa
#define CHAROSD_CHAR_ARROW_WNWR		0xab
#define CHAROSD_CHAR_ARROW_NWL		0xac
#define CHAROSD_CHAR_ARROW_NWR		0xad
#define CHAROSD_CHAR_ARROW_NNWL		0xae
#define CHAROSD_CHAR_ARROW_NNWR		0xaf
#define CHAROSD_CHAR_DEG		0xb0
#define CHAROSD_CHAR_PITCH		0xb1
#define CHAROSD_CHAR_ROLL		0xb2
#define CHAROSD_CHAR_TIME		0xb3
#define CHAROSD_CHAR_RSSI		0xb4
#define CHAROSD_CHAR_YAW		0xb5
#define CHAROSD_CHAR_AHI_HUDL		0xb8
#define CHAROSD_CHAR_AHI_HUDR		0xb9
#define CHAROSD_CHAR_AHI_CENL		0xc8
#define CHAROSD_CHAR_AHI_CENR		0xc9
#define CHAROSD_CHAR_WINGL		0xbd
#define CHAROSD_CHAR_WINGM		0xbe
#define CHAROSD_CHAR_WINGR		0xbf
#define CHAROSD_CHAR_COMPASS_RULER_LOW	0xc0
#define CHAROSD_CHAR_COMPASS_RULER_HIGH 0xc1
#define CHAROSD_CHAR_COMPASS_NORTH	0xc2
#define CHAROSD_CHAR_COMPASS_SOUTH	0xc3
#define CHAROSD_CHAR_COMPASS_EAST	0xc4
#define CHAROSD_CHAR_COMPASS_WEST	0xc5
#define CHAROSD_CHAR_COMPASS_POINT	0xc6
#define CHAROSD_CHAR_COMPASS_INTERNAL	0xc7
#define CHAROSD_CHAR_COMPASS_GPS	0xb6
#define CHAROSD_CHAR_BOX_TL		0xd0
#define CHAROSD_CHAR_BOX_T		0xd1
#define CHAROSD_CHAR_BOX_TR		0xd2
#define CHAROSD_CHAR_BOX_R		0xd3
#define CHAROSD_CHAR_BOX_BL		0xd4
#define CHAROSD_CHAR_BOX_B		0xd5
#define CHAROSD_CHAR_BOX_BR		0xd6
#define CHAROSD_CHAR_BOX_L		0xd7
#define CHAROSD_CHAR_FILL_TL		0xd8
#define CHAROSD_CHAR_FILL_T		0xd9
#define CHAROSD_CHAR_FILL_TR		0xda
#define CHAROSD_CHAR_FILL_R		0xdb
#define CHAROSD_CHAR_FILL_BL		0xdc
#define CHAROSD_CHAR_FILL_B		0xdd
#define CHAROSD_CHAR_FILL_BR		0xde
#define CHAROSD_CHAR_FILL_L		0xdf
#define CHAROSD_CHAR_ARMED		0xe0
#define CHAROSD_CHAR_BIGC		0xe1
#define CHAROSD_CHAR_SIG1_H		0xe2
#define CHAROSD_CHAR_SIG2_HH		0xe3
#define CHAROSD_CHAR_SIG3_HH		0xe4
#define CHAROSD_CHAR_SIG1_L		0xe5
#define CHAROSD_CHAR_SIG2_HL		0xe6
#define CHAROSD_CHAR_SIG3_HL		0xe7
#define CHAROSD_CHAR_SIG23_LL		0xe8
#define CHAROSD_CHAR_POWER2		0xec
#define CHAROSD_CHAR_ENERGY2		0xed
#define CHAROSD_CHAR_BAT_CRIT		0xee
#define CHAROSD_CHAR_BAT_EMPTY		0xef
#define CHAROSD_CHAR_BAT_LOW		0xf0
#define CHAROSD_CHAR_BAT_MID		0xf1
#define CHAROSD_CHAR_BAT_HIGH		0xf2
#define CHAROSD_CHAR_BAT_FULL		0xf3
#define CHAROSD_CHAR_BAT2_CRIT		0xf4
#define CHAROSD_CHAR_BAT2_EMPTY		0xf5
#define CHAROSD_CHAR_BAT2_LOW		0xf6
#define CHAROSD_CHAR_BAT2_MID		0xf7
#define CHAROSD_CHAR_BAT2_HIGH		0xf8
#define CHAROSD_CHAR_BAT2_FULL		0xf9
#define CHAROSD_CHAR_POWER		0xfa
#define CHAROSD_CHAR_ENERGY		0xfb
#define CHAROSD_CHAR_LEFTRIGHT		0xfc
#define CHAROSD_CHAR_TEMP		0xfd
#define CHAROSD_CHAR_ROUTE		0xfe

#endif // _CHAROSD_H

/**
  * @}
  * @}
  */
