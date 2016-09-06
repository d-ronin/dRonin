/* Imported from MultiOSD (https://github.com/UncleRus/MultiOSD/)
 * Altered for use on STM32 Flight controllers by dRonin
 * Copyright (C) dRonin 2016
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

typedef struct max7456_dev_s *max7456_dev_t;

void PIOS_MAX7456_wait_vsync ();

int PIOS_MAX7456_init (max7456_dev_t *dev_out,
		uint32_t spi_handle, uint32_t slave_idx);
void PIOS_MAX7456_clear (max7456_dev_t dev);
void PIOS_MAX7456_upload_char (max7456_dev_t dev, uint8_t char_index,
		uint8_t data []);
void PIOS_MAX7456_download_char (max7456_dev_t dev,
		uint8_t char_index, uint8_t data []);
void PIOS_MAX7456_put (max7456_dev_t dev, uint8_t col, uint8_t row,
		uint8_t chr, uint8_t attr);
void PIOS_MAX7456_puts (max7456_dev_t dev, uint8_t col, uint8_t row,
		const char *s, uint8_t attr);

void PIOS_MAX7456_open (max7456_dev_t dev, uint8_t col, uint8_t row,
		uint8_t attr);
void PIOS_MAX7456_close (max7456_dev_t dev);

void PIOS_MAX7456_get_extents(max7456_dev_t dev, 
		uint8_t *mode, uint8_t *right, uint8_t *bottom,
		uint8_t *hcenter, uint8_t *vcenter);

#endif /* LIB_MAX7456_MAX7456_H_ */
