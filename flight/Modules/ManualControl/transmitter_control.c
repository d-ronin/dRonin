/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{
 * @addtogroup Control Control Module
 * @{
 *
 * @file       transmitter_control.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2017
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Handles R/C link and flight mode.
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
#include "transmitter_control.h"
#include "pios_thread.h"

#include "altitudeholddesired.h"
#include "altitudeholdsettings.h"
#include "baroaltitude.h"
#include "flighttelemetrystats.h"
#include "flightstatus.h"
#include "loitercommand.h"
#include "pathdesired.h"
#include "positionactual.h"
#include "receiveractivity.h"
#include "systemsettings.h"

#include "misc_math.h"

#if defined(PIOS_INCLUDE_OPENLRS_RCVR)
#include "pios_openlrs.h"
#endif /* PIOS_INCLUDE_OPENLRS_RCVR */

#if defined(PIOS_INCLUDE_FRSKY_RSSI)
#include "pios_frsky_rssi.h"
#endif /* PIOS_INCLUDE_FRSKY_RSSI */

// This is how far "left" you have to deflect for "yaw left" arming, etc.
#define ARMED_THRESHOLD    0.50f

//safe band to allow a bit of calibration error or trim offset (in microseconds)
//these specify how far outside the "permitted range" throttle and other channels
//can go.  e.g. if range is 1000..2000us, values under 750us or over 2250us
//result in failsafe.
#define CONNECTION_OFFSET          250

#define RCVR_ACTIVITY_MONITOR_CHANNELS_PER_GROUP 12
#define RCVR_ACTIVITY_MONITOR_MIN_RANGE 20

// If the neutral point on the throttle channel is 35% over the configured
// minimum, we assume they're providing bidirectional thrust.
#define THRUST_BIDIR_THRESH 0.35f

/* All channels must have at least this many counts on each side of neutral.
 * (Except throttle which must have this many on the positive side).  This is
 * to prevent situations where a partial calibration results in spurious
 * arming, etc. */
#define MIN_MEANINGFUL_RANGE 40

struct rcvr_activity_fsm {
	ManualControlSettingsChannelGroupsOptions group;
	uint16_t prev[RCVR_ACTIVITY_MONITOR_CHANNELS_PER_GROUP];
	uint8_t check_count;
};

extern uintptr_t pios_rcvr_group_map[];

// Private variables
static ManualControlCommandData   cmd;
static ManualControlSettingsData  settings;

static uint8_t                    disconnected_count = 0;
static uint8_t                    connected_count = 0;
static struct rcvr_activity_fsm   activity_fsm;
static uint32_t                   lastActivityTime;
static uint32_t                   lastSysTime;
static float                      flight_mode_value;
static enum control_status        control_status;
static bool                       settings_updated;
static bool                       thrust_is_bidir;
static bool                       collective_is_thrust;

// Private functions
static float get_thrust_source(ManualControlCommandData *manual_control_command,
		bool always_fullrange);
static void update_stabilization_desired(ManualControlCommandData * manual_control_command, ManualControlSettingsData * settings);
static void altitude_hold_desired(ManualControlCommandData * cmd, bool flightModeChanged);
static void set_flight_mode();
static void process_transmitter_events(ManualControlCommandData * cmd, ManualControlSettingsData * settings, bool valid);
static void set_manual_control_error(SystemAlarmsManualControlOptions errorCode);
static float scaleChannel(int n, int16_t value);
static bool validInputRange(int n, uint16_t value, uint16_t offset);
static uint32_t timeDifferenceMs(uint32_t start_time, uint32_t end_time);
static void applyDeadband(float *value, float deadband);
static void resetRcvrActivity(struct rcvr_activity_fsm * fsm);
static bool updateRcvrActivity(struct rcvr_activity_fsm * fsm);
static void set_loiter_command(ManualControlCommandData *cmd);

// Exposed from manualcontrol to prevent attempts to arm when unsafe
extern bool ok_to_arm();

//! Convert a rssi type to the associated channel group.
int rssitype_to_channelgroup() {
	switch (settings.RssiType) {
		case MANUALCONTROLSETTINGS_RSSITYPE_PWM:
			return MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM;
		case MANUALCONTROLSETTINGS_RSSITYPE_PPM:
			return MANUALCONTROLSETTINGS_CHANNELGROUPS_PPM;
		case MANUALCONTROLSETTINGS_RSSITYPE_SBUS:
			return MANUALCONTROLSETTINGS_CHANNELGROUPS_SBUS;
		case MANUALCONTROLSETTINGS_RSSITYPE_HOTTSUM:
			return MANUALCONTROLSETTINGS_CHANNELGROUPS_HOTTSUM;
		case MANUALCONTROLSETTINGS_RSSITYPE_TBSCROSSFIRE:
			return MANUALCONTROLSETTINGS_CHANNELGROUPS_TBSCROSSFIRE;
		default:
			return -1;
	}
}

//! Initialize the transmitter control mode
int32_t transmitter_control_initialize()
{
	if (ManualControlCommandInitialize() == -1
			|| FlightStatusInitialize() == -1
			|| StabilizationDesiredInitialize() == -1
			|| ReceiverActivityInitialize() == -1
			|| LoiterCommandInitialize() == -1
			|| ManualControlSettingsInitialize() == -1) {
		return -1;
	}

	control_status = STATUS_ERROR;

	/* Initialize the RcvrActivty FSM */
	lastActivityTime = PIOS_Thread_Systime();
	resetRcvrActivity(&activity_fsm);

	// Use callback to update the settings when they change
	ManualControlSettingsConnectCallbackCtx(UAVObjCbSetFlag, &settings_updated);

	settings_updated = true;

	// Main task loop
	lastSysTime = PIOS_Thread_Systime();
	return 0;
}

static float get_thrust_source(ManualControlCommandData *manual_control_command,
		bool always_fullrange)
{
	float thrust = collective_is_thrust ?
			manual_control_command->Collective :
			manual_control_command->Throttle;

	if (thrust_is_bidir) {
		/* Thrust is bidirectional; apply a deadband and call it good */
		applyDeadband(&thrust, settings.ThrustDeadband);

		return thrust;
	}

	if (thrust < 0) {
		/* Thrust is not intended to be bidirectional, squish the
		 * bottom half of the range.
		 */
		thrust = 0;
	}

	if (!always_fullrange) {
		/* TODO/consider: use the value from 3D flight, and remove
		 * deadbands from downstream subsystems.  May want a
		 * different deadband, because e.g. helicopters may want no
		 * deadband except in althold etc.
		 */

		/* We have a 0..1 value, and they are OK with a 0..1 value */
		return thrust;
	}

	/* We want a -1..1 value, but thrust is squished into a 0..1 range. */
	return thrust * 2.0f - 1.0f;
}

static bool is_low_throttle_for_arming(
		ManualControlCommandData *manual_control_command)
{
	if (collective_is_thrust) {
		/* Always use throttle on vehicles with collective; in this
		 * case negative is OK for arming because it means we're below
		 * neutral (and presumably a vehicle with collective can't
		 * actually generate negative throttle).
		 */

		return manual_control_command->Throttle <= 0;
	}

	return get_thrust_source(manual_control_command, false) == 0;
}

static void perform_tc_settings_update()
{
	settings_updated = false;
	ManualControlSettingsGet(&settings);

	uint8_t thrust_channel;

	if (settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE]
			< MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE) {
		collective_is_thrust = true;
		thrust_channel = MANUALCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE;
	} else {
		collective_is_thrust = false;
		thrust_channel = MANUALCONTROLSETTINGS_CHANNELGROUPS_THROTTLE;
	}

	float minval  = settings.ChannelMin[thrust_channel];
	float maxval  = settings.ChannelMax[thrust_channel];
	float neutral = settings.ChannelNeutral[thrust_channel];

	float throt_span = (neutral - minval) / (maxval - minval);

	if (throt_span >= THRUST_BIDIR_THRESH) {
		thrust_is_bidir = true;
	} else {
		thrust_is_bidir = false;
	}
}

/**
  * @brief Process inputs and arming
  *
  * This will always process the arming signals as for now the transmitter
  * is always in charge.  When a transmitter is not detected control will
  * fall back to the failsafe module.  If the flight mode is in tablet
  * control position then control will be ceeded to that module.
  */
int32_t transmitter_control_update()
{
	lastSysTime = PIOS_Thread_Systime();

	float scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_NUMELEM];
	bool validChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_NUMELEM];

	if (settings_updated) {
		perform_tc_settings_update();

		/* TODO: Factor more of the valid-configuration checks up
		 * here, so they don't need to be performed each time.
		 * Also do additonal validation .. e.g. a mostly-negative
		 * throttle range doesn't make sense!
		 */
	}

	/* Update channel activity monitor */
	uint8_t arm_status;
	FlightStatusArmedGet(&arm_status);

	if (arm_status == FLIGHTSTATUS_ARMED_DISARMED) {
		if (updateRcvrActivity(&activity_fsm)) {
			/* Reset the aging timer because activity was detected */
			lastActivityTime = lastSysTime;
		}
	}

	if (timeDifferenceMs(lastActivityTime, lastSysTime) > 5000) {
		resetRcvrActivity(&activity_fsm);
		lastActivityTime = lastSysTime;
	}

	if (settings.RssiType != MANUALCONTROLSETTINGS_RSSITYPE_NONE) {
		int32_t value = 0;

		switch (settings.RssiType) {
		case MANUALCONTROLSETTINGS_RSSITYPE_ADC:
#if defined(PIOS_INCLUDE_ADC)
			if (settings.RssiChannelNumber > 0) {
				value = PIOS_ADC_GetChannelRaw(settings.RssiChannelNumber - 1);
			} else {
				value = 0;
			}
#endif
			break;
		case MANUALCONTROLSETTINGS_RSSITYPE_OPENLRS:
#if defined(PIOS_INCLUDE_OPENLRS_RCVR)
			value = PIOS_OpenLRS_RSSI_Get();
#endif /* PIOS_INCLUDE_OPENLRS_RCVR */
			break;
		case MANUALCONTROLSETTINGS_RSSITYPE_FRSKYPWM:
#if defined(PIOS_INCLUDE_FRSKY_RSSI)
			value = PIOS_FrSkyRssi_Get();
#endif /* PIOS_INCLUDE_FRSKY_RSSI */
			break;
		case MANUALCONTROLSETTINGS_RSSITYPE_RFM22B:
#if defined(PIOS_INCLUDE_RFM22B)
			value = PIOS_RFM22B_RSSI_Get();
#endif /* PIOS_INCLUDE_RFM22B */
			break;
		default:
			(void) 0 ;
			int mapped = rssitype_to_channelgroup();

			if (mapped >= 0) {
				value = PIOS_RCVR_Read(
						pios_rcvr_group_map[mapped],
						settings.RssiChannelNumber);
			}

			break;
		}

		if (value < settings.RssiMin) {
			cmd.Rssi = 0;
		} else if (settings.RssiMax == settings.RssiMin) {
			cmd.Rssi = 0;
		} else if (value > settings.RssiMax) {
			if (value > (settings.RssiMax + settings.RssiMax/4)) {
				cmd.Rssi = 0;
			} else {
				cmd.Rssi = 100;
			}
		} else {
			cmd.Rssi = ((float)(value - settings.RssiMin)/((float)settings.RssiMax-settings.RssiMin)) * 100;
		}

		cmd.RawRssi = value;
	}

	bool valid_input_detected = true;

	// Read channel values in us
	for (uint8_t n = 0;
	     n < MANUALCONTROLSETTINGS_CHANNELGROUPS_NUMELEM && n < MANUALCONTROLCOMMAND_CHANNEL_NUMELEM;
	     ++n) {

		if (settings.ChannelGroups[n] >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE) {
			cmd.Channel[n] = PIOS_RCVR_INVALID;
			validChannel[n] = false;
		} else {
			cmd.Channel[n] = PIOS_RCVR_Read(pios_rcvr_group_map[settings.ChannelGroups[n]],
							settings.ChannelNumber[n]);
		}

		// If a channel has timed out this is not valid data and we shouldn't update anything
		// until we decide to go to failsafe
		if(cmd.Channel[n] == (uint16_t) PIOS_RCVR_TIMEOUT) {
			valid_input_detected = false;
			validChannel[n] = false;
		} else {
			scaledChannel[n] = scaleChannel(n, cmd.Channel[n]);
			validChannel[n] = validInputRange(n, cmd.Channel[n], CONNECTION_OFFSET);
		}
	}

	// Check settings, if error raise alarm
	if (settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_ROLL] >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE ||
		settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_PITCH] >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE ||
		settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_YAW] >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE ||
		settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_THROTTLE] >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE ||
		// Check all channel mappings are valid
		cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ROLL] == (uint16_t) PIOS_RCVR_INVALID ||
		cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_PITCH] == (uint16_t) PIOS_RCVR_INVALID ||
		cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_YAW] == (uint16_t) PIOS_RCVR_INVALID ||
		cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_THROTTLE] == (uint16_t) PIOS_RCVR_INVALID ||
		// Check the driver exists
		cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ROLL] == (uint16_t) PIOS_RCVR_NODRIVER ||
		cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_PITCH] == (uint16_t) PIOS_RCVR_NODRIVER ||
		cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_YAW] == (uint16_t) PIOS_RCVR_NODRIVER ||
		cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_THROTTLE] == (uint16_t) PIOS_RCVR_NODRIVER ||
		// Check the FlightModeNumber is valid
		settings.FlightModeNumber < 1 || settings.FlightModeNumber > MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_NUMELEM ||
		// If we've got more than one possible valid FlightMode, we require a configured FlightMode channel
		((settings.FlightModeNumber > 1) && (settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_FLIGHTMODE] >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE)) ||
		// Whenever FlightMode channel is configured, it needs to be valid regardless of FlightModeNumber settings
		((settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_FLIGHTMODE] < MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE) && (
			cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_FLIGHTMODE] == (uint16_t) PIOS_RCVR_INVALID ||
			cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_FLIGHTMODE] == (uint16_t) PIOS_RCVR_NODRIVER ))) {

		set_manual_control_error(SYSTEMALARMS_MANUALCONTROL_SETTINGS);

		cmd.Connected = MANUALCONTROLCOMMAND_CONNECTED_FALSE;
		ManualControlCommandSet(&cmd);

		control_status = STATUS_ERROR;

		return -1;
	}

	// the block above validates the input configuration. this simply checks that the range is valid if flight mode is configured.
	bool flightmode_valid_input = settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_FLIGHTMODE] >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE ||
	    validChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_FLIGHTMODE];

	// because arming is optional we must determine if it is needed before checking it is valid
	bool arming_valid_input = (settings.Arming < MANUALCONTROLSETTINGS_ARMING_SWITCH) ||
		validChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ARMING];

	// decide if we have valid manual input or not
	valid_input_detected &= validChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_THROTTLE] &&
	    validChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ROLL] &&
	    validChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_YAW] &&
	    validChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_PITCH] &&
	    flightmode_valid_input &&
	    arming_valid_input;

	// Implement hysteresis loop on connection status
	if (valid_input_detected && (++connected_count > 10)) {
		cmd.Connected = MANUALCONTROLCOMMAND_CONNECTED_TRUE;
		connected_count = 0;
		disconnected_count = 0;
	} else if (!valid_input_detected && (++disconnected_count > 10)) {
		cmd.Connected = MANUALCONTROLCOMMAND_CONNECTED_FALSE;
		connected_count = 0;
		disconnected_count = 0;
	}

	if (cmd.Connected == MANUALCONTROLCOMMAND_CONNECTED_FALSE) {
		// These values are not used but just put ManualControlCommand in a sane state.  When
		// Connected is false, then the failsafe submodule will be in control.

		cmd.Throttle = 0;
		cmd.Roll = 0;
		cmd.Yaw = 0;
		cmd.Pitch = 0;
		cmd.Collective = 0;

		/* If all relevant channel groups aren't in error mode, then the receiver seems to be working fine.
		   So if we still reach this point, the channel min/max/neutral values on the input page are faulty. */
		bool rx_signal_detected = cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_THROTTLE] < (uint16_t)PIOS_RCVR_NODRIVER &&
			cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ROLL] < (uint16_t)PIOS_RCVR_NODRIVER &&
			cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_PITCH] < (uint16_t)PIOS_RCVR_NODRIVER &&
			cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_YAW] < (uint16_t)PIOS_RCVR_NODRIVER &&
			cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ARMING] < (uint16_t)PIOS_RCVR_NODRIVER &&
			cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_FLIGHTMODE] < (uint16_t)PIOS_RCVR_NODRIVER;

		if (rx_signal_detected)
			set_manual_control_error(SYSTEMALARMS_MANUALCONTROL_CHANNELCONFIGURATION);
		else
			set_manual_control_error(SYSTEMALARMS_MANUALCONTROL_NORX);

	} else if (valid_input_detected) {
		set_manual_control_error(SYSTEMALARMS_MANUALCONTROL_NONE);

		// Scale channels to -1 -> +1 range
		cmd.Roll           = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ROLL];
		cmd.Pitch          = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_PITCH];
		cmd.Yaw            = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_YAW];
		cmd.Throttle       = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_THROTTLE];
		cmd.ArmSwitch      = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ARMING] > 0 ?
		                     MANUALCONTROLCOMMAND_ARMSWITCH_ARMED : MANUALCONTROLCOMMAND_ARMSWITCH_DISARMED;
		flight_mode_value  = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_FLIGHTMODE];

		// Apply deadband for Roll/Pitch/Yaw stick inputs
		if (settings.Deadband) {
			applyDeadband(&cmd.Roll, settings.Deadband);
			applyDeadband(&cmd.Pitch, settings.Deadband);
			applyDeadband(&cmd.Yaw, settings.Deadband);
		}

		if(cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE] != (uint16_t) PIOS_RCVR_INVALID &&
		   cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE] != (uint16_t) PIOS_RCVR_NODRIVER &&
		   cmd.Channel[MANUALCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE] != (uint16_t) PIOS_RCVR_TIMEOUT) {
			cmd.Collective = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE];
		}

		// Set Accessory 0
		if (settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY0] !=
			MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE) {
			cmd.Accessory[0] = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY0];
		}

		// Set Accessory 1
		if (settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY1] !=
			MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE) {
			cmd.Accessory[1] = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY1];
		}
		// Set Accessory 2
		if (settings.ChannelGroups[MANUALCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY2] !=
			MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE) {
			cmd.Accessory[2] = scaledChannel[MANUALCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY2];
		}
	}

	// Process arming outside conditional so system will disarm when disconnected.  Notice this
	// is processed in the _update method instead of _select method so the state system is always
	// evalulated, even if not detected.
	process_transmitter_events(&cmd, &settings, valid_input_detected);

	// Update cmd object
	ManualControlCommandSet(&cmd);

	return 0;
}

