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
#ifndef LIB_MAX7456_MAX7456_H_
#define LIB_MAX7456_MAX7456_H_

#include <stdint.h>
#include <stdio.h>

#define MAX7456_MODE_PAL	0
#define MAX7456_MODE_NTSC	1

#define MAX7456_ATTR_NONE   0
#define MAX7456_ATTR_INVERT 1
#define MAX7456_ATTR_BLINK 	2
#define MAX7456_ATTR_LBC 	4

#define MAX7456_PAL_COLUMNS		30
#define MAX7456_PAL_ROWS		16
#define MAX7456_PAL_HCENTER		(MAX7456_PAL_COLUMNS / 2)
#define MAX7456_PAL_VCENTER		(MAX7456_PAL_ROWS / 2)

#define MAX7456_NTSC_COLUMNS	30
#define MAX7456_NTSC_ROWS		13
#define MAX7456_NTSC_HCENTER	(MAX7456_NTSC_COLUMNS / 2)
#define MAX7456_NTSC_VCENTER	(MAX7456_NTSC_ROWS / 2)

namespace max7456
{

namespace settings
{
	void init ();
	void reset ();
}

void wait_vsync ();

void init ();
void clear ();
void clear (uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void upload_char (uint8_t char_index, uint8_t data []);
void download_char (uint8_t char_index, uint8_t data []);
void put (uint8_t col, uint8_t row, uint8_t chr, uint8_t attr = 0);
void puts (uint8_t col, uint8_t row, const char *s, uint8_t attr = 0);
void puts_p (uint8_t col, uint8_t row, const char *progmem_str, uint8_t attr = 0);

void open (uint8_t col, uint8_t row, uint8_t attr = 0);
void open_center (uint8_t width, uint8_t height, uint8_t attr = 0);
void open_hcenter (uint8_t width, uint8_t row, uint8_t attr = 0);
void open_vcenter (uint8_t col, uint8_t height, uint8_t attr = 0);
void __attribute__ ((noinline)) close ();

extern FILE stream;
extern uint8_t mode, right, bottom, hcenter, vcenter;

};

#endif /* LIB_MAX7456_MAX7456_H_ */
