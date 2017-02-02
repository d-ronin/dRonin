/**
 ******************************************************************************
 * @file       pios_srxl.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SRXL Multiplex SRXL protocol receiver driver
 * @{
 * @brief Supports receivers using the Multiplex SRXL protocol
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


#ifndef PIOS_SRXL_H
#define PIOS_SRXL_H

#include <pios.h>
#include <pios_usart_priv.h>

#define PIOS_SRXL_MAX_CHANNELS 16

extern const struct pios_rcvr_driver pios_srxl_rcvr_driver;

/**
 * @brief Initialize a new SRXL device
 * @param[in] serial_id Pointer to serial device driver
 * @param[in] driver Pointer to comm driver associated with serial device
 * @param[in] lower_id Pointer to associated low-level serial device driver
 * @return 0 on success, negative on failure
 */
int32_t PIOS_SRXL_Init(uintptr_t *serial_id,
	const struct pios_com_driver *driver, uintptr_t lower_id);

#endif /* PIOS_SRXL_H */

/**
 * @}
 * @}
 */
