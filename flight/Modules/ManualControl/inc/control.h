/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup Control Control Module
 * @{
 *
 * @file       manualcontrol.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @brief      Control module. Handles safety R/C link and flight mode.
 *
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
#ifndef CONTROL_H
#define CONTROL_H

enum control_status {
	STATUS_DISCONNECTED,	///< RX is disconnected
	STATUS_ERROR,           ///< Something is fundamentally wrong
	STATUS_SAFETYTIMEOUT,	///< Disconnected timeout has occurred
		// Lower layer (transmitter control) responsible for timer.
	STATUS_DISARM,		///< User requested disarm, or low throt timeout
		// Lower layer (transmitter control) responsible for timers.
	STATUS_NORMAL,		///< Things are "normal"
	STATUS_ARM_INVALID,	///< User requested arm, controls in invalid pos
	STATUS_ARM_VALID,	///< User requested arm, controls in valid pos
		// For both of these, upper layer (manual control) responsible
		// for timer.  (So that manual control can manage "ARMING" state)
	STATUS_INVALID_FOR_DISARMED,
				///< Arming switch is in a state that doesn't
		// make sense for initial arming.. i.e. high throttle.  Go to
		// safety state
};

#endif /* CONTROL_H */

/**
 * @}
 * @}
 */
