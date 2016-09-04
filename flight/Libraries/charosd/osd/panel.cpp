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

#include "panel.h"
#include "../lib/max7456/max7456.h"
#include "../telemetry/telemetry.h"
#include <math.h>
#include <string.h>

namespace osd
{

namespace draw
{

	/*
	 * 012
	 * 3 7
	 * 456
	 */
	const uint8_t _rect_thin [] PROGMEM = {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7};
	const uint8_t _rect_fill [] PROGMEM = {0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf};

	void rect (uint8_t l, uint8_t t, uint8_t w, uint8_t h, bool filled, uint8_t attr)
	{
		if (w < 2 || h < 2) return;

		uint8_t r = w - 1;
		uint8_t b = h - 1;

		char _rect [8];
		memcpy_P (_rect, filled ? _rect_fill : _rect_thin, 8);
		char buffer [w + 1];

		for (uint8_t i = 0; i < h; i ++)
		{
			char spacer;
			if (i == 0)
			{
				buffer [0] = _rect [0];
				buffer [r] = _rect [2];
				spacer = _rect [1];
			}
			else if (i == b)
			{
				buffer [0] = _rect [4];
				buffer [r] = _rect [6];
				spacer = _rect [5];
			}
			else
			{
				buffer [0] = _rect [3];
				buffer [r] = _rect [7];
				spacer = ' ';
			}
			if (w > 2)
				memset (buffer + 1, spacer, w - 2);
			buffer [w] = 0;
			max7456::puts (l, t + i, buffer, attr);
		}
	}

#define _ARROWS 0x90

	void arrow (uint8_t x, uint8_t y, uint16_t direction, uint8_t attr)
	{
		uint8_t chr = _ARROWS + ((uint8_t) (direction / 360.0 * 16)) * 2;
		max7456::put (x, y, chr, attr);
		max7456::put (x + 1, y, chr + 1, attr);
	}

	const char err_str [] PROGMEM = "---";

	void battery_voltage (uint8_t x, uint8_t y, uint8_t symbol_offset, telemetry::battery::battery_t *bat)
	{
		max7456::put (x, y, symbol_offset + (uint8_t) round (bat->level / 20.0), bat->low ? MAX7456_ATTR_BLINK : 0);
		max7456::open (x + 1, y);
		fprintf_P (&max7456::stream, PSTR ("%.2f\x8e"), bat->voltage);
	}

}  // namespace draw


namespace panels
{

#define terminate_buffer() do { buffer [sizeof (buffer) - 1] = 0; } while (0)

#define DECLARE_BUF(n) char buffer [n];

#define PANEL_NAME(__name) const char name [] PROGMEM = __name;

#define STD_DRAW void draw (uint8_t x, uint8_t y) \
{ \
	max7456::puts (x, y, buffer); \
}

#define STD_UPDATE(fmt, ...) void update () \
{ \
	snprintf_P (buffer, sizeof (buffer), PSTR (fmt), __VA_ARGS__); \
	terminate_buffer (); \
}

#define STD_PANEL(__name, bs, fmt, ...) \
	PANEL_NAME (__name); \
	DECLARE_BUF (bs); \
	STD_UPDATE (fmt, __VA_ARGS__); \
	STD_DRAW;


namespace alt
{

	STD_PANEL ("StableAlt", 8, "\x85%d\x8d", (int16_t) round (telemetry::stable::altitude));
	//STD_PANEL ("StableAlt", 8, "\x85%.1f\x8d", telemetry::stable::altitude);

}  // namespace alt

namespace climb
{

#define _PAN_CLIMB_SYMB 0x03

	PANEL_NAME ("Climb");

	DECLARE_BUF (8);

	void update ()
	{
		int8_t c = round (telemetry::stable::climb * 10);
		uint8_t s;
		if (c >= 20) s = _PAN_CLIMB_SYMB + 5;
		else if (c >= 10) s = _PAN_CLIMB_SYMB + 4;
		else if (c >= 0) s = _PAN_CLIMB_SYMB + 3;
		else if (c <= -20) s = _PAN_CLIMB_SYMB + 2;
		else if (c <= -10) s = _PAN_CLIMB_SYMB + 1;
		else s = _PAN_CLIMB_SYMB;
		snprintf_P (buffer, sizeof (buffer), PSTR ("%c%.1f\x8c"), s, telemetry::stable::climb);
		terminate_buffer ();
	}

	STD_DRAW;

}  // namespace climb_rate

namespace flight_mode
{

