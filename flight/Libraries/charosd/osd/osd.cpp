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
#include "osd.h"
#include "../settings.h"
#include "../telemetry/telemetry.h"
#include "../config.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include "../eeprom.h"
#include "../lib/max7456/max7456.h"

namespace osd
{

namespace settings
{

#define EEPROM_ADDR_SCREENS            _eeprom_byte (OSD_EEPROM_OFFSET)
#define EEPROM_ADDR_SWITCH_MODE        _eeprom_byte (OSD_EEPROM_OFFSET + 1)
#define EEPROM_ADDR_SWITCH_CHANNEL     _eeprom_byte (OSD_EEPROM_OFFSET + 2)
#define EEPROM_ADDR_SELECTOR_MIN       _eeprom_word (OSD_EEPROM_OFFSET + 3)
#define EEPROM_ADDR_SELECTOR_MAX       _eeprom_word (OSD_EEPROM_OFFSET + 5)
#define EEPROM_ADDR_TOGGLE_TRESHOLD    _eeprom_word (OSD_EEPROM_OFFSET + 7)

const char __opt_screens [] PROGMEM = "SCREENS";
const char __opt_switch  [] PROGMEM = "SWM";
const char __opt_swch    [] PROGMEM = "SWCH";
const char __opt_selmin  [] PROGMEM = "SELMIN";
const char __opt_selmax  [] PROGMEM = "SELMAX";
const char __opt_tgtresh [] PROGMEM = "TGTRESH";

const ::settings::option_t __settings [] PROGMEM = {
	declare_uint8_option  (__opt_screens, EEPROM_ADDR_SCREENS),
	declare_uint8_option  (__opt_switch,  EEPROM_ADDR_SWITCH_MODE),
	declare_uint8_option  (__opt_swch,    EEPROM_ADDR_SWITCH_CHANNEL),
	declare_uint16_option (__opt_selmin,  EEPROM_ADDR_SELECTOR_MIN),
	declare_uint16_option (__opt_selmax,  EEPROM_ADDR_SELECTOR_MAX),
	declare_uint16_option (__opt_tgtresh, EEPROM_ADDR_TOGGLE_TRESHOLD),
};

void init ()
{
	::settings::append_section (__settings, sizeof (__settings) / sizeof (::settings::option_t));
}

void reset ()
{
	eeprom_update_byte (EEPROM_ADDR_SCREENS, OSD_DEFAULT_SCREENS);
	eeprom_update_byte (EEPROM_ADDR_SWITCH_MODE, OSD_DEFAULT_SWITCH_MODE);
	eeprom_update_byte (EEPROM_ADDR_SWITCH_CHANNEL, OSD_DEFAULT_SWITCH_CHANNEL);
	eeprom_update_word (EEPROM_ADDR_SELECTOR_MIN, OSD_DEFAULT_SELECTOR_MIN);
	eeprom_update_word (EEPROM_ADDR_SELECTOR_MAX, OSD_DEFAULT_SELECTOR_MAX);
	eeprom_update_word (EEPROM_ADDR_TOGGLE_TRESHOLD, OSD_DEFAULT_TOGGLE_TRESHOLD);
	screen::settings::reset ();
}

}  // namespace settings

///////////////////////////////////////////////////////////////////////////////

#ifndef OSD_EEPROM_SWITCH_RAW_CHANNEL_DEFAULT
	#define OSD_EEPROM_SWITCH_RAW_CHANNEL_DEFAULT 6
#endif

uint8_t switch_mode;
uint8_t switch_channel;
uint16_t selector_min, selector_max, sector_size;
uint16_t toggle_threshod;
uint8_t cur_screen;
uint8_t screens;
uint16_t old_value = 0;
//bool visible;

#if (OSD_DEFAULT_SCREENS <= 0) || (OSD_DEFAULT_SCREENS > OSD_MAX_SCREENS)
	#error OSD_DEFAULT_SCREENS must be between 0 and OSD_MAX_SCREENS
#endif


bool check_input ()
{
	if (screens < 2 || !telemetry::input::connected || !telemetry::input::rssi) return false;

	uint8_t old_screen = cur_screen;

	uint16_t value = telemetry::input::channels [switch_channel];

	if (switch_mode == OSD_SWITCH_MODE_SELECTOR)
	{
		if (value < selector_min) value = selector_min;
		else if (value > selector_max) value = selector_max;

		cur_screen = value - selector_min / sector_size;

		if (cur_screen >= screens) cur_screen = screens - 1;
	}
	else
	{
		if (value >= toggle_threshod && old_value < toggle_threshod)
		{
			cur_screen ++;
			if (cur_screen >= screens) cur_screen = 0;
		}

		old_value = value;
	}

	return cur_screen != old_screen;
}

uint8_t screens_enabled ()
{
	uint8_t res = eeprom_read_byte (EEPROM_ADDR_SCREENS);
	if (!res) res = 1;
	return res >= OSD_MAX_SCREENS ? OSD_MAX_SCREENS : res;
}

volatile bool started = false;
volatile bool mutex = false;

ISR (INT0_vect, ISR_NOBLOCK)
{
	if (!started || !screen::updated || mutex) return;
	mutex = true;

	screen::draw ();

	mutex = false;
}

void main ()
{
	// TODO: hide and show
	// visible = true;

	screen::load (0);
	screen::update ();

	// VSYNC external interrupt
	EICRA = _BV (ISC01);	// VSYNC falling edge
	EIMSK = _BV (INT0);		// enable
	started = true;

	// just in case
	wdt_enable (WDTO_250MS);

	while (true)
	{
		wdt_reset ();

		bool updated = telemetry::update ();
		if (screens > 1 && check_input ())
		{
			screen::load (cur_screen);
			updated = true;
		}

		//if (updated && visible)
		if (updated)
		{
			while (mutex)
				;
			mutex = true;
			screen::update ();
			mutex = false;
		}
	}
}

void init ()
{
	switch_mode = eeprom_read_byte (EEPROM_ADDR_SWITCH_MODE);
	switch_channel = eeprom_read_byte (EEPROM_ADDR_SWITCH_CHANNEL);
	selector_min = eeprom_read_word (EEPROM_ADDR_SELECTOR_MIN);
	selector_max = eeprom_read_word (EEPROM_ADDR_SELECTOR_MAX);
	selector_max = eeprom_read_word (EEPROM_ADDR_SELECTOR_MAX);
	toggle_threshod = eeprom_read_word (EEPROM_ADDR_TOGGLE_TRESHOLD);
	screens = screens_enabled ();
	sector_size = (selector_max - selector_min) / screens;
}

}  // namespace osd
