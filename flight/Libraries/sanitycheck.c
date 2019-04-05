/**
 ******************************************************************************
 * @addtogroup Libraries Libraries
 * @{
 *
 * @file       sanitycheck.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @brief      Utilities to validate a flight configuration
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
#include "taskmonitor.h"
#include <pios_board_info.h>
#include "flightstatus.h"
#include "sanitycheck.h"
#include "manualcontrolsettings.h"
#include "stabilizationsettings.h"
#include "stateestimation.h"
#include "systemalarms.h"
#include "systemsettings.h"
#include "systemident.h"

#include "pios_sensors.h"

/****************************
 * Current checks:
 * 1. If a flight mode switch allows autotune and autotune module not running
 * 2. If airframe is a multirotor and either manual is available or a stabilization mode uses "none"
 ****************************/

//! Check it is safe to arm in this position
static int32_t check_safe_to_arm();

//! Check a stabilization mode switch position for safety
static int32_t check_stabilization_settings(int index, bool multirotor);

//! Check a stabilization mode switch position for safety
static int32_t check_stabilization_rates();

//! Check the system is safe for autonomous flight
static int32_t check_safe_autonomous();

bool lqg_sysident_check()
{
	if (SystemIdentHandle()) {
		SystemIdentData si;
		SystemIdentGet(&si);

		if (si.Beta[SYSTEMIDENT_BETA_ROLL] < 6 || si.Tau[SYSTEMIDENT_TAU_ROLL] < 0.001f ||
			si.Beta[SYSTEMIDENT_BETA_PITCH] < 6 || si.Tau[SYSTEMIDENT_TAU_PITCH] < 0.001f ||
			si.Beta[SYSTEMIDENT_BETA_YAW] < 6 || si.Tau[SYSTEMIDENT_TAU_YAW] < 0.001f) {
			return false;
		}
	}
	return true;
}

/**
 * Run a preflight check over the hardware configuration
 * and currently active modules
 */
int32_t configuration_check()
{
	SystemAlarmsConfigErrorOptions error_code = SYSTEMALARMS_CONFIGERROR_NONE;
	
	// For when modules are not running we should explicitly check the objects are
	// valid
	if (ManualControlSettingsHandle() == NULL ||
		SystemSettingsHandle() == NULL) {
		AlarmsSet(SYSTEMALARMS_ALARM_SYSTEMCONFIGURATION, SYSTEMALARMS_ALARM_CRITICAL);
		return 0;
	}

	// Classify airframe type
	bool multirotor = true;
	uint8_t airframe_type;
	SystemSettingsAirframeTypeGet(&airframe_type);
	switch(airframe_type) {
		case SYSTEMSETTINGS_AIRFRAMETYPE_QUADX:
		case SYSTEMSETTINGS_AIRFRAMETYPE_QUADP:
		case SYSTEMSETTINGS_AIRFRAMETYPE_HEXA:
		case SYSTEMSETTINGS_AIRFRAMETYPE_OCTO:
		case SYSTEMSETTINGS_AIRFRAMETYPE_HEXAX:
		case SYSTEMSETTINGS_AIRFRAMETYPE_OCTOV:
		case SYSTEMSETTINGS_AIRFRAMETYPE_OCTOCOAXP:
		case SYSTEMSETTINGS_AIRFRAMETYPE_HEXACOAX:
		case SYSTEMSETTINGS_AIRFRAMETYPE_TRI:
			multirotor = true;
			break;
		default:
			multirotor = false;
	}

	// For each available flight mode position sanity check the available
	// modes
	uint8_t num_modes;
	uint8_t modes[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_NUMELEM];
	ManualControlSettingsFlightModeNumberGet(&num_modes);
	ManualControlSettingsFlightModePositionGet(modes);

	for(uint32_t i = 0; i < num_modes; i++) {
		switch(modes[i]) {
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_MANUAL:
				if (multirotor) {
					error_code = SYSTEMALARMS_CONFIGERROR_STABILIZATION;
				}
				break;
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_ACRO:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_ACROPLUS:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_ACRODYNE:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_LEVELING:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_HORIZON:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_AXISLOCK:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_VIRTUALBAR:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_FLIPREVERSED:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_FAILSAFE:
				// always ok
				break;
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_LQG:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_LQGLEVELING:
				if (!lqg_sysident_check()) {
					error_code = SYSTEMALARMS_CONFIGERROR_LQG;
				}
				break;
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_STABILIZED1:
				error_code = (error_code == SYSTEMALARMS_CONFIGERROR_NONE) ? check_stabilization_settings(1, multirotor) : error_code;
				break;
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_STABILIZED2:
				error_code = (error_code == SYSTEMALARMS_CONFIGERROR_NONE) ? check_stabilization_settings(2, multirotor) : error_code;
				break;
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_STABILIZED3:
				error_code = (error_code == SYSTEMALARMS_CONFIGERROR_NONE) ? check_stabilization_settings(3, multirotor) : error_code;
				break;
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_AUTOTUNE:
				if (!PIOS_Modules_IsEnabled(PIOS_MODULE_AUTOTUNE)) {
					error_code = SYSTEMALARMS_CONFIGERROR_AUTOTUNE;
				}
				break;
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_ALTITUDEHOLD:
				if (!PIOS_SENSORS_IsRegistered(PIOS_SENSOR_BARO)) {
					error_code = SYSTEMALARMS_CONFIGERROR_ALTITUDEHOLD;
				}
				break;
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_POSITIONHOLD:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_RETURNTOHOME:
				if (!TaskMonitorQueryRunning(TASKINFO_RUNNING_PATHFOLLOWER)) {
					error_code = SYSTEMALARMS_CONFIGERROR_PATHPLANNER;
				} else {
					error_code = check_safe_autonomous();
				}
				break;
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_PATHPLANNER:
			case MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_TABLETCONTROL:
				if (!TaskMonitorQueryRunning(TASKINFO_RUNNING_PATHFOLLOWER) ||
					!TaskMonitorQueryRunning(TASKINFO_RUNNING_PATHPLANNER)) {
					error_code = SYSTEMALARMS_CONFIGERROR_PATHPLANNER;
				} else {
					error_code = check_safe_autonomous();
				}
				break;
			default:
				// Uncovered modes are automatically an error
				error_code = SYSTEMALARMS_CONFIGERROR_UNDEFINED;
		}
	}

	// Check the stabilization rates are within what the sensors can track
	error_code = (error_code == SYSTEMALARMS_CONFIGERROR_NONE) ? check_stabilization_rates() : error_code;

	// Only check safe to arm if no other errors exist
	error_code = (error_code == SYSTEMALARMS_CONFIGERROR_NONE) ? check_safe_to_arm() : error_code;

	set_config_error(error_code);

	return 0;
}


