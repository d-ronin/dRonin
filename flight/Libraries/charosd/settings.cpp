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
#include "settings.h"
#include <string.h>
#include "config.h"

#include "lib/max7456/max7456.h"
#include "telemetry/telemetry.h"
#include "osd/osd.h"

#include "lib/uart/uart.h"

namespace settings
{

uint8_t sections_count = 0;
section_t sections [SETTINGS_MAX_SECTIONS];

void append_section (const option_t *options, uint8_t size)
{
	if (sections_count >= SETTINGS_MAX_SECTIONS) return;

	sections [sections_count].options = options;
	sections [sections_count].size = size;
	sections_count ++;
}

const option_t *get_option (const char *name)
{
	for (uint8_t s = 0; s < sections_count; s ++)
	{
		const option_t *opts = sections [s].options;
		for (uint8_t i  = 0; i < sections [s].size; i ++)
			if (!strncasecmp_P (name, (const char *) pgm_read_ptr (&opts [i].name_p), SETTINGS_MAX_NAME_LEN))
				return &opts [i];
	}
	return NULL;
}

void *get_addr (const char *name)
{
	const option_t *opt = get_option (name);
	return opt ? pgm_read_ptr (&opt->addr) : NULL;
}

uint8_t read_uint8_option (const option_t *option)
{
	return eeprom_read_byte ((const uint8_t *) pgm_read_ptr (&option->addr));
}

uint16_t read_uint16_option (const option_t *option)
{
	return eeprom_read_word ((const uint16_t *) pgm_read_ptr (&option->addr));
}

uint32_t read_uint32_option (const option_t *option)
{
	return eeprom_read_dword ((const uint32_t *) pgm_read_ptr (&option->addr));
}

float read_float_option (const option_t *option)
{
	return eeprom_read_float ((const float *) pgm_read_ptr (&option->addr));
}

void read_str_option (const option_t *option, char *dest)
{
	uint8_t size = pgm_read_byte (&option->size);
	eeprom_read_block (dest, pgm_read_ptr (&option->addr), size);
	dest [size] = 0;
}

void write_uint8_option (const char *name, uint8_t value)
{
	eeprom_update_byte ((uint8_t *) get_addr (name), value);
}

void write_uint16_option (const char *name, uint16_t value)
{
	eeprom_update_word ((uint16_t *) get_addr (name), value);
}

void write_uint32_option (const char *name, uint32_t value)
{
	eeprom_update_dword ((uint32_t *) get_addr (name), value);
}

void write_float_option (const char *name, float value)
{
	eeprom_update_float ((float *) get_addr (name), value);
}

void write_str_option (const char *name, const char *value)
{
	const option_t *opt = get_option (name);
	if (!opt) return;
	uint8_t max_size = pgm_read_byte (&opt->size) - 1;
	uint8_t size = strlen (value);
	if (size > max_size) size = max_size;
	uint8_t *addr = (uint8_t *) pgm_read_ptr (&opt->addr);
	eeprom_update_block (value, addr, size + 1);
	eeprom_update_byte (addr + size, 0);
}

void init ()
{
	// let's init modules settings
	max7456::settings::init ();
	telemetry::settings::init ();
	osd::settings::init ();

	if (eeprom_read_word (NULL) != EEPROM_HEADER || eeprom_read_word ((uint16_t *) 2) != VERSION)
		reset ();
}

void reset ()
{
	max7456::settings::reset ();
	telemetry::settings::reset ();
	osd::settings::reset ();

	eeprom_write_word (NULL, EEPROM_HEADER);
	eeprom_write_word ((uint16_t *) 2, VERSION);
}

}