/**
 * Select and use transmitter control
 * @param [in] reset_controller True if previously another controller was used
 */
int32_t transmitter_control_select(bool reset_controller)
{
	// Activate the flight mode corresponding to the switch position
	set_flight_mode();

	ManualControlCommandGet(&cmd);

	uint8_t flightMode;
	FlightStatusFlightModeGet(&flightMode);

	// Depending on the mode update the Stabilization object
	static uint8_t lastFlightMode = FLIGHTSTATUS_FLIGHTMODE_MANUAL;

	switch(flightMode) {
	case FLIGHTSTATUS_FLIGHTMODE_MANUAL:
	case FLIGHTSTATUS_FLIGHTMODE_ACRO:
	case FLIGHTSTATUS_FLIGHTMODE_ACROPLUS:
	case FLIGHTSTATUS_FLIGHTMODE_ACRODYNE:
	case FLIGHTSTATUS_FLIGHTMODE_LEVELING:
	case FLIGHTSTATUS_FLIGHTMODE_VIRTUALBAR:
	case FLIGHTSTATUS_FLIGHTMODE_HORIZON:
	case FLIGHTSTATUS_FLIGHTMODE_AXISLOCK:
	case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
	case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
	case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
	case FLIGHTSTATUS_FLIGHTMODE_FAILSAFE:
	case FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE:
		update_stabilization_desired(&cmd, &settings);
		break;
	case FLIGHTSTATUS_FLIGHTMODE_ALTITUDEHOLD:
		altitude_hold_desired(&cmd, lastFlightMode != flightMode);
		break;
	case FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD:
		set_loiter_command(&cmd);
		break;
	case FLIGHTSTATUS_FLIGHTMODE_RETURNTOHOME:
		// The path planner module processes data here
		break;
	case FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER:
		break;
	default:
		set_manual_control_error(SYSTEMALARMS_MANUALCONTROL_UNDEFINED);
		return -1;
	}
	lastFlightMode = flightMode;

	return 0;
}