	PANEL_NAME ("FlightMode");

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		osd::draw::rect (x, y, 6, 3);
		if (telemetry::status::flight_mode_name)
			max7456::puts (x + 1, y + 1, telemetry::status::flight_mode_name); // use RAM name
		else
			max7456::puts_p (x + 1, y + 1, telemetry::status::flight_mode_name_p ? telemetry::status::flight_mode_name_p : PSTR ("\x09\x09\x09\x09"));
//		max7456::open (x + 1, y + 1);
//		fprintf_P (&max7456::stream, PSTR ("%u"), telemetry::status::flight_mode);
	}

}  // namespace flight_mode

namespace armed_flag
{

	PANEL_NAME ("ArmedFlag");

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		uint8_t attr = telemetry::status::armed ? 0 : MAX7456_ATTR_INVERT;
		osd::draw::rect (x, y, 3, 3, true, attr);
		max7456::put (x + 1, y + 1, 0xe0, attr);
	}

}  // namespace name

namespace connection_state
{

	PANEL_NAME ("ConState");

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		// TODO blink MAX7456_ATTR_INVERT -> MAX7456_ATTR_NONE when CONNECTION_STATE_ESTABLISHING
		uint8_t attr = telemetry::status::connection != telemetry::status::CONNECTED
			? MAX7456_ATTR_INVERT
			: 0;

		osd::draw::rect (x, y, 3, 3, true, attr);
		max7456::put (x + 1, y + 1, 0xe1, attr);
	}

}  // namespace connection_state

namespace flight_time
{
	STD_PANEL ("FlightTime", 8, "\xb3%02u:%02u", telemetry::status::flight_time / 60, telemetry::status::flight_time % 60);

}  // namespace flight_time

namespace roll
{
	STD_PANEL ("Roll", 7, "\xb2%d\xb0", (int16_t) telemetry::attitude::roll);

}  // namespace roll

namespace pitch
{

	STD_PANEL ("Pitch", 7, "\xb1%d\xb0", (int16_t) telemetry::attitude::pitch);

}  // namespace pitch

namespace gps_state
{

	PANEL_NAME ("GPS");

#define _PAN_GPS_2D 0x01
#define _PAN_GPS_3D 0x02

	DECLARE_BUF (4);

	STD_UPDATE ("%d", telemetry::gps::satellites);

	void draw (uint8_t x, uint8_t y)
	{
		bool err = telemetry::gps::state == telemetry::GPS_STATE_NO_FIX;
		max7456::puts_p (x, y, PSTR ("\x10\x11"), err ? MAX7456_ATTR_INVERT : 0);
		max7456::put (x + 2, y, telemetry::gps::state < telemetry::GPS_STATE_3D ? _PAN_GPS_2D : _PAN_GPS_3D,
			err ? MAX7456_ATTR_INVERT : (telemetry::gps::state < telemetry::GPS_STATE_2D ? MAX7456_ATTR_BLINK : 0));
		if (err) max7456::puts_p (x + 3, y, draw::err_str, MAX7456_ATTR_INVERT);
		else max7456::puts (x + 3, y, buffer);
	}

}  // namespace gps_state

namespace gps_lat
{

	STD_PANEL ("Lat", 11, "\x83%02.6f", telemetry::gps::latitude);

}  // namespace gps_lat

namespace gps_lon
{

	STD_PANEL ("Lon", 11, "\x84%02.6f", telemetry::gps::longitude);

}  // namespace gps_lon

namespace horizon
{

#define PANEL_HORIZON_WIDTH 14
#define PANEL_HORIZON_HEIGHT 5

#define _PAN_HORZ_LINE 0x16
#define _PAN_HORZ_TOP 0x0e

#define _PAN_HORZ_CHAR_LINES 18
#define _PAN_HORZ_VRES 9
#define _PAN_HORZ_INT_WIDTH (PANEL_HORIZON_WIDTH - 2)
#define _PAN_HORZ_LINES (PANEL_HORIZON_HEIGHT * _PAN_HORZ_VRES)
#define _PAN_HORZ_TOTAL_LINES (PANEL_HORIZON_HEIGHT * _PAN_HORZ_CHAR_LINES)

	PANEL_NAME ("Horizon");

	const char _line [PANEL_HORIZON_WIDTH + 1] PROGMEM   = "\xb8            \xb9";
	const char _center [PANEL_HORIZON_WIDTH + 1] PROGMEM = "\xc8            \xc9";
	char buffer [PANEL_HORIZON_HEIGHT][PANEL_HORIZON_WIDTH + 1];

	float __attribute__ ((noinline)) deg2rad (float deg)
	{
		return deg * 0.017453293;
	}