DONT_BUILD_IF(MANUALCONTROLSETTINGS_STABILIZATION1SETTINGS_NUMELEM != MANUALCONTROLSETTINGS_STABILIZATION2SETTINGS_NUMELEM, StabSettingsNumElem);
DONT_BUILD_IF(MANUALCONTROLSETTINGS_STABILIZATION1SETTINGS_NUMELEM != MANUALCONTROLSETTINGS_STABILIZATION3SETTINGS_NUMELEM, StabSettingsNumElem2);

/**
 * Checks the stabiliation settings for a paritcular mode and makes
 * sure it is appropriate for the airframe
 * @param[in] index Which stabilization mode to check
 * @returns SYSTEMALARMS_CONFIGERROR_NONE or SYSTEMALARMS_CONFIGERROR_MULTIROTOR
 */
static int32_t check_stabilization_settings(int index, bool multirotor)
{
	uint8_t modes[MANUALCONTROLSETTINGS_STABILIZATION1SETTINGS_NUMELEM];
	uint8_t thrust_mode;

	// Get the different axis modes for this switch position
	switch(index) {
		case 1:
			ManualControlSettingsStabilization1SettingsGet(modes);
			ManualControlSettingsStabilization1ThrustGet(&thrust_mode);
			break;
		case 2:
			ManualControlSettingsStabilization2SettingsGet(modes);
			ManualControlSettingsStabilization2ThrustGet(&thrust_mode);
			break;
		case 3:
			ManualControlSettingsStabilization3SettingsGet(modes);
			ManualControlSettingsStabilization3ThrustGet(&thrust_mode);
			break;
		default:
			return SYSTEMALARMS_CONFIGERROR_NONE;
	}

	// For multirotors verify that nothing is set to "disabled" or "manual"
	if (multirotor &&
			(thrust_mode != MANUALCONTROLSETTINGS_STABILIZATION1THRUST_FLIPOVERMODE) &&
			(thrust_mode != MANUALCONTROLSETTINGS_STABILIZATION1THRUST_FLIPOVERMODETHRUSTREVERSED)) {
		for(uint32_t i = 0; i < NELEMENTS(modes); i++) {
			if ((modes[i] == MANUALCONTROLSETTINGS_STABILIZATION1SETTINGS_DISABLED || modes[i] == MANUALCONTROLSETTINGS_STABILIZATION1SETTINGS_MANUAL))
				return SYSTEMALARMS_CONFIGERROR_MULTIROTOR;
		}
	}

	if ((thrust_mode == SHAREDDEFS_THRUSTMODESTABBANK_ALTITUDEWITHSTICKSCALING) &&
			(!PIOS_SENSORS_IsRegistered(PIOS_SENSOR_BARO))) {
		return SYSTEMALARMS_CONFIGERROR_ALTITUDEHOLD;
	}

	for(uint32_t i = 0; i < NELEMENTS(modes); i++) {
		// If this axis allows enabling an autotune behavior without the module
		// running then set an alarm now that aututune module initializes the
		// appropriate objects
		if ((modes[i] == MANUALCONTROLSETTINGS_STABILIZATION1SETTINGS_SYSTEMIDENT) &&
				(!PIOS_Modules_IsEnabled(PIOS_MODULE_AUTOTUNE))) {
			return SYSTEMALARMS_CONFIGERROR_AUTOTUNE;
		}

		/* Throw an error if LQG modes are configured without system identification data. */
		if (modes[i] == MANUALCONTROLSETTINGS_STABILIZATION1SETTINGS_LQG ||
			modes[i] == MANUALCONTROLSETTINGS_STABILIZATION1SETTINGS_ATTITUDELQG) {
			if (!lqg_sysident_check())
				return SYSTEMALARMS_CONFIGERROR_LQG;
		}
	}


	// Warning: This assumes that certain conditions in the XML file are met.  That 
	// MANUALCONTROLSETTINGS_STABILIZATION1SETTINGS_DISABLED has the same numeric value for each channel
	// and is the same for STABILIZATIONDESIRED_STABILIZATIONMODE_DISABLED

	return SYSTEMALARMS_CONFIGERROR_NONE;
}

