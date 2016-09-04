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
#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <avr/eeprom.h>
#include "lib/pgmspace.h"

#define EEPROM_SIZE 0x400
#define EEPROM_HEADER 0x5552
#define EEPROM_START 4

#define SETTINGS_MAX_NAME_LEN 16
#define SETTINGS_MAX_SECTIONS 16

#define OSD_SCREENS_EEPROM_OFFSET (OSD_EEPROM_OFFSET + 0x10)

namespace settings
{

enum option_type_t { ot_bool = 0, ot_uint8, ot_uint16, ot_uint32, ot_float, ot_str };

struct option_t
{
	const char *name_p;
	option_type_t type;
	uint8_t size;
	void *addr;
};

#define declare_bool_option(NAME, ADDR) { NAME, ::settings::ot_bool, 1, ADDR }
#define declare_uint8_option(NAME, ADDR) { NAME, ::settings::ot_uint8, 1, ADDR }
#define declare_uint16_option(NAME, ADDR) { NAME, ::settings::ot_uint16, 2, ADDR }
#define declare_uint32_option(NAME, ADDR) { NAME, ::settings::ot_uint32, 4, ADDR }
#define declare_float_option(NAME, ADDR) { NAME, ::settings::ot_float, 4, ADDR }
#define declare_str_option(NAME, ADDR, SIZE) { NAME, ::settings::ot_str, (SIZE) + 1, ADDR }

struct section_t
{
	const option_t *options;
	uint8_t size;
};

extern section_t sections [SETTINGS_MAX_SECTIONS];
extern uint8_t sections_count;

// Define options
void append_section (const option_t *options, uint8_t size);

// Get option by name
const option_t *get_option (const char *name);

// Read option value by pointer
uint8_t read_uint8_option (const option_t *option);
inline bool read_bool_option (const option_t *option) { return (bool) read_uint8_option (option); };
uint16_t read_uint16_option (const option_t *option);
uint32_t read_uint32_option (const option_t *option);
float read_float_option (const option_t *option);
void read_str_option (const option_t *option, char *dest);

// Write option value by name
void write_uint8_option (const char *name, uint8_t value);
inline void write_bool_option (const char *name, bool value) { write_uint8_option (name, value); };
void write_uint16_option (const char *name, uint16_t value);
void write_uint32_option (const char *name, uint32_t value);
void write_float_option (const char *name, float value);
void write_str_option (const char *name, const char *value);

// Check EEPROM header & version and reset if needed
void init ();

// Write default settings to EEPROM
void reset ();

}

#endif /* SETTINGS_H_ */
