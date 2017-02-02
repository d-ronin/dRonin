/**
 ******************************************************************************
 *
 * @file       posix.h  
 * @author     Corvus Corax Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, 2013.
 * @brief      Definitions to run PiOS on posix
 * @see        The GNU Public License (GPL) Version 2
 *
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
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

#ifndef PIOS_POSIX_H
#define PIOS_POSIX_H

#include <stdint.h>

#include <stdbool.h>

#define FILEINFO FILE*

#define PIOS_SERVO_NUM_OUTPUTS 8
#define PIOS_SERVO_NUM_TIMERS PIOS_SERVO_NUM_OUTPUTS

#if (defined(_WIN32) || defined(WIN32) || defined(__MINGW32__))
#include <winsock2.h>
#include <ws2tcpip.h>
#include <dronin-strsep.h>
#endif

#endif