//! Get the control & arming status
enum control_status transmitter_control_get_status()
{
	return control_status;
}

//! Determine which of N positions the flight mode switch is in but do not set it
uint8_t transmitter_control_get_flight_mode()
{
	// Convert flightMode value into the switch position in the range [0..N-1]
	uint8_t pos = ((int16_t)(flight_mode_value * 256.0f) + 256) * settings.FlightModeNumber >> 9;
	if (pos >= settings.FlightModeNumber)
		pos = settings.FlightModeNumber - 1;

	return settings.FlightModePosition[pos];
}

/**
 * Check sticks to determine if they are in the arming position
 */
static bool arming_position(ManualControlCommandData *cmd,
		ManualControlSettingsData *settings,
		bool low_throt) {
	// If system is not appropriate to arm, do not even attempt
	if (!ok_to_arm())
		return false;

	switch(settings->Arming) {
	case MANUALCONTROLSETTINGS_ARMING_ALWAYSDISARMED:
		return false;
	case MANUALCONTROLSETTINGS_ARMING_ROLLLEFTTHROTTLE:
		return low_throt && cmd->Roll < -ARMED_THRESHOLD;
	case MANUALCONTROLSETTINGS_ARMING_ROLLRIGHTTHROTTLE:
		return low_throt && cmd->Roll > ARMED_THRESHOLD;
	case MANUALCONTROLSETTINGS_ARMING_YAWLEFTTHROTTLE:
		return low_throt && cmd->Yaw < -ARMED_THRESHOLD;
	case MANUALCONTROLSETTINGS_ARMING_YAWRIGHTTHROTTLE:
		return low_throt && cmd->Yaw > ARMED_THRESHOLD;
	case MANUALCONTROLSETTINGS_ARMING_CORNERSTHROTTLE:
		return low_throt && (
			(cmd->Yaw > ARMED_THRESHOLD || cmd->Yaw < -ARMED_THRESHOLD) &&
			(cmd->Roll > ARMED_THRESHOLD || cmd->Roll < -ARMED_THRESHOLD) ) &&
			(cmd->Pitch > ARMED_THRESHOLD);
			// Note that pulling pitch stick down corresponds to positive values
	case MANUALCONTROLSETTINGS_ARMING_SWITCHTHROTTLE:
		if (!low_throt) return false;
	case MANUALCONTROLSETTINGS_ARMING_SWITCH:
		return cmd->ArmSwitch == MANUALCONTROLCOMMAND_ARMSWITCH_ARMED;
	default:
		return false;
	}
}

