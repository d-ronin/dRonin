/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup Control Control Module
 * @{
 *
 * @file       geofence_control.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2015
 * @brief      Geofence controller that is enabled when out of zone
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "openpilot.h"
#include "control.h"
#include "geofence_control.h"
#include "transmitter_control.h"

#include "flightstatus.h"
#include "geofencesettings.h"
#include "stabilizationdesired.h"
#include "systemalarms.h"
#include "systemsettings.h"

//! Initialize the geofence controller
int32_t geofence_control_initialize()
{
	return 0;
}

//! Perform any updates to the geofence controller
int32_t geofence_control_update()
{
	return 0;
}

static bool geofence_armed_when_enabled;

//! Use geofence control mode
int32_t geofence_control_select(bool reset_controller)
{
	// This behavior is the same as the current failsafe. 
	if (reset_controller) {
		FlightStatusArmedOptions armed; 
		FlightStatusArmedGet(&armed);
		geofence_armed_when_enabled = (armed == FLIGHTSTATUS_ARMED_ARMED);
	}

	uint8_t flight_status;
	FlightStatusFlightModeGet(&flight_status);
	if (flight_status != FLIGHTSTATUS_FLIGHTMODE_STABILIZED1 || reset_controller) {
		flight_status = FLIGHTSTATUS_FLIGHTMODE_STABILIZED1;
		FlightStatusFlightModeSet(&flight_status);
	}

	SystemSettingsAirframeTypeOptions airframe_type;
	SystemSettingsAirframeTypeGet(&airframe_type);

	// Pick default values that will roughly cause a plane to circle down
	// and a quad to fall straight down
	StabilizationDesiredData stabilization_desired;
	StabilizationDesiredGet(&stabilization_desired);
	StabilizationDesiredGet(&stabilization_desired);
	stabilization_desired.Thrust = (airframe_type == SYSTEMSETTINGS_AIRFRAMETYPE_HELICP) ? 0 : -1;
	stabilization_desired.Roll  = 0;
	stabilization_desired.Pitch = 0;
	stabilization_desired.Yaw   = 0;
	stabilization_desired.ReprojectionMode = STABILIZATIONDESIRED_REPROJECTIONMODE_NONE;

	if (!geofence_armed_when_enabled) {
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

//! Query if out of bounds and the geofence controller should take over
bool geofence_control_activate()
{
	// Check if the module is running
	if (GeoFenceSettingsHandle()) {
		uint8_t alarm_status[SYSTEMALARMS_ALARM_NUMELEM];
		SystemAlarmsAlarmGet(alarm_status);

		if (alarm_status[SYSTEMALARMS_ALARM_GEOFENCE] == SYSTEMALARMS_ALARM_ERROR ||
			alarm_status[SYSTEMALARMS_ALARM_GEOFENCE] == SYSTEMALARMS_ALARM_CRITICAL) {
			return true;
		}
	}

	return false;
}

/**
 * @}
 * @}
 */