/**
 * If the system is disarmed, look for a variety of conditions that
 * make it unsafe to arm (that might not be dangerous to engage once
 * flying).
 *
 * Note this does not check every possible situation that prevents
 * arming.  In particular, transmitter_control checks for failsafe and
 * ranges and switch arming configuration to allow/prevent arming.
 */
static int32_t check_safe_to_arm()
{
	FlightStatusData flightStatus;
	FlightStatusGet(&flightStatus);

	// Only arm in traditional modes where pilot has control
	if (flightStatus.Armed != FLIGHTSTATUS_ARMED_ARMED) {
		switch (flightStatus.FlightMode) {
			case FLIGHTSTATUS_FLIGHTMODE_MANUAL:
			case FLIGHTSTATUS_FLIGHTMODE_ACRO:
			case FLIGHTSTATUS_FLIGHTMODE_ACROPLUS:
			case FLIGHTSTATUS_FLIGHTMODE_ACRODYNE:
			case FLIGHTSTATUS_FLIGHTMODE_LEVELING:
			case FLIGHTSTATUS_FLIGHTMODE_HORIZON:
			case FLIGHTSTATUS_FLIGHTMODE_AXISLOCK:
			case FLIGHTSTATUS_FLIGHTMODE_VIRTUALBAR:
			case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
			case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
			case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
			case FLIGHTSTATUS_FLIGHTMODE_ALTITUDEHOLD:
			case FLIGHTSTATUS_FLIGHTMODE_FLIPREVERSED:
				break;
			case FLIGHTSTATUS_FLIGHTMODE_LQG:
			case FLIGHTSTATUS_FLIGHTMODE_LQGLEVELING:
				if (!lqg_sysident_check()) {
					return SYSTEMALARMS_CONFIGERROR_LQG;
				}
				break;

			case FLIGHTSTATUS_FLIGHTMODE_FAILSAFE:
				/* for failsafe, we don't want to prevent
				 * arming here because it makes an ugly looking
				 * GCS config error.
				 */
				break;
			default:
				// Any mode not specifically allowed prevents arming
				return SYSTEMALARMS_CONFIGERROR_UNSAFETOARM;
		}
	}

	return SYSTEMALARMS_CONFIGERROR_NONE;
}

