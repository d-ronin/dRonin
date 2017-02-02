/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_HMC5843 HMC5843 Functions
 * @brief Deals with the hardware interface to the magnetometers
 * @{
 *
 * @file       pios_hmc5843.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      HMC5843 functions header.
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
 */

#ifndef PIOS_HMC5843_H
#define PIOS_HMC5843_H

/* Public Functions */
extern void PIOS_HMC5843_Init(void);
extern bool PIOS_HMC5843_NewDataAvailable(void);
extern void PIOS_HMC5843_ReadMag(int16_t out[3]);
extern void PIOS_HMC5843_ReadID(uint8_t out[4]);

#endif /* PIOS_HMC5843_H */

/** 
  * @}
  * @}
  */
