/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup Control Control Module
 * @{
 * @brief      Highest level control module which decides the control state
 *
 * This module users the values from the transmitter and its settings to
 * to determine if the system is currently controlled by failsafe, transmitter,
 * or a tablet.  The transmitter values are read out and stored in @ref
 * ManualControlCommand.  The tablet sends values via @ref TabletInfo which
 * may be used if the flight mode switch is in the appropriate position. The
 * transmitter settings come from @ref ManualControlSettings.
 *
 * @file       manualcontrol.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2016
 * @brief      ManualControl module. Handles safety R/C link and flight mode.
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

#include "openpilot.h"

#include "pios_thread.h"
#include "pios_rcvr.h"

#include "control.h"
#include "failsafe_control.h"
#include "tablet_control.h"
#include "transmitter_control.h"
#include "geofence_control.h"

#include "flightstatus.h"
#include "manualcontrolcommand.h"
#include "manualcontrolsettings.h"
#include "systemalarms.h"

// Private constants
#if defined(PIOS_MANUAL_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_MANUAL_STACK_SIZE
#else
#define STACK_SIZE_BYTES 1200
#endif

#define TASK_PRIORITY PIOS_THREAD_PRIO_HIGHEST
#define UPDATE_PERIOD_MS 20

// Private variables
static struct pios_thread *taskHandle;

// Private functions
static void manualControlTask(void *parameters);
static FlightStatusControlSourceOptions control_source_select();

bool vehicle_is_armed = false;

// This is exposed to transmitter_control
bool ok_to_arm(void);

/**
 * Module starting
 */
int32_t ManualControlStart()
{
	// Watchdog must be registered before starting task
	PIOS_WDG_RegisterFlag(PIOS_WDG_MANUAL);

	// Start main task
	taskHandle = PIOS_Thread_Create(manualControlTask, "Control", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
	TaskMonitorAdd(TASKINFO_RUNNING_MANUALCONTROL, taskHandle);

	return 0;
}

/**
 * Module initialization
 */
int32_t ManualControlInitialize()
{
	if (failsafe_control_initialize() == -1 \
		|| transmitter_control_initialize() == -1 \
		|| tablet_control_initialize() == -1 \
		|| geofence_control_initialize() == -1) {
	
		return -1;
	}

	return 0;
}

MODULE_HIPRI_INITCALL(ManualControlInitialize, ManualControlStart);

/**
 * Module task
 */
static void manualControlTask(void *parameters)
{
	/* Make sure disarmed on power up */
	FlightStatusData flightStatus;
	FlightStatusGet(&flightStatus);
	flightStatus.Armed = FLIGHTSTATUS_ARMED_DISARMED;
	FlightStatusSet(&flightStatus);

	// Select failsafe before run
	failsafe_control_select(true);

	uint32_t arm_time = 1000;

	enum arm_state {
		ARM_STATE_SAFETY,
		ARM_STATE_DISARMED,
		ARM_STATE_ARMING,
		ARM_STATE_HOLDING,
		ARM_STATE_ARMED,
	} arm_state = ARM_STATE_SAFETY;

	uint32_t arm_state_time = 0;

	while (1) {

		// Process periodic data for each of the controllers, including reading
		// all available inputs
		failsafe_control_update();
		transmitter_control_update();
		tablet_control_update();
		geofence_control_update();

		// Initialize to invalid value to ensure first update sets FlightStatus
		static FlightStatusControlSourceOptions last_control_selection = -1;

		// Control logic to select the valid controller
		FlightStatusControlSourceOptions control_selection =
			control_source_select();
		bool reset_controller = control_selection != last_control_selection;

		int ret = -1;

		enum control_status status = transmitter_control_get_status();

		// This logic would be better collapsed into control_source_select but
		// in the case of the tablet we need to be able to abort and fall back
		// to failsafe for now
		switch(control_selection) {
		case FLIGHTSTATUS_CONTROLSOURCE_TRANSMITTER:
			ret = transmitter_control_select(reset_controller);
			break;
		case FLIGHTSTATUS_CONTROLSOURCE_TABLET:
			ret = tablet_control_select(reset_controller);
			break;
		case FLIGHTSTATUS_CONTROLSOURCE_GEOFENCE:
			ret = geofence_control_select(reset_controller);
			break;
		default:
			/* Fall through into failsafe */
			break;
		}

		if (ret) {
			failsafe_control_select(last_control_selection !=
					FLIGHTSTATUS_CONTROLSOURCE_FAILSAFE);
			control_selection = FLIGHTSTATUS_CONTROLSOURCE_FAILSAFE;
		}

		if (control_selection != last_control_selection) {
			FlightStatusControlSourceSet(&control_selection);
			last_control_selection = control_selection;
		}

		uint32_t now = PIOS_Thread_Systime();
#define GO_STATE(x) \
				do { \
					arm_state = (x); \
					arm_state_time = (now); \
				} while (0)

		/* Global state transitions: weird stuff means disarmed +
		 * safety hold-down.
		 */
		if ((status == STATUS_ERROR) ||
				(status == STATUS_SAFETYTIMEOUT)) {
			GO_STATE(ARM_STATE_SAFETY);
		}

		/* If there's an alarm that prevents arming, enter a 0.2s
		 * no-arming-holddown.  This also stretches transient alarms
		 * and makes them more visible.
		 */
		if (!ok_to_arm() && (arm_state != ARM_STATE_ARMED)) {
			GO_STATE(ARM_STATE_SAFETY);
		}

		switch (arm_state) {
			case ARM_STATE_SAFETY:
				/* state_time > 0.2s + "NONE" or "DISARM" -> DISARMED
				 *
				 * !"NONE" and !"DISARM" -> SAFETY, reset state_time
				 */
				if ((status == STATUS_NONE) ||
						(status == STATUS_DISARM)) {
					if ((now - arm_state_time) > 200) {
						GO_STATE(ARM_STATE_DISARMED);
					}
				} else {
					GO_STATE(ARM_STATE_SAFETY);
				}

				break;
			case ARM_STATE_DISARMED:
				/* "ARM_*" -> ARMING
				 * "DISCONNECTED" -> SAFETY
				 */

				if ((status == STATUS_ARM_INVALID) ||
						(status == STATUS_ARM_VALID)) {
					GO_STATE(ARM_STATE_ARMING);
				} else if (status == STATUS_DISCONNECTED) {
					GO_STATE(ARM_STATE_SAFETY);
				}

				ManualControlSettingsArmTimeOptions arm_enum;
				ManualControlSettingsArmTimeGet(&arm_enum);

				arm_time = (arm_enum == MANUALCONTROLSETTINGS_ARMTIME_250) ? 250 :
					(arm_enum == MANUALCONTROLSETTINGS_ARMTIME_500) ? 500 :
					(arm_enum == MANUALCONTROLSETTINGS_ARMTIME_1000) ? 1000 :
					(arm_enum == MANUALCONTROLSETTINGS_ARMTIME_2000) ? 2000 : 1000;

				break;
			case ARM_STATE_ARMING:
				/* Anything !"ARM_*" -> SAFETY
				 *
				 * state_time > ArmTime + ARM_INVALID -> HOLDING
				 * state_time > ArmTime + ARM_VALID -> ARMED
				 */

				if ((status != STATUS_ARM_INVALID) &&
						(status != STATUS_ARM_VALID)) {
					GO_STATE(ARM_STATE_SAFETY);
				} else if ((now - arm_state_time) > arm_time) {
					if (status == STATUS_ARM_VALID) {
						GO_STATE(ARM_STATE_ARMED);
					} else {
						/* Must be STATUS_ARM_INVALID */
						GO_STATE(ARM_STATE_HOLDING);
					}
				}

				break;
			case ARM_STATE_HOLDING:
				/* ARM_VALID -> ARMED
				 * NONE -> ARMED
				 *
				 * ARM_INVALID -> Stay here
				 *
				 * Anything else -> SAFETY
				 */
				if ((status == STATUS_ARM_VALID) ||
						(status == STATUS_NONE)) {
					GO_STATE(ARM_STATE_ARMED);
				} else if (status == STATUS_ARM_INVALID) {
					/* TODO: could consider having a maximum
					 * time before we go to safety */
				} else {
					GO_STATE(ARM_STATE_SAFETY);
				}

				break;
			case ARM_STATE_ARMED:
				/* "DISARM" -> SAFETY (lower layer's job to check
				 * 	DisarmTime)
				 */
				if (status == STATUS_DISARM) {
					GO_STATE(ARM_STATE_SAFETY);
				}

				break;
		}

		FlightStatusArmedOptions armed, prev_armed;

		FlightStatusArmedGet(&prev_armed);

		switch (arm_state) {
			default:
			case ARM_STATE_SAFETY:
			case ARM_STATE_DISARMED:
				armed = FLIGHTSTATUS_ARMED_DISARMED;
				vehicle_is_armed = false;
				break;
			case ARM_STATE_ARMING:
				armed = FLIGHTSTATUS_ARMED_ARMING;
				break;
			case ARM_STATE_HOLDING:
				/* For now consider "HOLDING" an armed state,
				 * like old code does.  (Necessary to get the
				 * "initial spin while armed" that causes user
				 * to release arming position).
				 *
				 * TODO: do something different because
				 * control position invalid.
				 */
			case ARM_STATE_ARMED:
				armed = FLIGHTSTATUS_ARMED_ARMED;
				vehicle_is_armed = true;
				break;
		}

		if (armed != prev_armed) {
			FlightStatusArmedSet(&armed);
		}

		// Wait until next update
		PIOS_RCVR_WaitActivity(UPDATE_PERIOD_MS);
		PIOS_WDG_UpdateFlag(PIOS_WDG_MANUAL);
	}
}


/**
 * @brief control_source_select Determine which sub-module to use
 * for the main control source of the flight controller.
 * @returns @ref FlightStatusControlSourceOptions indicating the selected
 * mode
 *
 * This function is the ultimate one that controls what happens and
 * selects modes such as failsafe, transmitter control, geofencing
 * and potentially other high level modes in the future
 */
static FlightStatusControlSourceOptions control_source_select()
{
	// If the geofence controller is triggered, it takes precendence
	// over even transmitter or failsafe. This means this must be
	// EXTREMELY robust. The current behavior of geofence is simply
	// to shut off the motors.
	if (geofence_control_activate()) {
		return FLIGHTSTATUS_CONTROLSOURCE_GEOFENCE;
	}

	ManualControlCommandData cmd;
	ManualControlCommandGet(&cmd);
	if (cmd.Connected != MANUALCONTROLCOMMAND_CONNECTED_TRUE) {
		return FLIGHTSTATUS_CONTROLSOURCE_FAILSAFE;
	} else if (transmitter_control_get_flight_mode() ==
	           MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_TABLETCONTROL) {
		return FLIGHTSTATUS_CONTROLSOURCE_TABLET;
	} else {
		return FLIGHTSTATUS_CONTROLSOURCE_TRANSMITTER;
	}

}
/**
 * @brief Determine if the aircraft is safe to arm based on alarms
 * @returns True if safe to arm, false otherwise
 */
bool ok_to_arm(void)
{
	// read alarms
	SystemAlarmsData alarms;
	SystemAlarmsGet(&alarms);

	// Check each alarm
	for (int i = 0; i < SYSTEMALARMS_ALARM_NUMELEM; i++)
	{
		if (alarms.Alarm[i] >= SYSTEMALARMS_ALARM_ERROR &&
			i != SYSTEMALARMS_ALARM_GPS &&
			i != SYSTEMALARMS_ALARM_TELEMETRY)
		{
			return false;
		}
	}

	uint8_t flight_mode;
	FlightStatusFlightModeGet(&flight_mode);

	if (flight_mode == FLIGHTSTATUS_FLIGHTMODE_FAILSAFE) {
		/* Separately mask FAILSAFE arming here. */
		return false;
	}

	return true;
}

/**
  * @}
  * @}
  */
