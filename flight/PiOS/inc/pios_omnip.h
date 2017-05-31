/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_OMNIP Driver for OmniPreSense mm-wave radar modules
 * @{
 *
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @brief      Interfaces with simple serial-based doppler & ranging devices
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

#ifndef _PIOS_OMNIP_H
#define _PIOS_OMNIP_H

#include <pios_com.h>

typedef struct omnip_dev_s *omnip_dev_t;

/**
 * @brief Allocate and initialise OMNIP device
 * @param[out] dev Device handle, only valid when return value is success
 * @param[in] lower_id PIOS_COM handle
 *
 * @retval 0 on success, else failure
 */
int32_t PIOS_OMNIP_Init(omnip_dev_t *dev,
		const struct pios_com_driver *driver,
		uintptr_t lower_id);

#endif /* _PIOS_OMNIP_H */