/**
 * Check sticks to determine if they are in the disarmed position
 */
static bool disarming_position(ManualControlCommandData * cmd, ManualControlSettingsData * settings, bool low_throt)
{
	switch(settings->Arming) {
	case MANUALCONTROLSETTINGS_ARMING_ALWAYSDISARMED:
		return true;
	case MANUALCONTROLSETTINGS_ARMING_ROLLLEFTTHROTTLE:
		return low_throt && cmd->Roll > ARMED_THRESHOLD;
	case MANUALCONTROLSETTINGS_ARMING_ROLLRIGHTTHROTTLE:
		return low_throt && cmd->Roll < -ARMED_THRESHOLD;
	case MANUALCONTROLSETTINGS_ARMING_YAWLEFTTHROTTLE:
		return low_throt && cmd->Yaw > ARMED_THRESHOLD;
	case MANUALCONTROLSETTINGS_ARMING_YAWRIGHTTHROTTLE:
		return low_throt && cmd->Yaw < -ARMED_THRESHOLD;
	case MANUALCONTROLSETTINGS_ARMING_CORNERSTHROTTLE:
		return low_throt && (
			(cmd->Yaw > ARMED_THRESHOLD || cmd->Yaw < -ARMED_THRESHOLD) &&
			(cmd->Roll > ARMED_THRESHOLD || cmd->Roll < -ARMED_THRESHOLD) );
	case MANUALCONTROLSETTINGS_ARMING_SWITCH:
	case MANUALCONTROLSETTINGS_ARMING_SWITCHTHROTTLE:
		return cmd->ArmSwitch != MANUALCONTROLCOMMAND_ARMSWITCH_ARMED;
	default:
		return false;
	}
}

