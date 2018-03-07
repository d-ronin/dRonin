 /**
 ******************************************************************************
 * @file       pios_bmp280.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMP280 BMP280 Functions
 * @{
 * @brief Driver for BMP280
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

#ifndef PIOS_BMP280_H
#define PIOS_BMP280_H

#include <pios.h>

#if defined(PIOS_INCLUDE_BMP280)
#ifndef TARGET_MAY_HAVE_BARO
#define TARGET_MAY_HAVE_BARO
#endif

/* Public Functions */
extern int32_t PIOS_BMP280_Test();
#endif

#endif /* PIOS_BMP280_H */

/**
  * @}
  * @}
  */
