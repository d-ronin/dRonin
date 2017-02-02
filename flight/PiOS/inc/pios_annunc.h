/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_ANNUNC Annunciator (LED, buzzer, etc, functions).
 * @{
 *
 * @file       pios_annunc.h   
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Annunciator functions header.
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

#ifndef PIOS_LED_H
#define PIOS_LED_H

/* Public Functions */
extern void PIOS_ANNUNC_On(uint32_t annunc_id);
extern void PIOS_ANNUNC_Off(uint32_t annunc_id);
extern void PIOS_ANNUNC_Toggle(uint32_t annunc_id);

#ifdef SIM_POSIX
extern bool PIOS_ANNUNC_GetStatus(uint32_t annunc_id); // Currently sim only
#endif


#endif /* PIOS_LED_H */
