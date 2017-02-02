/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MAX7456 Max7456 Functions
 *
 * @file       pios_max7456.c
 * @author     dRonin, http://dronin.org Copyright (C) 2016
 * @author     @UncleRus and MultiOSD
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

// NTSC and PAL have the same number of columns.
#define MAX7456_COLUMNS MAX7456_NTSC_COLUMNS
#define MAX7456_HCENTER MAX7456_NTSC_HCENTER

// Special formatting values
#define MAX7456_FMT_H_CENTER 31

typedef struct max7456_dev_s *max7456_dev_t;

void PIOS_MAX7456_wait_vsync ();

/**
 * @brief Allocate and initialise MAX7456 device
 * @param[out] dev_out Device handle, only valid when return value is success
 * @param[in] spi_dev SPI device driver handle for communication with MAX7456
 * @param[in] spi_slave SPI slave number
 * @retval 0 on success, else failure
 */
int PIOS_MAX7456_init (max7456_dev_t *dev_out,
		uint32_t spi_handle, uint32_t slave_idx);

/**
 * @brief Clear the screen
 * @param[in] dev The max7456 device handle
 */
void PIOS_MAX7456_clear (max7456_dev_t dev);

/**
 * @brief Upload a character to the device
 * @param[in] dev The max7456 device handle
 * @param[in] char_index The index of the character memory to update
 * @param[in] data A pointer to 54 bytes of memory (18x12x2 bpp)
 */
void PIOS_MAX7456_upload_char (max7456_dev_t dev, uint8_t char_index,
		const uint8_t *data);

/**
 * @brief Download a character from the device
 * @param[in] dev The max7456 device handle
 * @param[in] char_index The index of the character memory to retrieve
 * @param[out] data 54 bytes of memory (18x12x2 bpp) filled with the character.
 */
void PIOS_MAX7456_download_char (max7456_dev_t dev,
		uint8_t char_index, uint8_t *data);

/**
 * @brief Sets a position of character memory
 * @param[in] dev The max7456 device handle
 * @param[in] col The column to update
 * @param[in] row The row of the character to update
 * @param[in] chr The character to store at this position
 * @param[in] attr An attribute mask for this character.
 */
void PIOS_MAX7456_put (max7456_dev_t dev, uint8_t col, uint8_t row,
		uint8_t chr, uint8_t attr);

/**
 * @brief Sets a string into character memory
 * @param[in] dev The max7456 device handle
 * @param[in] col The column to begin the update at
 * @param[in] row The row of the character to update
 * @param[in] s The string to store at this position
 * @param[in] attr An attribute mask for this string
 */
void PIOS_MAX7456_puts (max7456_dev_t dev, uint8_t col, uint8_t row,
		const char *s, uint8_t attr);

/**
 * @brief Gets the extents of the screen.
 * @param[in] dev The max7456 device handle
 * @param[out] mode Video mode-- MAX7456_MODE_PAL or MAX7456_MODE_NTSC
 * @param[out] right A screen coordinate
 * @param[out] bottom A screen coordinate
 * @param[out] hcenter Horizontal center of the screen
 * @param[out] vcenter Vertical center of the screen
 */
void PIOS_MAX7456_get_extents(max7456_dev_t dev, 
		uint8_t *mode, uint8_t *right, uint8_t *bottom,
		uint8_t *hcenter, uint8_t *vcenter);

/**
 * @brief Waits for vsync in a configuration-dependant way
 * @param[in] dev The max7456 device handle
 */
void PIOS_MAX7456_wait_vsync(max7456_dev_t dev);

/**
 * @brief Detects whether the OSD chip has stalled and attempts to restart it.
 * @param[in] dev The max7456 device handle
 * @return true if the chip was detected to have stalled
 */
bool PIOS_MAX7456_stall_detect(max7456_dev_t dev);

/**
 * @brief Allows overriding the video mode used by OSD.
 * @param[in] dev The max7456 dev handle
 * @param[in] force Whether to force the mode and skip autodetection.
 * @param[in] fallback The mode to select if autodetection fails or is skipped.
 */
void PIOS_MAX7456_set_mode(max7456_dev_t dev, bool force, uint8_t fallback);

#endif /* LIB_MAX7456_MAX7456_H_ */