static bool disarm_commanded(ManualControlCommandData *cmd,
		ManualControlSettingsData *settings,
		bool low_throt)
{
	static uint32_t start_time;

	if (!disarming_position(cmd, settings, low_throt)) {
		start_time = 0;
		return false;
	}

	uint32_t now = PIOS_Thread_Systime();

	if (!start_time) {
		start_time = now;
		return false;
	}

	uint16_t disarm_time = 100;

	if ((settings->Arming != MANUALCONTROLSETTINGS_ARMING_SWITCH) &&
			(settings->Arming != MANUALCONTROLSETTINGS_ARMING_SWITCHTHROTTLE)) {
		disarm_time = (settings->DisarmTime == MANUALCONTROLSETTINGS_DISARMTIME_250) ? 250 :
			(settings->DisarmTime == MANUALCONTROLSETTINGS_DISARMTIME_500) ? 500 :
			(settings->DisarmTime == MANUALCONTROLSETTINGS_DISARMTIME_1000) ? 1000 :
			(settings->DisarmTime == MANUALCONTROLSETTINGS_DISARMTIME_2000) ? 2000 : 1000;
	}

	return (now - start_time) >= disarm_time;
}

#define RECEIVER_TIMER_FIRED 0xffffffff

static uint32_t receiver_timer = RECEIVER_TIMER_FIRED;

static void reset_receiver_timer() {
	receiver_timer = 0;
}

static bool check_receiver_timer(uint32_t threshold) {
	if (!threshold) {
		/* 0 threshold means timer disabled */

		reset_receiver_timer();

		return false;
	}

	if (receiver_timer == RECEIVER_TIMER_FIRED) {
		/* It already went off!  We just want a strobe. */
		return false;
	}

	uint32_t now = PIOS_Thread_Systime();

	if (receiver_timer) {
		if ((now - receiver_timer) >= threshold) {
			receiver_timer = RECEIVER_TIMER_FIRED;
			return true;
		}

		return false;
	}

	receiver_timer = now;

	return false;
}

/**
 * @brief Process the inputs and determine whether to arm or not
 * @param[in] cmd The manual control inputs
 * @param[in] settings Settings indicating the necessary position
 * @param[in] scaled The scaled channels, used for checking arming
 * @param[in] valid If the input data is valid (i.e. transmitter is transmitting)
 */
static void process_transmitter_events(ManualControlCommandData * cmd, ManualControlSettingsData * settings, bool valid)
{
	valid &= cmd->Connected == MANUALCONTROLCOMMAND_CONNECTED_TRUE;

	if (!valid || (cmd->Connected != MANUALCONTROLCOMMAND_CONNECTED_TRUE)) {
		/* disarm timeout check-- use least favorable timeout.
		 * Note that if things were low throttle, and are now
		 * disconnected, we keep the old timer running.
		 *
		 * (The same is true for vice-versa).
		 */
		uint32_t timeout = 0;

		if (settings->InvalidRXArmedTimeout && settings->ArmedTimeout) {
			timeout = MIN(settings->InvalidRXArmedTimeout,
				settings->ArmedTimeout);
		} else if (settings->InvalidRXArmedTimeout) {
			timeout = settings->InvalidRXArmedTimeout;
		} else {
			timeout = settings->ArmedTimeout;
		}

		if (check_receiver_timer(timeout)) {
			control_status = STATUS_SAFETYTIMEOUT;
		} else {
			control_status = STATUS_DISCONNECTED;
		}

		return;
	}

	bool low_throt = is_low_throttle_for_arming(cmd);

	if (low_throt) {
		/* Determine whether to disarm when throttle is low */
		uint8_t flt_mode;
		FlightStatusFlightModeGet(&flt_mode);

		if (flt_mode == FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD ||
				flt_mode == FLIGHTSTATUS_FLIGHTMODE_RETURNTOHOME ||
				flt_mode == FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER  ||
				flt_mode == FLIGHTSTATUS_FLIGHTMODE_ALTITUDEHOLD ||
				flt_mode == FLIGHTSTATUS_FLIGHTMODE_TABLETCONTROL) {
			low_throt = false;
		}
	}

	if (low_throt) {
		if (check_receiver_timer(settings->ArmedTimeout)) {
			control_status = STATUS_DISARM;
			return;
		}
	} else {
		/* Not low throt, not disconnected. */
		reset_receiver_timer();
	}

	if (disarm_commanded(cmd, settings, low_throt)) {
		/* Separate timer from the above */
		control_status = STATUS_DISARM;
		return;
	} else if (arming_position(cmd, settings, low_throt)) {
		/* Not disarming pos, not low throt timeout, not disconnected. */

		if (control_status == STATUS_DISARM) {
			/* Edge detector says it's time to reset the receiver
			 * timer
			 */
			reset_receiver_timer();
		}

		if (settings->Arming == MANUALCONTROLSETTINGS_ARMING_SWITCH ||
				settings->Arming == MANUALCONTROLSETTINGS_ARMING_SWITCHTHROTTLE) {
			control_status = STATUS_ARM_VALID;
		} else {
			control_status = STATUS_ARM_INVALID;
		}

		return;
	} else {
		/* If the arming switch is flipped on, but we don't otherwise
		 * qualify for arming, let the upper layer know.
		 */
		if (settings->Arming == MANUALCONTROLSETTINGS_ARMING_SWITCHTHROTTLE) {
			if (cmd->ArmSwitch == MANUALCONTROLCOMMAND_ARMSWITCH_ARMED) {
				control_status = STATUS_INVALID_FOR_DISARMED;
				return;
			}
		}
	}

	control_status = STATUS_NORMAL;

	return;

}


//! Determine which of N positions the flight mode switch is in and set flight mode accordingly
static void set_flight_mode()
{
	uint8_t new_mode = transmitter_control_get_flight_mode();

	FlightStatusFlightModeOptions cur_mode;

	FlightStatusFlightModeGet(&cur_mode);

	if (cur_mode != new_mode) {
		FlightStatusFlightModeSet(&new_mode);
	}
}


