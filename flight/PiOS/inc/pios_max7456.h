/**
 ******************************************************************************
 * @file       pios_max7456.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_Max7456 PiOS MAX7456 Driver
 * @{
 * @brief Driver for MAX7456 OSD
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef PIOS_MAX7456_H_
#define PIOS_MAX7456_H_

#include "pios_spi.h"

/* Opaque structures */
struct pios_max7456_dev;
struct pios_max7456_cfg;

/* Public types */
enum pios_max7456_video_type {
	PIOS_MAX7456_VIDEO_UNKNOWN,
	PIOS_MAX7456_VIDEO_PAL,
	PIOS_MAX7456_VIDEO_NTSC,
};

/* Public methods */
/**
 * @brief Allocate and initialise MAX7456 device
 * @param[out] max7456_dev Device handle, only valid when return value is success
 * @param[in] spi_dev SPI device driver handle for communication with MAX7456
 * @param[in] spi_slave SPI slave number
 * @param[in] cfg MAX7456 device configuration
 * @retval 0 on success, else failure
 */
int32_t PIOS_MAX7456_Init(struct pios_max7456_dev **max7456_dev, struct pios_spi_dev *spi_dev, uint32_t spi_slave, const struct pios_max7456_cfg *cfg);

/**
 * @brief Get detected video type
 * @param[in] dev MAX7456 device handle
 * @retval Video type
 */
enum pios_max7456_video_type PIOS_MAX7456_GetVideoType(struct pios_max7456_dev *dev);

/**
 * @brief Set output video type
 * @param[in] dev MAX7456 device handle
 * @param[in] type Video type
 * @retval 0 on success, failure otherwise
 */
int32_t PIOS_MAX7456_SetOutputType(struct pios_max7456_dev *dev, enum pios_max7456_video_type type);

#endif // PIOS_MAX7456_H_

/**
 * @}
 * @}
 */
