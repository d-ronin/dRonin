/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @defgroup   PIOS_DEBUG Debugging Functions
 * @brief Debugging functionality
 * @{
 *
 * @file       pios_i2c.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Debug helper functions header.
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

#ifndef PIOS_DEBUG_H
#define PIOS_DEBUG_H

extern const char *PIOS_DEBUG_AssertMsg;

#ifndef PIOS_NO_HW
#include <pios_tim_priv.h>

void PIOS_DEBUG_Init(const struct pios_tim_channel * channels, uint8_t num_channels);
#endif

void PIOS_DEBUG_PinHigh(uint8_t pin);
void PIOS_DEBUG_PinLow(uint8_t pin);
void PIOS_DEBUG_PinValue8Bit(uint8_t value);
void PIOS_DEBUG_PinValue4BitL(uint8_t value);
void PIOS_DEBUG_Panic(const char *msg) __attribute__((noreturn));

#ifdef DEBUG
#define PIOS_DEBUG_Assert(test) do { if (!(test)) PIOS_DEBUG_Panic(PIOS_DEBUG_AssertMsg); } while (0)
#define PIOS_Assert(test) PIOS_DEBUG_Assert(test)
#else
#define PIOS_DEBUG_Assert(test) do { } while(0)
#define PIOS_Assert(test) do { if (!(test)) PIOS_DEBUG_Panic(NULL); } while (0)
#endif

#define PIOS_Abort() PIOS_Assert(0)

#endif /* PIOS_DEBUG_H */

/**
  * @}
  * @}
  */