static void resetRcvrActivity(struct rcvr_activity_fsm * fsm)
{
	ReceiverActivityData data;
	bool updated = false;

	/* Clear all channel activity flags */
	ReceiverActivityGet(&data);
	if (data.ActiveGroup != RECEIVERACTIVITY_ACTIVEGROUP_NONE &&
		data.ActiveChannel != 255) {
		data.ActiveGroup = RECEIVERACTIVITY_ACTIVEGROUP_NONE;
		data.ActiveChannel = 255;
		updated = true;
	}
	if (updated) {
		ReceiverActivitySet(&data);
	}

	/* Reset the FSM state */
	fsm->group        = 0;
	fsm->check_count = 0;
}

static void updateRcvrActivitySample(uintptr_t rcvr_id, uint16_t samples[], uint8_t max_channels) {
	for (uint8_t channel = 1; channel <= max_channels; channel++) {
		// Subtract 1 because channels are 1 indexed
		samples[channel - 1] = PIOS_RCVR_Read(rcvr_id, channel);
	}
}

static bool updateRcvrActivityCompare(uintptr_t rcvr_id, struct rcvr_activity_fsm * fsm)
{
	uint8_t active_channel = 0;

	int threshold = RCVR_ACTIVITY_MONITOR_MIN_RANGE;

	int rssi_channel = 0;

	/* Start off figuring out if there's a receive-rssi-injection channel */
	int rssi_group = rssitype_to_channelgroup();

	/* If so, see if it's what we're processing */
	if ((rssi_group >= 0) && (pios_rcvr_group_map[rssi_group] == rcvr_id)) {
		// Yup.  So let's skip the configured RSSI channel
		rssi_channel = settings.RssiChannelNumber;
	}

	/* Compare the current value to the previous sampled value */
	for (uint8_t channel = 1;
	     channel <= RCVR_ACTIVITY_MONITOR_CHANNELS_PER_GROUP;
	     channel++) {
		if (rssi_channel == channel) {
			// Don't spot activity on the configured RSSI channel
			continue;
		}

		uint16_t delta;
		uint16_t prev = fsm->prev[channel - 1];   // Subtract 1 because channels are 1 indexed
		uint16_t curr = PIOS_RCVR_Read(rcvr_id, channel);
		if (curr > prev) {
			delta = curr - prev;
		} else {
			delta = prev - curr;
		}

		if (delta > threshold) {
			active_channel = channel;

			// This is the best channel so far.  Only look for
			// "more active" channels now.
			threshold = delta;
		}
	}

	if (active_channel) {
		/* Mark this channel as active */
		ReceiverActivityActiveGroupSet((uint8_t *)&fsm->group);
		ReceiverActivityActiveChannelSet(&active_channel);

		return true;
	}

	return false;
}

static bool updateRcvrActivity(struct rcvr_activity_fsm * fsm)
{
	bool activity_updated = false;

	if (fsm->group >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE) {
		/* We're out of range, reset things */
		resetRcvrActivity(fsm);
	}

	if (fsm->check_count && (pios_rcvr_group_map[fsm->group])) {
		/* Compare with previous sample */
		activity_updated = updateRcvrActivityCompare(pios_rcvr_group_map[fsm->group], fsm);
	}

	if (!activity_updated && fsm->check_count < 3) {
		// First time through this is 1, so basically be willing
		// to check twice.
		fsm->check_count++;

		return false;
	}

	/* Reset the sample counter */
	fsm->check_count = 0;

	/* Find the next active group, but limit search so we can't loop forever here */
	for (uint8_t i = 0; i <= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE; i++) {
		/* Move to the next group */
		fsm->group++;
		if (fsm->group >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE) {
			/* Wrap back to the first group */
			fsm->group = 0;
		}
		if (pios_rcvr_group_map[fsm->group]) {
			/*
			 * Found an active group, take a sample here to avoid an
			 * extra 20ms delay in the main thread so we can speed up
			 * this algorithm.
			 */
			updateRcvrActivitySample(pios_rcvr_group_map[fsm->group],
						fsm->prev,
						NELEMENTS(fsm->prev));

			fsm->check_count = 1;

			break;
		}
	}

	return (activity_updated);
}

static inline float scale_stabilization(StabilizationSettingsData *stabSettings,
		float cmd, SharedDefsStabilizationModeOptions mode,
		StabilizationDesiredStabilizationModeElem axis) {
	switch (mode) {
		case SHAREDDEFS_STABILIZATIONMODE_DISABLED:
		case SHAREDDEFS_STABILIZATIONMODE_FAILSAFE:
		case SHAREDDEFS_STABILIZATIONMODE_POI:
			return 0;
		case SHAREDDEFS_STABILIZATIONMODE_MANUAL:
		case SHAREDDEFS_STABILIZATIONMODE_VIRTUALBAR:
		case SHAREDDEFS_STABILIZATIONMODE_COORDINATEDFLIGHT:
			return cmd;
		case SHAREDDEFS_STABILIZATIONMODE_SYSTEMIDENTRATE:
		case SHAREDDEFS_STABILIZATIONMODE_RATE:
		case SHAREDDEFS_STABILIZATIONMODE_WEAKLEVELING:
		case SHAREDDEFS_STABILIZATIONMODE_AXISLOCK:
			cmd = expoM(cmd, stabSettings->RateExpo[axis],
					stabSettings->RateExponent[axis]*0.1f);
			return cmd * stabSettings->ManualRate[axis];
		case SHAREDDEFS_STABILIZATIONMODE_ACRODYNE:
			// For acrodyne, pass the command through raw.
			return cmd;
		case SHAREDDEFS_STABILIZATIONMODE_ACROPLUS:
			cmd = expoM(cmd, stabSettings->RateExpo[axis],
					stabSettings->RateExponent[axis]*0.1f);
			return cmd;
		case SHAREDDEFS_STABILIZATIONMODE_SYSTEMIDENT:
		case SHAREDDEFS_STABILIZATIONMODE_ATTITUDE:
			return cmd * stabSettings->MaxLevelAngle[axis];
		case SHAREDDEFS_STABILIZATIONMODE_HORIZON:
			cmd = expo3(cmd, stabSettings->HorizonExpo[axis]);
			return cmd;
	}

	PIOS_Assert(false);
	return 0;
}

