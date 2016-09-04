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
#include "max7456.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "../common.h"
#include "../spi/spi.h"
#include "../../settings.h"
#include "../../config.h"
#include "../../eeprom.h"

// MAX7456 reg read addresses
#define MAX7456_REG_STAT  0x20 // 0xa0 Status
#define MAX7456_REG_DMDO  0x30 // 0xb0 Display Memory Data Out
#define MAX7456_REG_CMDO  0x40 // 0xc0 Character Memory Data Out

// MAX7456 reg write addresses
#define MAX7456_REG_VM0   0x00
#define MAX7456_REG_VM1   0x01
#define MAX7456_REG_DMM   0x04
#define MAX7456_REG_DMAH  0x05
#define MAX7456_REG_DMAL  0x06
#define MAX7456_REG_DMDI  0x07
#define MAX7456_REG_OSDM  0x0c // not used. Is to set mix
#define MAX7456_REG_OSDBL 0x6c // black level

// MAX7456 reg write addresses to recording NVM process
#define MAX7456_REG_CMM   0x08
#define MAX7456_REG_CMAH  0x09
#define MAX7456_REG_CMAL  0x0a
#define MAX7456_REG_CMDI  0x0b

#define MAX7456_MASK_PAL  0x40 // PAL mask 01000000
#define MAX7456_MASK_NTCS 0x00 // NTSC mask 00000000