	void update ()
	{
		for (uint8_t i = 0; i < PANEL_HORIZON_HEIGHT; i ++)
		{
			memcpy_P (buffer [i], i == PANEL_HORIZON_HEIGHT / 2 ? _center : _line, PANEL_HORIZON_WIDTH);
			buffer [i][PANEL_HORIZON_WIDTH] = 0;
		}

		// code below from minoposd
		int16_t pitch_line = tan (deg2rad (telemetry::attitude::pitch)) * _PAN_HORZ_LINES;
		float roll = tan (deg2rad (telemetry::attitude::roll));
		for (uint8_t col = 1; col <= _PAN_HORZ_INT_WIDTH; col ++)
		{
			// center X point at middle of each column
			float middle = col * _PAN_HORZ_INT_WIDTH - (_PAN_HORZ_INT_WIDTH * _PAN_HORZ_INT_WIDTH / 2) - _PAN_HORZ_INT_WIDTH / 2;
			// calculating hit point on Y plus offset to eliminate negative values
			int16_t hit = roll * middle + pitch_line + _PAN_HORZ_LINES;
			if (hit > 0 && hit < _PAN_HORZ_TOTAL_LINES)
			{
				int8_t row = PANEL_HORIZON_HEIGHT - ((hit - 1) / _PAN_HORZ_CHAR_LINES);
				int8_t subval = (hit - (_PAN_HORZ_TOTAL_LINES - row * _PAN_HORZ_CHAR_LINES + 1)) * _PAN_HORZ_VRES / _PAN_HORZ_CHAR_LINES;
				if (subval == _PAN_HORZ_VRES - 1)
					buffer [row - 2][col] = _PAN_HORZ_TOP;
				buffer [row - 1][col] = _PAN_HORZ_LINE + subval;
			}
		}
	}

	void draw (uint8_t x, uint8_t y)
	{
		for (uint8_t i = 0; i < PANEL_HORIZON_HEIGHT; i ++)
			max7456::puts (x, y + i, buffer [i]);
	}

}  // namespace horizon

namespace throttle
{

	STD_PANEL ("Throttle", 7, "\x87%d%%", telemetry::input::throttle);

}  // namespace throttle

namespace groundspeed
{

	STD_PANEL ("Groundspeed", 7, "\x80%d\x81", (int16_t) (telemetry::stable::groundspeed * 3.6));

}  // namespace ground_speed

namespace battery1_voltage
{

	PANEL_NAME ("Bat1Voltage");

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		osd::draw::battery_voltage (x, y, 0xee, &telemetry::battery::battery1);
	}

}  // namespace battery_voltage

namespace battery2_voltage
{

	PANEL_NAME ("Bat2Voltage");

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		osd::draw::battery_voltage (x, y, 0xf4, &telemetry::battery::battery2);
	}

}  // namespace battery_voltage

namespace battery1_current
{

	STD_PANEL ("Bat1Current", 8, "\xfa%.2f\x8f", telemetry::battery::battery1.amperage);

}  // namespace battery_current

namespace battery2_current
{

	STD_PANEL ("Bat2Current", 8, "\xec%.2f\x8f", telemetry::battery::battery2.amperage);

}  // namespace battery_current

namespace battery1_consumed
{

	STD_PANEL ("Bat1Consumed", 8, "\xfb%u\x82", (uint16_t) telemetry::battery::battery1.consumed);

}  // namespace battery_consumed

namespace battery2_consumed
{

	STD_PANEL ("Bat2Consumed", 8, "\xed%u\x82", (uint16_t) telemetry::battery::battery2.consumed);

}  // namespace battery_consumed

namespace rssi_flag
{

	PANEL_NAME ("RSSIFlag");

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		if (telemetry::input::rssi_low) max7456::put (x, y, 0xb4, MAX7456_ATTR_BLINK);
	}

}  // namespace rssi_flag

namespace home_distance
{

	PANEL_NAME ("HomeDistance");

	DECLARE_BUF (8);
	uint8_t attr, i_attr;

	void update ()
	{
		attr = telemetry::home::state == telemetry::home::NO_FIX ? MAX7456_ATTR_INVERT : 0;
		i_attr = telemetry::home::state != telemetry::home::FIXED ? MAX7456_ATTR_INVERT : 0;
		if (i_attr)
		{
			snprintf_P (buffer, sizeof (buffer), PSTR ("%S"),
				telemetry::home::state == telemetry::home::NO_FIX ? draw::err_str : PSTR ("\x09\x09\x09\x8d"));
			return;
		}
		if (telemetry::home::distance >= 10000)
			 snprintf_P (buffer, sizeof (buffer), PSTR ("%.1f\x8b"), telemetry::home::distance / 1000);
		else snprintf_P (buffer, sizeof (buffer), PSTR ("%u\x8d"), (uint16_t) telemetry::home::distance);
	}

	void draw (uint8_t x, uint8_t y)
	{
		max7456::put (x, y, 0x12, i_attr);
		max7456::puts (x + 1, y, buffer, attr);
	}

}  // namespace home_distance

namespace home_direction
{

#define _PAN_HD_ARROWS 0x90

