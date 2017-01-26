/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_ANNUNCDAC Annunciator DAC Functions
 * @brief PIOS interface for Annunciator DAC implementation
 * @{
 *
 * @file       pios_annuncdac.h
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

#ifndef _PIOS_ANNUNCDAC_H
#define _PIOS_ANNUNCDAC_H

#include <pios_dac.h>

typedef struct annuncdac_dev_s *annuncdac_dev_t;

/**
 * @brief Allocate and initialise ANNUNCDAC device
 * @param[out] annuncdac_id Device handle, only valid when return value is success
 * @param[in] dac Handle to a PIOS_DAC subsystem to use for xmit
 * @retval 0 on success, else failure
 */
int32_t PIOS_ANNUNCDAC_Init(annuncdac_dev_t *annuncdac_id, dac_dev_t dac);

/**
 * @brief Set whether we should be beeping.
 * @param[in] dev ANNUNCDAC device handle
 * @param[in] active Whether we are "in a symbol".  Currently unused.
 * @param[in] value True for beep, false for silence.
 */
void PIOS_ANNUNCDAC_SetValue(annuncdac_dev_t dev, bool active, bool value);

#endif /* _PIOS_ANNUNCDAC_H */
