/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_FSKDAC FSK DAC Functions
 * @brief PIOS interface for FSK DAC implementation
 * @{
 *
 * @file       pios_fskdac.h
 * @author     dRonin, http://dronin.org Copyright (C) 2017
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef _PIOS_FSKDAC_H
#define _PIOS_FSKDAC_H

#include <pios_dac.h>

typedef struct fskdac_dev_s *fskdac_dev_t;

/**
 * @brief Allocate and initialise FSKDAC device
 * @param[out] fskdac_id Device handle, only valid when return value is success
 * @param[in] dac Handle to a PIOS_DAC subsystem to use for xmit
 * @retval 0 on success, else failure
 */
int32_t PIOS_FSKDAC_Init(fskdac_dev_t *fskdac_id, dac_dev_t dac);

extern const struct pios_com_driver pios_fskdac_com_driver;

#endif /* _PIOS_FSKDAC_H */
