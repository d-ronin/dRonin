/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup  PIOS_UAVTALKRCVR Receiver-over-UAVTALK Input Functions
 * @brief	Code to read the channels within the Receiver UAVObject
 * @{
 *
 * @file       pios_uavtalkrcvr_priv.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      GCS receiver private functions
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

#ifndef PIOS_UAVTALKRCVR_PRIV_H
#define PIOS_UAVTALKRCVR_PRIV_H

#include <pios.h>
#include "uavobjectmanager.h"	/* UAVObj* */
#include "uavtalkreceiver.h"

extern const struct pios_rcvr_driver pios_uavtalk_rcvr_driver;

extern int32_t PIOS_UAVTALKRCVR_Init(uintptr_t *gcsrcvr_id);

#endif /* PIOS_UAVTALKRCVR_PRIV_H */

/**
 * @}
 * @}
 */