//! In stabilization mode, set stabilization desired
static void update_stabilization_desired(ManualControlCommandData * manual_control_command, ManualControlSettingsData * settings)
{
	StabilizationDesiredData stabilization;
	StabilizationDesiredGet(&stabilization);

	StabilizationSettingsData stabSettings;
	StabilizationSettingsGet(&stabSettings);

	const uint8_t MANUAL_SETTINGS[3] = {
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_MANUAL,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_MANUAL,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_MANUAL };
	const uint8_t RATE_SETTINGS[3] = {  STABILIZATIONDESIRED_STABILIZATIONMODE_RATE,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_RATE,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK};
	const uint8_t ATTITUDE_SETTINGS[3] = {
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK};
	const uint8_t VIRTUALBAR_SETTINGS[3] = {
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_VIRTUALBAR,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_VIRTUALBAR,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK};
	const uint8_t HORIZON_SETTINGS[3] = {
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_HORIZON,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_HORIZON,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK};
	const uint8_t AXISLOCK_SETTINGS[3] = {
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK,
	                                    STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK};
	const uint8_t ACROPLUS_SETTINGS[3] = {  STABILIZATIONDESIRED_STABILIZATIONMODE_ACROPLUS,
                                          STABILIZATIONDESIRED_STABILIZATIONMODE_ACROPLUS,
                                          STABILIZATIONDESIRED_STABILIZATIONMODE_RATE};

	const uint8_t ACRODYNE_SETTINGS[3] = {  STABILIZATIONDESIRED_STABILIZATIONMODE_ACRODYNE,
                                          STABILIZATIONDESIRED_STABILIZATIONMODE_ACRODYNE,
                                          STABILIZATIONDESIRED_STABILIZATIONMODE_ACRODYNE};

	const uint8_t FAILSAFE_SETTINGS[3] = {  STABILIZATIONDESIRED_STABILIZATIONMODE_FAILSAFE,
                                          STABILIZATIONDESIRED_STABILIZATIONMODE_FAILSAFE,
                                          STABILIZATIONDESIRED_STABILIZATIONMODE_FAILSAFE};

	const uint8_t SYSTEMIDENT_SETTINGS[3] = {  STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT,
                                          STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT,
                                          STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENTRATE };
	const uint8_t * stab_modes;

	uint8_t reprojection = STABILIZATIONDESIRED_REPROJECTIONMODE_NONE;

	uint8_t flightMode;

	FlightStatusFlightModeGet(&flightMode);
	switch(flightMode) {
		case FLIGHTSTATUS_FLIGHTMODE_MANUAL:
			stab_modes = MANUAL_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_ACRO:
			stab_modes = RATE_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_ACRODYNE:
			stab_modes = ACRODYNE_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_ACROPLUS:
			stab_modes = ACROPLUS_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_LEVELING:
			stab_modes = ATTITUDE_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_VIRTUALBAR:
			stab_modes = VIRTUALBAR_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_HORIZON:
			stab_modes = HORIZON_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_AXISLOCK:
			stab_modes = AXISLOCK_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_FAILSAFE:
			stab_modes = FAILSAFE_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_AUTOTUNE:
			stab_modes = SYSTEMIDENT_SETTINGS;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
			stab_modes = settings->Stabilization1Settings;
			reprojection = settings->Stabilization1Reprojection;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
			stab_modes = settings->Stabilization2Settings;
			reprojection = settings->Stabilization2Reprojection;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
			stab_modes = settings->Stabilization3Settings;
			reprojection = settings->Stabilization3Reprojection;
			break;
		default:
			{
				// Major error, this should not occur because only enter this block when one of these is true
				set_manual_control_error(SYSTEMALARMS_MANUALCONTROL_UNDEFINED);
			}
			return;
	}

	stabilization.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL]  = stab_modes[0];
	stabilization.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = stab_modes[1];
	stabilization.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW]   = stab_modes[2];

	stabilization.Roll = scale_stabilization(&stabSettings,
			manual_control_command->Roll,
			stab_modes[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL],
			STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL);

	stabilization.Pitch = scale_stabilization(&stabSettings,
			manual_control_command->Pitch,
			stab_modes[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH],
			STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH);

	stabilization.Yaw = scale_stabilization(&stabSettings,
			manual_control_command->Yaw,
			stab_modes[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW],
			STABILIZATIONDESIRED_STABILIZATIONMODE_YAW);

	// get thrust source; normalize_positive = false since we just want to pass the value through
	stabilization.Thrust = get_thrust_source(manual_control_command, false);

	// Set the reprojection.
	stabilization.ReprojectionMode = reprojection;

	StabilizationDesiredSet(&stabilization);
}

/**
 * @brief Update the altitude desired to current altitude when
 * enabled and enable altitude mode for stabilization
 */
static void altitude_hold_desired(ManualControlCommandData * cmd,
		bool flightModeChanged)
{
	if (AltitudeHoldDesiredHandle() == NULL) {
		set_manual_control_error(SYSTEMALARMS_MANUALCONTROL_ALTITUDEHOLD);
		return;
	}

	const float MIN_CLIMB_RATE = 0.01f;

	AltitudeHoldDesiredData altitudeHoldDesired;
	AltitudeHoldDesiredGet(&altitudeHoldDesired);

	StabilizationSettingsData stabSettings;
	StabilizationSettingsGet(&stabSettings);

	altitudeHoldDesired.Roll = cmd->Roll * stabSettings.RollMax;
	altitudeHoldDesired.Pitch = cmd->Pitch * stabSettings.PitchMax;
	altitudeHoldDesired.Yaw = cmd->Yaw * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_YAW];

	float current_down;
	PositionActualDownGet(&current_down);

	if(flightModeChanged) {
		// Initialize at the current location. Note that this object uses the up is positive
		// convention.
		altitudeHoldDesired.Altitude = -current_down;
		altitudeHoldDesired.ClimbRate = 0;
	} else {
		uint8_t altitude_hold_expo;
		uint8_t altitude_hold_maxclimbrate10;
		uint8_t altitude_hold_maxdescentrate10;
		uint8_t altitude_hold_deadband;
		AltitudeHoldSettingsMaxClimbRateGet(&altitude_hold_maxclimbrate10);
		AltitudeHoldSettingsMaxDescentRateGet(&altitude_hold_maxdescentrate10);

		// Scale altitude hold rate
		float altitude_hold_maxclimbrate = altitude_hold_maxclimbrate10 * 0.1f;
		float altitude_hold_maxdescentrate = altitude_hold_maxdescentrate10 * 0.1f;

		AltitudeHoldSettingsExpoGet(&altitude_hold_expo);
		AltitudeHoldSettingsDeadbandGet(&altitude_hold_deadband);

		const float DEADBAND = altitude_hold_deadband * 0.01f;

		float const thrust_source = get_thrust_source(cmd, true);

		float climb_rate = 0.0f;

		if (thrust_source > DEADBAND) {
			climb_rate = expo3((thrust_source - DEADBAND) / (1.0f - DEADBAND), altitude_hold_expo) *
		                         altitude_hold_maxclimbrate;
		} else if (thrust_source < -DEADBAND && altitude_hold_maxdescentrate > MIN_CLIMB_RATE) {
			climb_rate = expo3((thrust_source + DEADBAND) / (1.0f - DEADBAND), altitude_hold_expo) *
		                         altitude_hold_maxdescentrate;
		}

		// If more than MIN_CLIMB_RATE enter vario mode
		if (fabsf(climb_rate) > MIN_CLIMB_RATE) {
			// Desired state is at the current location with the requested rate
			altitudeHoldDesired.Altitude = -current_down;
			altitudeHoldDesired.ClimbRate = climb_rate;
		} else {
			// Here we intentionally do not change the set point, it will
			// remain where the user released vario mode
			altitudeHoldDesired.ClimbRate = 0.0f;
		}
	}

	// Must always set since this contains the control signals
	AltitudeHoldDesiredSet(&altitudeHoldDesired);
}