/**
 * If an autonomous mode is available, make sure all the configurations are
 * valid for it
 */
static int32_t check_safe_autonomous()
{
	// The current filter combinations are safe for navigation
	//   Attitude   |  Navigation
	//     Comp     |     Raw          (not recommended)
	//     Comp     |     INS          (recommmended)
	//    Anything  |     None         (unsafe)
	//   INSOutdoor |     INS
	//   INSIndoor  |     INS          (unsafe)

	StateEstimationData stateEstimation;
	StateEstimationGet(&stateEstimation);

	if (stateEstimation.AttitudeFilter == STATEESTIMATION_ATTITUDEFILTER_INSINDOOR)
		return SYSTEMALARMS_CONFIGERROR_NAVFILTER;

	// Anything not allowed is invalid, safe default
	if (stateEstimation.NavigationFilter != STATEESTIMATION_NAVIGATIONFILTER_INS &&
		stateEstimation.NavigationFilter != STATEESTIMATION_NAVIGATIONFILTER_RAW)
		return SYSTEMALARMS_CONFIGERROR_NAVFILTER;

	return SYSTEMALARMS_CONFIGERROR_NONE;
}

/**
 * Check the rates achieved via stabilization are ones that can be tracked
 * by the gyros as configured.
 * @return error code if not
 */
static int32_t check_stabilization_rates()
{
	const float MAXIMUM_SAFE_FRACTIONAL_RATE = 0.85;
	int32_t max_safe_rate = PIOS_SENSORS_GetMaxGyro() * MAXIMUM_SAFE_FRACTIONAL_RATE;
	float rates[3];

	StabilizationSettingsManualRateGet(rates);
	if (rates[0] > max_safe_rate || rates[1] > max_safe_rate || rates[2] > max_safe_rate)
		return SYSTEMALARMS_CONFIGERROR_STABILIZATION;

	StabilizationSettingsMaximumRateGet(rates);
	if (rates[0] > max_safe_rate || rates[1] > max_safe_rate || rates[2] > max_safe_rate)
		return SYSTEMALARMS_CONFIGERROR_STABILIZATION;

	return SYSTEMALARMS_CONFIGERROR_NONE;
}

/**
 * Set the error code and alarm state
 * @param[in] error code
 */
void set_config_error(SystemAlarmsConfigErrorOptions error_code)
{
	// Get the severity of the alarm given the error code
	SystemAlarmsAlarmOptions severity;

	static bool sticky = false;

	/* Once a sticky error occurs, never change the error code */
	if (sticky) return;

	switch (error_code) {
	case SYSTEMALARMS_CONFIGERROR_NONE:
		severity = SYSTEMALARMS_ALARM_OK;
		break;
	default:
		error_code = SYSTEMALARMS_CONFIGERROR_UNDEFINED;
		/* and fall through */

	case SYSTEMALARMS_CONFIGERROR_DUPLICATEPORTCFG:
		sticky = true;
		/* and fall through */
	case SYSTEMALARMS_CONFIGERROR_AUTOTUNE:
	case SYSTEMALARMS_CONFIGERROR_ALTITUDEHOLD:
	case SYSTEMALARMS_CONFIGERROR_MULTIROTOR:
	case SYSTEMALARMS_CONFIGERROR_NAVFILTER:
	case SYSTEMALARMS_CONFIGERROR_PATHPLANNER:
	case SYSTEMALARMS_CONFIGERROR_POSITIONHOLD:
	case SYSTEMALARMS_CONFIGERROR_STABILIZATION:
	case SYSTEMALARMS_CONFIGERROR_UNDEFINED:
	case SYSTEMALARMS_CONFIGERROR_UNSAFETOARM:
	case SYSTEMALARMS_CONFIGERROR_LQG:
		severity = SYSTEMALARMS_ALARM_ERROR;
		break;
	}

	// Make sure not to set the error code if it didn't change
	SystemAlarmsConfigErrorOptions current_error_code;
	SystemAlarmsConfigErrorGet((uint8_t *) &current_error_code);
	if (current_error_code != error_code) {
		SystemAlarmsConfigErrorSet((uint8_t *) &error_code);
	}

	// AlarmSet checks only updates on toggle
	AlarmsSet(SYSTEMALARMS_ALARM_SYSTEMCONFIGURATION, (uint8_t) severity);
}

/**
 * @}
 */