namespace max7456
{

namespace settings
{

#define EEPROM_ADDR_MODE       _eeprom_byte (MAX7456_EEPROM_OFFSET)
#define EEPROM_ADDR_BRIGHTNESS _eeprom_byte (MAX7456_EEPROM_OFFSET + 1)

const char __opt_mode [] PROGMEM = "VMODE";
const char __opt_brightness [] PROGMEM = "VBRIGHT";

const ::settings::option_t __settings [] PROGMEM = {
	declare_uint8_option (__opt_mode, EEPROM_ADDR_MODE),
	declare_uint8_option (__opt_brightness, EEPROM_ADDR_BRIGHTNESS),
};

bool initialized = false;

void init ()
{
	if (initialized) return;
	::settings::append_section (__settings, sizeof (__settings) / sizeof (::settings::option_t));
	initialized = true;
}

void reset ()
{
	eeprom_write_byte (EEPROM_ADDR_MODE, MAX7456_DEFAULT_MODE);
	eeprom_write_byte (EEPROM_ADDR_BRIGHTNESS, MAX7456_DEFAULT_BRIGHTNESS);
}

}  // namespace settings

///////////////////////////////////////////////////////////////////////////////

uint8_t mode, right, bottom, hcenter, vcenter;

uint8_t _mask = 0;
bool _opened = false;

#define _chip_select() do { cbi (MAX7456_SELECT_PORT, MAX7456_SELECT_BIT); } while (0)
#define _chip_unselect() do { sbi (MAX7456_SELECT_PORT, MAX7456_SELECT_BIT); } while (0)

// Write VM0[3] = 1 to enable the display of the OSD image.
// mode | autosync | vsync on | enable OSD
#define _enable_osd() do { write_register (MAX7456_REG_VM0, _mask | 0x0c); } while (0)
#define _disable_osd() do { write_register (MAX7456_REG_VM0, 0); } while (0)

void wait_vsync ()
{
	loop_until_bit_is_clear (MAX7456_VSYNC_PIN, MAX7456_VSYNC_BIT);
}

uint8_t read_register (uint8_t reg)
{
	spi::transfer (reg | 0x80);
	return spi::transfer (0xff);
}

void write_register (uint8_t reg, uint8_t val)
{
	spi::transfer (reg);
	spi::transfer (val);
}

inline void _set_mode (uint8_t value)
{
	mode = value;
	if (value == MAX7456_MODE_NTSC)
	{
		_mask = MAX7456_MASK_NTCS;
		right = MAX7456_NTSC_COLUMNS - 1;
		bottom = MAX7456_NTSC_ROWS - 1;
		hcenter = MAX7456_NTSC_HCENTER;
		vcenter = MAX7456_NTSC_VCENTER;
		return;
	}
	_mask = MAX7456_MASK_PAL;
	right = MAX7456_PAL_COLUMNS - 1;
	bottom = MAX7456_PAL_ROWS - 1;
	hcenter = MAX7456_PAL_HCENTER;
	vcenter = MAX7456_PAL_VCENTER;
}

inline void _detect_mode ()
{
	if (bis (MAX7456_MODE_JUMPER_PORT, MAX7456_MODE_JUMPER_BIT))
	{
		// jumper is closed, ignore settings
		_set_mode (MAX7456_DEFAULT_MODE);
		return;
	}

	_chip_select ();
	// read STAT and auto detect video mode PAL/NTSC
	uint8_t stat = read_register (MAX7456_REG_STAT);
	_chip_unselect ();

	if (bis (stat, 0))
	{
		_set_mode (MAX7456_MODE_PAL);
		return;
	}
	if (bis (stat, 1))
	{
		_set_mode (MAX7456_MODE_NTSC);
		return;
	}

	_set_mode (eeprom_read_byte (EEPROM_ADDR_MODE));
}

FILE stream;

int _put (char chr, FILE *s);

void init ()
{
	// Prepare pins
	cbi (MAX7456_MODE_JUMPER_DDR, MAX7456_MODE_JUMPER_BIT);
	sbi (MAX7456_SELECT_DDR, MAX7456_SELECT_BIT);
	cbi (MAX7456_VSYNC_DDR, MAX7456_VSYNC_BIT);
	sbi (MAX7456_VSYNC_PORT, MAX7456_VSYNC_BIT); // pull-up

	// Reset
	write_register (MAX7456_REG_VM0, _BV (1));
	_delay_us (100);
	while (read_register (MAX7456_REG_VM0) & _BV (1))
		;

	// Detect video mode
	_detect_mode ();

	_chip_select ();
	/*
	Write OSDBL[4] = 0 to enable automatic OSD black
	level control. This ensures the correct OSD image
	brightness. This register contains 4 factory-preset
	bits [3:0] that must not be changed.
	*/
	write_register (MAX7456_REG_OSDBL, read_register (MAX7456_REG_OSDBL) & 0xef);

	write_register (MAX7456_REG_VM1, 0x7f);

	_enable_osd ();

	// set all rows to the same character brightness black/white level
	uint8_t brightness = eeprom_read_byte (EEPROM_ADDR_BRIGHTNESS);
	for (uint8_t r = 0; r < 16; ++ r)
		write_register (0x10 + r, brightness);

	_chip_unselect ();

	fdev_setup_stream (&stream, _put, NULL, _FDEV_SETUP_WRITE);
}

void clear ()
{
	if (_opened) close ();
	_chip_select ();
	write_register (MAX7456_REG_DMM, 0x04);
	_chip_unselect ();
	_delay_us (30);
}

void upload_char (uint8_t char_index, uint8_t data [])
{
	_chip_select ();
	_disable_osd ();

	_delay_us (10);

	// Write CMAH[7:0] = xxH to select the character (0–255) to be written
	write_register (MAX7456_REG_CMAH, char_index);

	for (uint8_t i = 0; i < 54; i ++)
	{
		// Write CMAL[7:0] = xxH to select the 4-pixel byte (0–53) in the character to be written
		write_register (MAX7456_REG_CMAL, i);
		// Write CMDI[7:0] = xxH to set the pixel values of the selected part of the character
		write_register (MAX7456_REG_CMDI, data [i]);
	}

	// Write CMM[7:0] = 1010xxxx to write to the NVM array from the shadow RAM
	write_register (MAX7456_REG_CMM, 0xa0);
	/*
	The character memory is busy for approximately 12ms during this operation.
	STAT[5] can be read to verify that the NVM writing process is complete.
	*/
	while (read_register (MAX7456_REG_STAT) & 0x20)
		;

	_enable_osd ();
	_chip_unselect ();
}

void download_char (uint8_t char_index, uint8_t data [])
{
	_chip_select ();
	_disable_osd ();

	_delay_us (10);

	// Write CMAH[7:0] = xxH to select the character (0–255) to be read
	write_register (MAX7456_REG_CMAH, char_index);

	// Write CMM[7:0] = 0101xxxx to read the character data from the NVM to the shadow RAM
	write_register (MAX7456_REG_CMM, 0x50);
	/*
	 * The character memory is busy for approximately 12ms during this operation.
	 * The Character Memory Mode register is cleared and STAT[5] is reset to 0 after
	 * the write operation has been completed.
	 */
	while (read_register (MAX7456_REG_STAT) & 0x20)
		;

	for (uint8_t i = 0; i < 54; i ++)
	{
		// Write CMAL[7:0] = xxH to select the 4-pixel byte (0–53) in the character to be written
		write_register (MAX7456_REG_CMAL, i);
		// Write CMDI[7:0] = xxH to set the pixel values of the selected part of the character
		data [i] = read_register (MAX7456_REG_CMDO);
	}

	_enable_osd ();
	_chip_unselect ();
}

inline void _set_offset (uint8_t col, uint8_t row)
{
	uint16_t offset = (row * 30 + col) & 0x1ff;
	write_register (MAX7456_REG_DMAH, offset >> 8);
	write_register (MAX7456_REG_DMAL, (uint8_t) offset);
}

void put (uint8_t col, uint8_t row, uint8_t chr, uint8_t attr)
{
	if (_opened) close ();

	_chip_select ();
	_set_offset (col, row);
	write_register (MAX7456_REG_DMM, (attr & 0x07) << 3);
	write_register (MAX7456_REG_DMDI, chr);
	_chip_unselect ();
}

void open (uint8_t col, uint8_t row, uint8_t attr)
{
	if (_opened) close ();

	_opened = true;

	_chip_select ();
	_set_offset (col > right ? 0 : col, row > bottom ? 0 : row);
	// 16 bits operating mode, char attributes, autoincrement
	write_register (MAX7456_REG_DMM, ((attr & 0x07) << 3) | 0x01);
}

void open_center (uint8_t width, uint8_t height, uint8_t attr)
{
	open (hcenter - width / 2, vcenter - width / 2, attr);
}

void open_hcenter (uint8_t width, uint8_t row, uint8_t attr)
{
	open (hcenter - width / 2, row, attr);
}

void open_vcenter (uint8_t col, uint8_t height, uint8_t attr)
{
	open (col, vcenter - height / 2, attr);
}

void __attribute__ ((noinline)) close ()
{
	if (!_opened) return;

	// terminate autoincrement mode
	write_register (MAX7456_REG_DMDI, 0xff);
	_chip_unselect ();
	_opened = false;
}

// 0xff terminates autoincrement mode
#define _valid_char(c) (c == 0xff ? 0x00 : c)

int _put (char chr, FILE *s)
{
	write_register (MAX7456_REG_DMDI, _valid_char (chr));
	return 0;
}

void puts (uint8_t col, uint8_t row, const char *s, uint8_t attr)
{
	open (col, row, attr);
	while (*s)
	{
		write_register (MAX7456_REG_DMDI, _valid_char (*s));
		s ++;
	}
	close ();
}

void puts_p (uint8_t col, uint8_t row, const char *progmem_str, uint8_t attr)
{
	open (col, row, attr);
	register char c;
	while ((c = pgm_read_byte (progmem_str ++)))
		write_register (MAX7456_REG_DMDI, _valid_char (c));
	close ();
}

void clear (uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	for (uint8_t r = 0; r < h; r ++)
	{
		open (x, y + r);
		for (uint8_t i = 0; i < w; i ++)
			write_register (MAX7456_REG_DMDI, 0x00);
		close ();
	}
}

}  // namespace max7456