static void set_loiter_command(ManualControlCommandData *cmd)
{
	LoiterCommandData loiterCommand;

	loiterCommand.Pitch = cmd->Pitch;
	loiterCommand.Roll = cmd->Roll;

	loiterCommand.Thrust = get_thrust_source(cmd, true);

	loiterCommand.Frame = LOITERCOMMAND_FRAME_BODY;

	LoiterCommandSet(&loiterCommand);
}

/**
 * Convert channel from servo pulse duration (microseconds) to scaled -1/+1 range.
 */
static float scaleChannel(int n, int16_t value)
{
	// TODO : change these variable names to be safer
	int16_t min = settings.ChannelMin[n];
	int16_t max = settings.ChannelMax[n];
	int16_t neutral = settings.ChannelNeutral[n];

	float valueScaled;

	// Scale
	if ((max > min && value >= neutral) || (min > max && value <= neutral))
	{
		if (max != neutral)
			valueScaled = (float)(value - neutral) / (float)(max - neutral);
		else
			valueScaled = 0;
	}
	else
	{
		if (min != neutral)
			valueScaled = (float)(value - neutral) / (float)(neutral - min);
		else
			valueScaled = 0;
	}

	// Bound
	if (valueScaled >  1.0f) valueScaled =  1.0f;
	else
	if (valueScaled < -1.0f) valueScaled = -1.0f;

	return valueScaled;
}

static uint32_t timeDifferenceMs(uint32_t start_time, uint32_t end_time) {
	if (end_time >= start_time)
		return end_time - start_time;
	else
		return (uint32_t)PIOS_THREAD_TIMEOUT_MAX - start_time + end_time + 1;
}

/**
 * @brief Determine if the manual input value is within acceptable limits
 * @returns return TRUE if so, otherwise return FALSE
 */
bool validInputRange(int n, uint16_t value, uint16_t offset)
{
	int16_t min = settings.ChannelMin[n];
	int16_t max = settings.ChannelMax[n];
	int16_t neutral = settings.ChannelNeutral[n];
	int16_t range;
	bool inverted = false;

	if (min > max) {
		int16_t tmp = min;
		min = max;
		max = tmp;

		inverted = true;
	}

	if ((neutral > max) || (neutral < min)) {
		return false;
	}

	if (n == MANUALCONTROLSETTINGS_CHANNELGROUPS_THROTTLE) {
		/* Pick just the one that results in the "positive" side of
		 * the throttle scale */
		if (inverted) {
			range = neutral - min;
		} else {
			range = max - neutral;
		}
	} else {
		range = MIN(max - neutral, neutral - min);
	}

	if (range < MIN_MEANINGFUL_RANGE) {
		return false;
	}

	return (value >= (min - offset) && value <= (max + offset));
}

/**
 * @brief Apply deadband to Roll/Pitch/Yaw channels
 */
static void applyDeadband(float *value, float deadband)
{
	if (deadband < 0.0001f) return; /* ignore tiny deadband value */
	if (deadband >= 0.85f) return;	/* reject nonsensical db values */

	if (fabsf(*value) < deadband) {
		*value = 0.0f;
	} else {
		if (*value > 0.0f) {
			*value -= deadband;
		} else {
			*value += deadband;
		}

		*value /= (1 - deadband);
	}
}

/**
 * Set the error code and alarm state
 * @param[in] error code
 */
static void set_manual_control_error(SystemAlarmsManualControlOptions error_code)
{
	// Get the severity of the alarm given the error code
	SystemAlarmsAlarmOptions severity;
	switch (error_code) {
	case SYSTEMALARMS_MANUALCONTROL_NONE:
		severity = SYSTEMALARMS_ALARM_OK;
		break;
	case SYSTEMALARMS_MANUALCONTROL_NORX:
	case SYSTEMALARMS_MANUALCONTROL_CHANNELCONFIGURATION:
	case SYSTEMALARMS_MANUALCONTROL_ACCESSORY:
		severity = SYSTEMALARMS_ALARM_WARNING;
		break;
	case SYSTEMALARMS_MANUALCONTROL_SETTINGS:
		severity = SYSTEMALARMS_ALARM_CRITICAL;
		break;
	case SYSTEMALARMS_MANUALCONTROL_ALTITUDEHOLD:
		severity = SYSTEMALARMS_ALARM_ERROR;
		break;
	case SYSTEMALARMS_MANUALCONTROL_UNDEFINED:
	default:
		severity = SYSTEMALARMS_ALARM_CRITICAL;
		error_code = SYSTEMALARMS_MANUALCONTROL_UNDEFINED;
	}

	// Make sure not to set the error code if it didn't change
	SystemAlarmsManualControlOptions current_error_code;
	SystemAlarmsManualControlGet((uint8_t *) &current_error_code);
	if (current_error_code != error_code) {
		SystemAlarmsManualControlSet((uint8_t *) &error_code);
	}

	// AlarmSet checks only updates on toggle
	AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, (uint8_t) severity);
}

/**
  * @}
  * @}
  */