	PANEL_NAME ("HomeDirection");

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		if (telemetry::home::state == telemetry::home::FIXED)
			osd::draw::arrow (x, y, telemetry::home::direction);
	}

}  // namespace home_direction

namespace callsign
{

	PANEL_NAME ("Callsign");

	void update () {}

	void draw (uint8_t x, uint8_t y)
	{
		max7456::puts (x, y, telemetry::status::callsign);
	}

}  // namespace callsign

namespace temperature
{

	STD_PANEL ("Temperature", 9, "\xfd%.1f\x8a", telemetry::environment::temperature);

}  // namespace temperature


namespace rssi
{

	PANEL_NAME ("RSSI");

	const char _l0 [] PROGMEM = "\xe5\xe8\xe8";
	const char _l1 [] PROGMEM = "\xe2\xe8\xe8";
	const char _l2 [] PROGMEM = "\xe2\xe6\xe8";
	const char _l3 [] PROGMEM = "\xe2\xe3\xe8";
	const char _l4 [] PROGMEM = "\xe2\xe3\xe7";
	const char _l5 [] PROGMEM = "\xe2\xe3\xe4";

	const char * const levels [] PROGMEM = { _l0, _l1, _l2, _l3, _l4, _l5 };

	const char *scale = NULL;

	void update ()
	{
		uint8_t level = round (telemetry::input::rssi / 20.0);
		if (level == 0 && telemetry::input::rssi > 0) level = 1;
		if (level > 5) level = 5;
		scale = (const char *) pgm_read_ptr (&levels [level]);
	}

	void draw (uint8_t x, uint8_t y)
	{
		max7456::puts_p (x, y, scale);
	}

}  // namespace rssi

namespace compass
{

	PANEL_NAME ("Compass");

	// Code from MinOpOSD
	const uint8_t ruler [] PROGMEM = {
		0xc2, 0xc0, 0xc1, 0xc0,
		0xc4, 0xc0, 0xc1, 0xc0,
		0xc3, 0xc0, 0xc1, 0xc0,
		0xc5, 0xc0, 0xc1, 0xc0,
	};

	const int8_t ruler_size = sizeof (ruler);

	DECLARE_BUF (12);

	uint8_t attr, arrow;

	void update ()
	{
		int16_t offset = round (telemetry::stable::heading * ruler_size / 360.0) - (sizeof (buffer) - 1) / 2;
		if (offset < 0) offset += ruler_size;
		for (uint8_t i = 0; i < sizeof (buffer) - 1; i ++)
		{
			buffer [i] = pgm_read_byte (&ruler [offset]);
			if (++ offset >= ruler_size) offset = 0;
		}
		terminate_buffer ();
		switch (telemetry::stable::heading_source)
		{
			case telemetry::stable::DISABLED:
				attr = MAX7456_ATTR_INVERT;
				arrow = 0xc6;
				break;
			case telemetry::stable::GPS:
				attr = 0;
				arrow = 0xb6;
				break;
			case telemetry::stable::INTERNAL_MAG:
				attr = 0;
				arrow = 0xc7;
				break;
			default:
				attr = 0;
				arrow = 0xc6;
		}
	}

	void draw (uint8_t x, uint8_t y)
	{
		max7456::put (x + (sizeof (buffer) - 1) / 2, y, arrow, attr);
		max7456::puts (x, y + 1, buffer, attr);
	}

}  // namespace compass

namespace airspeed
{

	STD_PANEL ("Airspeed", 7, "\x88%d\x81", (int16_t) (telemetry::stable::airspeed * 3.6));

}  // namespace airspeed

}  // namespace panels

namespace panel
{

#define declare_panel(NS) { osd::panels:: NS ::name, osd::panels:: NS ::update, osd::panels:: NS ::draw }

const panel_t panels [] PROGMEM = {
	declare_panel (alt),
	declare_panel (climb),
	declare_panel (flight_mode),
	declare_panel (armed_flag),
	declare_panel (connection_state),
	declare_panel (flight_time),
	declare_panel (roll),
	declare_panel (pitch),
	declare_panel (gps_state),
	declare_panel (gps_lat),
	declare_panel (gps_lon),
	declare_panel (horizon),
	declare_panel (throttle),
	declare_panel (groundspeed),
	declare_panel (battery1_voltage),
	declare_panel (battery2_voltage),
	declare_panel (battery1_current),
	declare_panel (battery2_current),
	declare_panel (battery1_consumed),
	declare_panel (battery2_consumed),
	declare_panel (rssi_flag),
	declare_panel (home_distance),
	declare_panel (home_direction),
	declare_panel (callsign),
	declare_panel (temperature),
	declare_panel (rssi),
	declare_panel (compass),
	declare_panel (airspeed),
};

const uint8_t count = sizeof (panels) / sizeof (panel_t);

}  // namespace panels

}  // namespace osd
