/**
 ******************************************************************************
 * @file       pios_ibus.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_IBus PiOS IBus receiver driver
 * @{
 * @brief Receives and decodes IBus protocol reciever packets
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

#include "pios.h"

#ifndef PIOS_IBUS_H
#define PIOS_IBUS_H

#include "pios_rcvr.h"

extern const struct pios_rcvr_driver pios_ibus_rcvr_driver;

/**
 * @brief Initialises the IBus Rx driver with a serial port
 * @param[out] ibus_id IBus receiver device handle
 * @param[in] driver PiOS COM driver for the serial port
 * @param[in] uart_id UART port driver handle
 * @retval 0 on success, otherwise error
 */
int PIOS_IBus_Init(uintptr_t *ibus_id, const struct pios_com_driver *driver,
		uintptr_t uart_id);

#endif // PIOS_IBUS_H

  /**
  * @}
  * @}
  */
