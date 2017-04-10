/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup ActuatorModule Actuator Module
 * @{
 *
 * @file       actuator.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Actuator module. Drives the actuators (servos, motors etc).
 * @brief      Take the values in @ref ActuatorDesired and mix to set the outputs
 *
 * This module ultimately controls the outputs.  The values from @ref ActuatorDesired
 * are combined based on the values in @ref MixerSettings and then scaled by the
 * values in @ref ActuatorSettings to create the output PWM times.
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

#include <math.h>

#include "openpilot.h"
#include "actuatorsettings.h"
#include "systemsettings.h"
#include "actuatordesired.h"
#include "actuatorcommand.h"
#include "flightstatus.h"
#include "mixersettings.h"
#include "cameradesired.h"
#include "manualcontrolcommand.h"
#include "pios_thread.h"
#include "pios_queue.h"
#include "misc_math.h"

// Private constants
#define MAX_QUEUE_SIZE 2

#if defined(PIOS_ACTUATOR_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_ACTUATOR_STACK_SIZE
#else
#define STACK_SIZE_BYTES 1336
#endif

#define TASK_PRIORITY PIOS_THREAD_PRIO_HIGHEST
#define FAILSAFE_TIMEOUT_MS 100

#ifndef MAX_MIX_ACTUATORS
#define MAX_MIX_ACTUATORS ACTUATORCOMMAND_CHANNEL_NUMELEM
#endif


DONT_BUILD_IF(ACTUATORSETTINGS_TIMERUPDATEFREQ_NUMELEM > PIOS_SERVO_MAX_BANKS, TooManyServoBanks);
DONT_BUILD_IF(MAX_MIX_ACTUATORS > ACTUATORCOMMAND_CHANNEL_NUMELEM, TooManyMixers);

#define MIXER_SCALE 128

// Private types

// Private variables
static struct pios_queue *queue;
static struct pios_thread *taskHandle;

static bool flightStatusUpdated = true;
static bool manualControlCommandUpdated = true;

// used to inform the actuator thread that actuator / mixer settings are updated
// set true to ensure they're fetched on first run
static volatile bool actuator_settings_updated = true;
static volatile bool mixer_settings_updated = true;

// The actual mixer settings data, pulled at the top of the actuator thread
static MixerSettingsData mixerSettings;

static float motor_mixer[MIXERSETTINGS_MIXER1VECTOR_NUMELEM * MAX_MIX_ACTUATORS];

// Ditto, for the actuator settings.
static ActuatorSettingsData actuatorSettings;

// Private functions
static void actuator_task(void* parameters);
static float scale_channel(float value, int idx);
static void set_failsafe();
static float throt_curve(const float input, const float* curve, uint8_t num_points);
static float collective_curve(const float input, const float* curve, uint8_t num_points);
static void actuator_set_servo_mode(void);

static MixerSettingsMixer1TypeOptions get_mixer_type(int idx);

/**
 * @brief Module initialization
 * @return 0
 */
int32_t ActuatorStart()
{
	// Watchdog must be registered before starting task
	PIOS_WDG_RegisterFlag(PIOS_WDG_ACTUATOR);

	// Start main task
	taskHandle = PIOS_Thread_Create(actuator_task, "Actuator", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
	TaskMonitorAdd(TASKINFO_RUNNING_ACTUATOR, taskHandle);

	return 0;
}

/**
 * @brief Module initialization
 * @return 0
 */
int32_t ActuatorInitialize()
{
	// Register for notification of changes to ActuatorSettings
	if (ActuatorSettingsInitialize()  == -1) {
		return -1;
	}
	ActuatorSettingsConnectCallbackCtx(UAVObjCbSetFlag, &actuator_settings_updated);

	// Register for notification of changes to MixerSettings
	if (MixerSettingsInitialize()  == -1) {
		return -1;
	}
	MixerSettingsConnectCallbackCtx(UAVObjCbSetFlag, &mixer_settings_updated);

	// Listen for ActuatorDesired updates (Primary input to this module)
	if (ActuatorDesiredInitialize()  == -1) {
		return -1;
	}

	queue = PIOS_Queue_Create(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));
	ActuatorDesiredConnectQueue(queue);

	// Primary output of this module
	if (ActuatorCommandInitialize() == -1) {
		return -1;
	}

#if defined(MIXERSTATUS_DIAGNOSTICS)
	// UAVO only used for inspecting the internal status of the mixer during debug
	if (MixerStatusInitialize()  == -1) {
		return -1;
	}
#endif

	return 0;
}
MODULE_HIPRI_INITCALL(ActuatorInitialize, ActuatorStart);

static float get_curve2_source(ActuatorDesiredData *desired, SystemSettingsAirframeTypeOptions airframe_type, MixerSettingsCurve2SourceOptions source)
{
	float tmp;

	switch (source) {
	case MIXERSETTINGS_CURVE2SOURCE_THROTTLE:
		if(airframe_type == SYSTEMSETTINGS_AIRFRAMETYPE_HELICP)
		{
			ManualControlCommandThrottleGet(&tmp);
			return tmp;
		}
		return desired->Thrust;
		break;
	case MIXERSETTINGS_CURVE2SOURCE_ROLL:
		return desired->Roll;
		break;
	case MIXERSETTINGS_CURVE2SOURCE_PITCH:
		return desired->Pitch;
		break;
	case MIXERSETTINGS_CURVE2SOURCE_YAW:
		return desired->Yaw;
		break;
	case MIXERSETTINGS_CURVE2SOURCE_COLLECTIVE:
		if (airframe_type == SYSTEMSETTINGS_AIRFRAMETYPE_HELICP)
		{
			return desired->Thrust;
		}
		ManualControlCommandCollectiveGet(&tmp);
		return tmp;
		break;
	case MIXERSETTINGS_CURVE2SOURCE_ACCESSORY0:
	case MIXERSETTINGS_CURVE2SOURCE_ACCESSORY1:
	case MIXERSETTINGS_CURVE2SOURCE_ACCESSORY2:
		(void) 0;

		int idx = source - MIXERSETTINGS_CURVE2SOURCE_ACCESSORY0;

		if (idx < 0) {
			return 0;
		}

		if (idx >= MANUALCONTROLCOMMAND_ACCESSORY_NUMELEM) {
			return 0;
		}

		float accessories[MANUALCONTROLCOMMAND_ACCESSORY_NUMELEM];

		ManualControlCommandAccessoryGet(accessories);

		return accessories[idx];
		break;
	}

	/* Can't get here */
	return 0;
}

static void compute_one_mixer(int i,
		int16_t (*vals)[MIXERSETTINGS_MIXER1VECTOR_NUMELEM])
{
	MixerSettingsMixer1TypeOptions type = get_mixer_type(i);

	i *= MIXERSETTINGS_MIXER1VECTOR_NUMELEM;

	if ((type != MIXERSETTINGS_MIXER1TYPE_SERVO) &&
			(type != MIXERSETTINGS_MIXER1TYPE_MOTOR)) {
		for (int j = 0; j < MIXERSETTINGS_MIXER1VECTOR_NUMELEM; j++) {
			// Ensure unused types are zero-filled
			motor_mixer[i+j] = 0;
		}
	}

	for (int j = 0; j < MIXERSETTINGS_MIXER1VECTOR_NUMELEM; j++) {
		motor_mixer[i+j] = (*vals)[j] * (1.0f / MIXER_SCALE);
	}
}

static void compute_mixer()
{
#if MAX_MIX_ACTUATORS > 0
	compute_one_mixer(0, &mixerSettings.Mixer1Vector);
#endif
#if MAX_MIX_ACTUATORS > 1
	compute_one_mixer(1, &mixerSettings.Mixer2Vector);
#endif
#if MAX_MIX_ACTUATORS > 2
	compute_one_mixer(2, &mixerSettings.Mixer3Vector);
#endif
#if MAX_MIX_ACTUATORS > 3
	compute_one_mixer(3, &mixerSettings.Mixer4Vector);
#endif
#if MAX_MIX_ACTUATORS > 4
	compute_one_mixer(4, &mixerSettings.Mixer5Vector);
#endif
#if MAX_MIX_ACTUATORS > 5
	compute_one_mixer(5, &mixerSettings.Mixer6Vector);
#endif
#if MAX_MIX_ACTUATORS > 6
	compute_one_mixer(6, &mixerSettings.Mixer7Vector);
#endif
#if MAX_MIX_ACTUATORS > 7
	compute_one_mixer(7, &mixerSettings.Mixer8Vector);
#endif
#if MAX_MIX_ACTUATORS > 8
	compute_one_mixer(8, &mixerSettings.Mixer9Vector);
#endif
#if MAX_MIX_ACTUATORS > 9
	compute_one_mixer(9, &mixerSettings.Mixer10Vector);
#endif
}

static void fill_command_vector(
		ActuatorDesiredData *desired,
		ManualControlCommandData *cmd,
		float curve1, float curve2,
		float (*cmd_vector)[MIXERSETTINGS_MIXER1VECTOR_NUMELEM])
{
	(*cmd_vector)[MIXERSETTINGS_MIXER1VECTOR_THROTTLECURVE1] = curve1;
	(*cmd_vector)[MIXERSETTINGS_MIXER1VECTOR_THROTTLECURVE2] = curve2;
	(*cmd_vector)[MIXERSETTINGS_MIXER1VECTOR_ROLL] = desired->Roll;
	(*cmd_vector)[MIXERSETTINGS_MIXER1VECTOR_PITCH] = desired->Pitch;
	(*cmd_vector)[MIXERSETTINGS_MIXER1VECTOR_YAW] = desired->Yaw;
	(*cmd_vector)[MIXERSETTINGS_MIXER1VECTOR_ACCESSORY0] = cmd->Accessory[0];
	(*cmd_vector)[MIXERSETTINGS_MIXER1VECTOR_ACCESSORY1] = cmd->Accessory[1];
	(*cmd_vector)[MIXERSETTINGS_MIXER1VECTOR_ACCESSORY2] = cmd->Accessory[2];
}

/**
 * @brief Main Actuator module task
 *
 * Universal matrix based mixer for VTOL, helis and fixed wing.
 * Converts desired roll,pitch,yaw and throttle to servo/ESC outputs.
 *
 * Because of how the Throttle ranges from 0 to 1, the motors should too!
 *
 * Note this code depends on the UAVObjects for the mixers being all being the same
 * and in sequence. If you change the object definition, make sure you check the code!
 *
 * @return -1 if error, 0 if success
 */
static void actuator_task(void* parameters)
{
	float dT = 0.0f;

	ActuatorCommandData command;
	ActuatorDesiredData desired;
	FlightStatusData flightStatus;
	ManualControlCommandData manual_cmd;

	SystemSettingsAirframeTypeOptions airframe_type;

	// Connect update callbacks
	FlightStatusConnectCallbackCtx(UAVObjCbSetFlag, &flightStatusUpdated);
	ManualControlCommandConnectCallbackCtx(UAVObjCbSetFlag, &manualControlCommandUpdated);

	// Main task loop
	uint32_t last_systime = PIOS_Thread_Systime();

	bool rc = false;

	while (1) {
		if (actuator_settings_updated) {
			actuator_settings_updated = false;
			ActuatorSettingsGet(&actuatorSettings);
			actuator_set_servo_mode();
		}

		if (mixer_settings_updated) {
			mixer_settings_updated = false;
			MixerSettingsGet(&mixerSettings);
			SystemSettingsAirframeTypeGet(&airframe_type);

			compute_mixer();
		}

		if (rc != true) {
			/* Update of ActuatorDesired timed out,
			 * or first iteration.  Go to failsafe */
			set_failsafe();
		}

		PIOS_WDG_UpdateFlag(PIOS_WDG_ACTUATOR);

		UAVObjEvent ev;

		// Wait until the ActuatorDesired object is updated
		rc = PIOS_Queue_Receive(queue, &ev, FAILSAFE_TIMEOUT_MS);

		/* If we timed out, go to top of loop, which sets failsafe
		 * and waits again. */
		if (rc != true) {
			continue;
		}

		// Check how long since last update
		uint32_t this_systime = PIOS_Thread_Systime();
		if (this_systime > last_systime) // reuse dt in case of wraparound
			dT = (this_systime - last_systime) / 1000.0f;
		last_systime = this_systime;

		ActuatorDesiredGet(&desired);
		ActuatorCommandGet(&command);

		if (flightStatusUpdated) {
			FlightStatusGet(&flightStatus);
			flightStatusUpdated = false;
		}

		if (manualControlCommandUpdated) {
			ManualControlCommandGet(&manual_cmd);
			manualControlCommandUpdated = false;
		}

		bool armed = flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED;
		bool spin_while_armed = actuatorSettings.MotorsSpinWhileArmed == ACTUATORSETTINGS_MOTORSSPINWHILEARMED_TRUE;

		float throttle_source = -1;
		if (airframe_type == SYSTEMSETTINGS_AIRFRAMETYPE_HELICP) {
			// Helis set throttle from manual control's throttle value,
			// unless in failsafe.
			if (flightStatus.FlightMode != FLIGHTSTATUS_FLIGHTMODE_FAILSAFE) {
				throttle_source = manual_cmd.Throttle;
			}
		} else {
			throttle_source = desired.Thrust;
		}

		bool stabilize_now = armed && (throttle_source > 0.0f);

		static uint32_t last_pos_throttle_time = 0;

		if (stabilize_now) {
			if (actuatorSettings.LowPowerStabilizationMaxTime) {
				last_pos_throttle_time = this_systime;
			}

			// Could consider stabilizing on a positive arming edge,
			// but this seems problematic.
		} else if (last_pos_throttle_time) {
			if ((this_systime - last_pos_throttle_time) <
					1000.0f * actuatorSettings.LowPowerStabilizationMaxTime) {
				stabilize_now = true;
				throttle_source = 0.0f;
			} else {
				last_pos_throttle_time = 0;
			}
		}

		float curve1 = throt_curve(throttle_source, mixerSettings.ThrottleCurve1, MIXERSETTINGS_THROTTLECURVE1_NUMELEM);

		//The source for the secondary curve is selectable
		float curve2 = collective_curve(
				get_curve2_source(&desired, airframe_type, mixerSettings.Curve2Source),
				mixerSettings.ThrottleCurve2,
				MIXERSETTINGS_THROTTLECURVE2_NUMELEM);

		float min_chan = INFINITY;
		float max_chan = -INFINITY;
		float neg_clip = 0;
		int num_motors = 0;

		float command_vect[MIXERSETTINGS_MIXER1VECTOR_NUMELEM];
		float motor_vect[MAX_MIX_ACTUATORS];

		fill_command_vector(&desired, &manual_cmd, curve1, curve2,
				&command_vect);

		matrix_mul_check(motor_mixer, command_vect, motor_vect,
				MAX_MIX_ACTUATORS,
				MIXERSETTINGS_MIXER1VECTOR_NUMELEM,
				1);

		for (int ct = 0; ct < MAX_MIX_ACTUATORS; ct++) {
			MixerSettingsMixer1TypeOptions type = get_mixer_type(ct);

			switch (type) {
				case MIXERSETTINGS_MIXER1TYPE_DISABLED:
					// Set to minimum if disabled.
					// This is not the same as saying
					// PWM pulse = 0 us
					motor_vect[ct] = -1;
					break;

				case MIXERSETTINGS_MIXER1TYPE_SERVO:
					break;

				case MIXERSETTINGS_MIXER1TYPE_MOTOR:
					min_chan = fminf(min_chan, motor_vect[ct]);
					max_chan = fmaxf(max_chan, motor_vect[ct]);

					if (motor_vect[ct] < 0.0f) {
						neg_clip += motor_vect[ct];
					}

					num_motors++;
					break;
				case MIXERSETTINGS_MIXER1TYPE_CAMERAPITCH:
					if (CameraDesiredHandle()) {
						CameraDesiredPitchGet(
								&motor_vect[ct]);
					} else {
						motor_vect[ct] = -1;
					}
					break;
				case MIXERSETTINGS_MIXER1TYPE_CAMERAROLL:
					if (CameraDesiredHandle()) {
						CameraDesiredRollGet(
								&motor_vect[ct]);
					} else {
						motor_vect[ct] = -1;
					}
					break;
				case MIXERSETTINGS_MIXER1TYPE_CAMERAYAW:
					if (CameraDesiredHandle()) {
						CameraDesiredRollGet(
								&command_vect[ct]);
					} else {
						motor_vect[ct] = -1;
					}
					break;
				default:
					set_failsafe();
					PIOS_Assert(0);
			}
		}

		float gain = 1.0f;
		float offset = 0.0f;

		/* This is a little dubious.  Scale down command ranges to
		 * fit.  It may cause some cross-axis coupling, though
		 * generally less than if we were to actually let it clip.
		 */
		if ((max_chan - min_chan) > 1.0f) {
			gain = 1.0f / (max_chan - min_chan);

			max_chan *= gain;
			min_chan *= gain;
		}

		/* Sacrifice throttle because of clipping */
		if (max_chan > 1.0f) {
			offset = 1.0f - max_chan;
		} else if (min_chan < 0.0f) {
			/* Low-side clip management-- how much power are we
			 * willing to add??? */

			neg_clip /= num_motors;

			/* neg_clip is now the amount of throttle "already added." by
			 * clipping...
			 *
			 * Find the "highest possible value" of offset.
			 * if neg_clip is -15%, and maxpoweradd is 10%, we need to add
			 * -5% to all motors.
			 * if neg_clip is 5%, and maxpoweradd is 10%, we can add up to
			 * 5% to all motors to further fix clipping.
			 */
			offset = neg_clip + actuatorSettings.LowPowerStabilizationMaxPowerAdd;

			/* Add the lesser of--
			 * A) the amount the lowest channel is out of range.
			 * B) the above calculated offset.
			 */
			offset = MIN(-min_chan, offset);
		}

		for (int ct = 0; ct < MAX_MIX_ACTUATORS; ct++) {
			// Motors have additional protection for when to be on
			if (get_mixer_type(ct) == MIXERSETTINGS_MIXER1TYPE_MOTOR) {
				if (!armed) {
					motor_vect[ct] = -1;  //force min throttle
				} else if (!stabilize_now) {
					if (!spin_while_armed) {
						motor_vect[ct] = -1;
					} else {
						motor_vect[ct] = 0;
					}
				} else {
					motor_vect[ct] = motor_vect[ct] * gain + offset;

					if (motor_vect[ct] > 0) {
						// Apply curve fitting, mapping the input to the propeller output.
						motor_vect[ct] = powapprox(motor_vect[ct], actuatorSettings.MotorInputOutputCurveFit);
					} else {
						motor_vect[ct] = 0;
					}
				}
			}

			command.Channel[ct] = scale_channel(motor_vect[ct], ct);
		}

		// Store update time
		command.UpdateTime = 1000.0f*dT;
		if (1000.0f*dT > command.MaxUpdateTime)
			command.MaxUpdateTime = 1000.0f*dT;

		// Update output object
		if (!ActuatorCommandReadOnly()) {
			ActuatorCommandSet(&command);
		} else {
			// it's read only during servo configuration--
			// so GCS takes precedence.
			ActuatorCommandGet(&command);
		}

		for (int n = 0; n < MAX_MIX_ACTUATORS; ++n) {
			PIOS_Servo_Set(n, command.Channel[n]);
		}

		PIOS_Servo_Update();

		AlarmsClear(SYSTEMALARMS_ALARM_ACTUATOR);
	}
}

/**
 * Interpolate a throttle curve
 *
 * throttle curve assumes input is [0,1]
 * this means that the throttle channel neutral value is nearly the same as its min value
 * this is convenient for throttle, since the neutral value is used as a failsafe and would thus shut off the motor
 *
 * @param input the input value, in [0,1]
 * @param curve the array of points in the curve
 * @param num_points the number of points in the curve
 * @return the output value, in [0,1]
 */
static float throt_curve(float const input, float const * curve, uint8_t num_points)
{
	return linear_interpolate(input, curve, num_points, 0.0f, 1.0f);
}

/**
 * Interpolate a collective curve
 *
 * we need to accept input in [-1,1] so that the neutral point may be set arbitrarily within the typical channel input range, which is [-1,1]
 *
 * @param input The input value, in [-1,1]
 * @param curve Array of points in the curve
 * @param num_points Number of points in the curve
 * @return the output value, in [-1,1]
 */
static float collective_curve(float const input, float const * curve, uint8_t num_points)
{
	return linear_interpolate(input, curve, num_points, -1.0f, 1.0f);
}

/**
 * Convert channel from -1/+1 to servo pulse duration in microseconds
 */
static float scale_channel(float value, int idx)
{
	float max = actuatorSettings.ChannelMax[idx];
	float min = actuatorSettings.ChannelMin[idx];
	float neutral = actuatorSettings.ChannelNeutral[idx];

	float valueScaled;
	// Scale
	if (value >= 0.0f) {
		valueScaled = value*(max-neutral) + neutral;
	} else {
		valueScaled = value*(neutral-min) + neutral;
	}

	if (max>min) {
		if (valueScaled > max) valueScaled = max;
		if (valueScaled < min) valueScaled = min;
	} else {
		if (valueScaled < max) valueScaled = max;
		if (valueScaled > min) valueScaled = min;
	}

	return valueScaled;
}

static float channel_failsafe_value(int idx)
{
	switch (get_mixer_type(idx)) {
	case MIXERSETTINGS_MIXER1TYPE_MOTOR:
		return actuatorSettings.ChannelMin[idx];
	case MIXERSETTINGS_MIXER1TYPE_SERVO:
		return actuatorSettings.ChannelNeutral[idx];
	default:
		// TODO: is this actually right/safe?
		return 0;
	}

}

/**
 * Set actuator output to the neutral values (failsafe)
 */
static void set_failsafe()
{
	float Channel[MAX_MIX_ACTUATORS];

	ActuatorCommandChannelGet(Channel);

	// Set alarm
	AlarmsSet(SYSTEMALARMS_ALARM_ACTUATOR, SYSTEMALARMS_ALARM_CRITICAL);

	// Update servo outputs
	for (int n = 0; n < ACTUATORCOMMAND_CHANNEL_NUMELEM; ++n) {
		float fs_val = channel_failsafe_value(n);

		PIOS_Servo_Set(n, fs_val);
	}
	
	PIOS_Servo_Update();

	// Update output object's parts that we changed
	ActuatorCommandChannelSet(Channel);
}

/**
 * @brief Update the servo update rate
 */
static void actuator_set_servo_mode(void)
{
	PIOS_Servo_SetMode(actuatorSettings.TimerUpdateFreq,
			ACTUATORSETTINGS_TIMERUPDATEFREQ_NUMELEM,
			actuatorSettings.ChannelMax,
			actuatorSettings.ChannelMin);
}

static MixerSettingsMixer1TypeOptions get_mixer_type(int idx)
{
	switch (idx) {
	case 0:
		return mixerSettings.Mixer1Type;
		break;
	case 1:
		return mixerSettings.Mixer2Type;
		break;
	case 2:
		return mixerSettings.Mixer3Type;
		break;
	case 3:
		return mixerSettings.Mixer4Type;
		break;
	case 4:
		return mixerSettings.Mixer5Type;
		break;
	case 5:
		return mixerSettings.Mixer6Type;
		break;
	case 6:
		return mixerSettings.Mixer7Type;
		break;
	case 7:
		return mixerSettings.Mixer8Type;
		break;
	case 8:
		return mixerSettings.Mixer9Type;
		break;
	case 9:
		return mixerSettings.Mixer10Type;
		break;
	default:
		// We can never get here unless there are mixer channels not handled in the above. Fail out.
		PIOS_Assert(0);
	}
}

/**
 * @}
 * @}
 */
