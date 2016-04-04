/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup Control Control Module
 * @{
 *
 * @file       failsafe_control.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @brief      Failsafe controller when transmitter control is lost
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "openpilot.h"
#include "control.h"
#include "failsafe_control.h"
#include "transmitter_control.h"

#include "flightstatus.h"
#include "stabilizationdesired.h"
#include "systemsettings.h"

//! Initialize the failsafe controller
int32_t failsafe_control_initialize()
{
	return 0;
}

//! Perform any updates to the failsafe controller
int32_t failsafe_control_update()
{
	return 0;
}

static bool armed_when_enabled;
/**
 * Select and use failsafe control
 * @param [in] reset_controller True if previously another controller was used
 */
int32_t failsafe_control_select(bool reset_controller)
{
	if (reset_controller) {
		FlightStatusArmedOptions armed; 
		FlightStatusArmedGet(&armed);
		armed_when_enabled = (armed == FLIGHTSTATUS_ARMED_ARMED);
	}

	uint8_t flight_status;
	FlightStatusFlightModeGet(&flight_status);
	if (flight_status != FLIGHTSTATUS_FLIGHTMODE_FAILSAFE || reset_controller) {
		flight_status = FLIGHTSTATUS_FLIGHTMODE_FAILSAFE;
		FlightStatusFlightModeSet(&flight_status);
	}

	SystemSettingsAirframeTypeOptions airframe_type;
	SystemSettingsAirframeTypeGet(&airframe_type);

	StabilizationDesiredData stabilization_desired;
	StabilizationDesiredGet(&stabilization_desired);
	stabilization_desired.Thrust = (airframe_type == SYSTEMSETTINGS_AIRFRAMETYPE_HELICP) ? 0 : -1;
	stabilization_desired.Roll = 0;
	stabilization_desired.Pitch = 0;
	stabilization_desired.Yaw   = 0;

	if (!armed_when_enabled) {
		/* disable stabilization so outputs do not move when system was not armed */
		stabilization_desired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL] = STABILIZATIONDESIRED_STABILIZATIONMODE_DISABLED;
		stabilization_desired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = STABILIZATIONDESIRED_STABILIZATIONMODE_DISABLED;
		stabilization_desired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW] = STABILIZATIONDESIRED_STABILIZATIONMODE_DISABLED;
	} else {
		stabilization_desired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL] = STABILIZATIONDESIRED_STABILIZATIONMODE_FAILSAFE;
		stabilization_desired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = STABILIZATIONDESIRED_STABILIZATIONMODE_FAILSAFE;
		stabilization_desired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW] = STABILIZATIONDESIRED_STABILIZATIONMODE_FAILSAFE;
	}

	StabilizationDesiredSet(&stabilization_desired);

	return 0;
}

//! Get any control events
enum control_events failsafe_control_get_events()
{
	// For now ARM / DISARM events still come from the transmitter.  This
	// means the normal disarm timeout still applies.  To be replaced later
	// by a full state machine determining how long to stay in failsafe before
	// disarming.
	return transmitter_control_get_events();
}

/**
 * @}
 * @}
 */

