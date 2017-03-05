/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup StabilizationModule Stabilization Module
 * @{
 *
 * @file       stabilization.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Control the UAV attitude to @ref StabilizationDesired
 *
 * The main control code which keeps the UAV at the attitude requested by
 * @ref StabilizationDesired.  This is done by comparing against
 * @ref AttitudeActual to compute the error in roll pitch and yaw then
 * then based on the mode and values in @ref StabilizationSettings computing
 * the desired outputs and placing them in @ref ActuatorDesired.
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
#include "stabilization.h"
#include "pios_thread.h"
#include "pios_queue.h"

#include "accels.h"
#include "actuatordesired.h"
#include "attitudeactual.h"
#include "cameradesired.h"
#include "flightstatus.h"
#include "gyros.h"
#include "ratedesired.h"
#include "systemident.h"
#include "stabilizationdesired.h"
#include "stabilizationsettings.h"
#include "subtrim.h"
#include "subtrimsettings.h"
#include "systemsettings.h"
#include "manualcontrolcommand.h"
#include "vbarsettings.h"

// Math libraries
#include "coordinate_conversions.h"
#include "physical_constants.h"
#include "pid.h"
#include "misc_math.h"

// Includes for various stabilization algorithms
#include "virtualflybar.h"

// Private constants
#define MAX_QUEUE_SIZE 1

#if defined(PIOS_STABILIZATION_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_STABILIZATION_STACK_SIZE
#else
#define STACK_SIZE_BYTES 860
#endif

#define TASK_PRIORITY PIOS_THREAD_PRIO_HIGHEST
#define FAILSAFE_TIMEOUT_MS 30
#define COORDINATED_FLIGHT_MIN_ROLL_THRESHOLD 3.0f
#define COORDINATED_FLIGHT_MAX_YAW_THRESHOLD 0.05f

//! Set the stick position that maximally transitions to rate
// This value is also used for the expo plot in GCS (config/stabilization/advanced).
// Please adapt changes here also to the init of the plots  in /ground/gcs/src/plugins/config/configstabilizationwidget.cpp
#define HORIZON_MODE_MAX_BLEND               0.85f

enum {
	PID_GROUP_RATE,   // Rate controller settings
	PID_RATE_ROLL = PID_GROUP_RATE,
	PID_RATE_PITCH,
	PID_RATE_YAW,
	PID_GROUP_ATT,    // Attitude controller settings
	PID_ATT_ROLL = PID_GROUP_ATT,
	PID_ATT_PITCH,
	PID_ATT_YAW,
	PID_GROUP_VBAR,   // Virtual flybar settings
	PID_VBAR_ROLL = PID_GROUP_VBAR,
	PID_VBAR_PITCH,
	PID_VBAR_YAW,
	PID_COORDINATED_FLIGHT_YAW,
	PID_MAX
};

// Private variables
static struct pios_thread *taskHandle;
static StabilizationSettingsData settings;
static VbarSettingsData vbar_settings;

static SubTrimData subTrim;
static struct pios_queue *queue;

uint16_t ident_wiggle_points;

float axis_lock_accum[3] = {0,0,0};
uint8_t max_axis_lock = 0;
uint8_t max_axislock_rate = 0;
float weak_leveling_kp = 0;
uint8_t weak_leveling_max = 0;
bool lowThrottleZeroIntegral;
float vbar_decay = 0.991f;
float gyro_alpha = 0.6f;
static float max_rate_alpha = 0.8f;

struct pid pids[PID_MAX];

#ifndef NO_CONTROL_DEADBANDS
struct pid_deadband *deadbands = NULL;
#endif

volatile bool gyro_filter_updated = true; 

static bool actuatorDesiredUpdated = true;
static bool flightStatusUpdated = true;
static bool systemSettingsUpdated = true;

// Private functions
static void stabilizationTask(void* parameters);
static void zero_pids(void);
static void calculate_pids(void);
static void SettingsUpdatedCb(UAVObjEvent * objEv, void *ctx, void *obj, int len);
static float get_throttle(StabilizationDesiredData *stabilization_desired, SystemSettingsAirframeTypeOptions *airframe_type);

#ifndef NO_CONTROL_DEADBANDS
#define get_deadband(axis) (deadbands ? (deadbands + axis) : NULL)
#else
#define get_deadband(axis) NULL
#endif

static float get_throttle(StabilizationDesiredData *stabilization_desired, SystemSettingsAirframeTypeOptions *airframe_type)
{
	if (*airframe_type == SYSTEMSETTINGS_AIRFRAMETYPE_HELICP) {
		float heli_throttle;
		ManualControlCommandThrottleGet( &heli_throttle );
		return heli_throttle;
	}
	return stabilization_desired->Thrust;
}

/**
 * Module initialization
 */
int32_t StabilizationStart()
{
	// Initialize variables
	// Create object queue
	queue = PIOS_Queue_Create(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));

	// Listen for updates.
	//	AttitudeActualConnectQueue(queue);
	GyrosConnectQueue(queue);

	// Connect settings callback
	StabilizationSettingsConnectCallback(SettingsUpdatedCb);
	VbarSettingsConnectCallback(SettingsUpdatedCb);
	SubTrimSettingsConnectCallback(SettingsUpdatedCb);

	// Watchdog must be registered before starting task
	PIOS_WDG_RegisterFlag(PIOS_WDG_STABILIZATION);

	// Start main task
	taskHandle = PIOS_Thread_Create(stabilizationTask, "Stabilization", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
	TaskMonitorAdd(TASKINFO_RUNNING_STABILIZATION, taskHandle);

	return 0;
}

/**
 * Module initialization
 */
int32_t StabilizationInitialize()
{
	// Initialize variables
	if (StabilizationSettingsInitialize() == -1
		|| ActuatorDesiredInitialize() == -1
		|| SubTrimInitialize() == -1
		|| SubTrimSettingsInitialize() == -1
		|| ManualControlCommandInitialize() == -1
		|| VbarSettingsInitialize() == -1) {
		return -1;
	}

#if defined(RATEDESIRED_DIAGNOSTICS)
	if (RateDesiredInitialize() == -1) {
		return -1;
	}
#endif

	return 0;
}

void stabilization_failsafe_checks(StabilizationDesiredData *stab_desired,
	ActuatorDesiredData *actuator_desired,
	SystemSettingsAirframeTypeOptions airframe_type,
	float *input)
{
	for(int i = 0; i < MAX_AXES; i++)
	{
		uint8_t *mode = stab_desired->StabilizationMode;
		input[i] = stab_desired->Roll+i;

		if (mode[i] == STABILIZATIONDESIRED_STABILIZATIONMODE_FAILSAFE) {
			// Everything except planes should drop straight down
			if ((airframe_type != SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWING) &&
					(airframe_type != SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGELEVON) &&
					(airframe_type != SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGVTAIL)) {
				actuator_desired->Thrust = 0.0f;
				switch (i) {
					case 0: /* Roll */
						mode[i] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
						input[i] = 0;
						break;
					case 1:
					default:
						mode[i] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
						input[i] = 0;
						break;
					case 2:
						mode[i] = STABILIZATIONDESIRED_STABILIZATIONMODE_RATE;
						input[i] = 0;
						break;
				}
			}
			else
			{
				actuator_desired->Thrust = -1.0f;

				switch (i) {
					case 0: /* Roll */
						mode[i] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
						input[i] = -10;
						break;
					case 1:
					default:
						mode[i] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
						input[i] = 0;
						break;
					case 2:
						mode[i] = STABILIZATIONDESIRED_STABILIZATIONMODE_RATE;
						input[i] = -5;
						break;
				}
			}
		}
	}
}

MODULE_HIPRI_INITCALL(StabilizationInitialize, StabilizationStart);

/**
 * Module task
 */
static void stabilizationTask(void* parameters)
{
	UAVObjEvent ev;

	uint32_t timeval = PIOS_DELAY_GetRaw();

	ActuatorDesiredData actuatorDesired;
	StabilizationDesiredData stabDesired;
	RateDesiredData rateDesired;
	AttitudeActualData attitudeActual;
	GyrosData gyrosData;
	FlightStatusData flightStatus;
	SystemSettingsAirframeTypeOptions airframe_type;

	float *actuatorDesiredAxis = &actuatorDesired.Roll;
	float *rateDesiredAxis = &rateDesired.Roll;
	float horizonRateFraction = 0.0f;

	// Connect callbacks
	ActuatorDesiredConnectCallbackCtx(UAVObjCbSetFlag, &actuatorDesiredUpdated);
	FlightStatusConnectCallbackCtx(UAVObjCbSetFlag, &flightStatusUpdated);
	SystemSettingsConnectCallbackCtx(UAVObjCbSetFlag, &systemSettingsUpdated);

	// Force refresh of all settings immediately before entering main task loop
	SettingsUpdatedCb(NULL, NULL, NULL, 0);

	uint32_t iteration = 0;
	float dT_measured = 0;

	uint8_t ident_shift = 5;

	float dT_expected = 0.001;	// assume 1KHz if we don't know.

	uint16_t samp_rate = PIOS_SENSORS_GetSampleRate(PIOS_SENSOR_GYRO);

	if (samp_rate) {
		dT_expected = 1.0f / samp_rate;
	}

	if (dT_expected < 0.0004f) {
		// For future 3.2KHz-- 640ms period
		ident_shift = 8;
	} else if (dT_expected < 0.0008f) {
		// 2KHz - 512ms period
		// 1.6KHz - 640ms period
		ident_shift = 7;
	} else if (dT_expected < 0.0014f) {
		// 1.2KHz - 427ms
		// 1KHz - 512ms period
		// 800Hz - 640ms period
		// 750Hz - 683ms period
		ident_shift = 6;
	} else {
		// 700Hz - 365ms period
		// 500Hz - 512ms period
		// 333Hz - 768ms period
		ident_shift = 5;
	}

	ident_wiggle_points = (1 << (ident_shift + 3));
	uint32_t ident_mask = ident_wiggle_points - 1;

	// Main task loop
	zero_pids();
	while(1) {
		iteration++;

		PIOS_WDG_UpdateFlag(PIOS_WDG_STABILIZATION);

		// Wait until the AttitudeRaw object is updated, if a timeout then go to failsafe
		if (PIOS_Queue_Receive(queue, &ev, FAILSAFE_TIMEOUT_MS) != true)
		{
			AlarmsSet(SYSTEMALARMS_ALARM_STABILIZATION,SYSTEMALARMS_ALARM_WARNING);
			continue;
		}

		static bool frequency_wrong = false;

		float dT = PIOS_DELAY_DiffuS(timeval) * 1.0e-6f;
		timeval = PIOS_DELAY_GetRaw();

		if (iteration < 100) {
			dT_measured = 0;
		} else if (iteration < 2100) {
			dT_measured += dT;
		} else if (iteration == 2100) {
			dT_measured /= 2000;

			/* Other modules-- attitude, etc, -- rely on us having
			 * done this test and set an alarm here.  Do not remove
			 * without verifying those places
			 */
			if ((dT_measured > dT_expected * 1.15f) || 
					(dT_measured < dT_expected * 0.85f)) {
				frequency_wrong = true;
			}
		}

		bool error = frequency_wrong;

		if (gyro_filter_updated) {
			if (settings.GyroCutoff < 1.0f) {
				gyro_alpha = 0;
			} else {
				gyro_alpha = expf(-2.0f * (float)(M_PI) *
						settings.GyroCutoff * dT_expected);
			}

			// Default 350ms.
			// 175ms to 39.3% of response
			// 350ms to 63.2% of response
			// 700ms to 86.4% of response
			max_rate_alpha = expf(-dT_expected / settings.AcroDynamicTau);

			// Compute time constant for vbar decay term
			if (vbar_settings.VbarTau < 0.001f) {
				vbar_decay = 0;
			} else {
				vbar_decay = expf(-dT_expected / vbar_settings.VbarTau);
			}

			gyro_filter_updated = false;
		}

		if (actuatorDesiredUpdated) {
			ActuatorDesiredGet(&actuatorDesired);
			actuatorDesiredUpdated = false;
		}

		if (flightStatusUpdated) {
			FlightStatusGet(&flightStatus);
			flightStatusUpdated = false;
		}

		if (systemSettingsUpdated) {
			SystemSettingsAirframeTypeGet(&airframe_type);
			systemSettingsUpdated = false;
		}

		StabilizationDesiredGet(&stabDesired);
		AttitudeActualGet(&attitudeActual);
		GyrosGet(&gyrosData);

		actuatorDesired.Thrust = stabDesired.Thrust;

		// Re-project axes if necessary prior to running stabilization algorithms.
		uint8_t reprojection = stabDesired.ReprojectionMode;
		static uint8_t previous_reprojection = 255;

		if (reprojection == STABILIZATIONDESIRED_REPROJECTIONMODE_CAMERAANGLE) {
			float camera_tilt_angle = settings.CameraTilt;
			if (camera_tilt_angle) {
				float roll = stabDesired.Roll;
				float yaw = stabDesired.Yaw;
				// The roll input should be the cosine of the camera angle multiplied by the roll,
				// added to the sine of camera angle multiplied by yaw.
				stabDesired.Roll = (cosf(DEG2RAD * camera_tilt_angle) * roll +
						(sinf(DEG2RAD * camera_tilt_angle) * yaw));
				// Yaw is similar but uses the negative sine of the camera angle, multiplied by roll,
				// added to the cosine of the camera angle, times the yaw
				stabDesired.Yaw = (-1 * sinf(DEG2RAD * camera_tilt_angle) * roll) +
						(cosf(DEG2RAD * camera_tilt_angle) * yaw);
			}
		} else if (reprojection == STABILIZATIONDESIRED_REPROJECTIONMODE_HEADFREE) {
			static float reference_yaw;

			if (previous_reprojection != reprojection) {
				reference_yaw = attitudeActual.Yaw;
			}

			float rotation_angle = attitudeActual.Yaw - reference_yaw;
			float roll = stabDesired.Roll;
			float pitch = stabDesired.Pitch;

			stabDesired.Roll = cosf(DEG2RAD * rotation_angle) * roll
					+ sinf(DEG2RAD * rotation_angle) * pitch;
			stabDesired.Pitch = cosf(DEG2RAD * rotation_angle) * pitch
					+ sinf(DEG2RAD * rotation_angle) * -roll;
		}

		previous_reprojection = reprojection;

#if defined(RATEDESIRED_DIAGNOSTICS)
		RateDesiredGet(&rateDesired);
#endif
		// raw_input will contain desired stabilization or the failsafe overrides.
		float raw_input[MAX_AXES];
		stabilization_failsafe_checks(&stabDesired, &actuatorDesired, airframe_type, raw_input);

		struct TrimmedAttitudeSetpoint {
			float Roll;
			float Pitch;
			float Yaw;
		} trimmedAttitudeSetpoint;

		// Mux in level trim values, and saturate the trimmed attitude setpoint.
		trimmedAttitudeSetpoint.Roll = bound_min_max(
			stabDesired.Roll + subTrim.Roll,
			-settings.RollMax + subTrim.Roll,
			 settings.RollMax + subTrim.Roll);
		trimmedAttitudeSetpoint.Pitch = bound_min_max(
			stabDesired.Pitch + subTrim.Pitch,
			-settings.PitchMax + subTrim.Pitch,
			 settings.PitchMax + subTrim.Pitch);
		trimmedAttitudeSetpoint.Yaw = stabDesired.Yaw;

		// For horizon mode we need to compute the desire attitude from an unscaled value and apply the
		// trim offset. Also track the stick with the most deflection to choose rate blending.
		horizonRateFraction = 0.0f;
		if (stabDesired.StabilizationMode[ROLL] == STABILIZATIONDESIRED_STABILIZATIONMODE_HORIZON) {
			trimmedAttitudeSetpoint.Roll = bound_min_max(
				stabDesired.Roll * settings.RollMax + subTrim.Roll,
				-settings.RollMax + subTrim.Roll,
				 settings.RollMax + subTrim.Roll);
			horizonRateFraction = fabsf(stabDesired.Roll);
		}
		if (stabDesired.StabilizationMode[PITCH] == STABILIZATIONDESIRED_STABILIZATIONMODE_HORIZON) {
			trimmedAttitudeSetpoint.Pitch = bound_min_max(
				stabDesired.Pitch * settings.PitchMax + subTrim.Pitch,
				-settings.PitchMax + subTrim.Pitch,
				 settings.PitchMax + subTrim.Pitch);
			horizonRateFraction = MAX(horizonRateFraction, fabsf(stabDesired.Pitch));
		}
		if (stabDesired.StabilizationMode[YAW] == STABILIZATIONDESIRED_STABILIZATIONMODE_HORIZON) {
			trimmedAttitudeSetpoint.Yaw = stabDesired.Yaw * settings.YawMax;
			horizonRateFraction = MAX(horizonRateFraction, fabsf(stabDesired.Yaw));
		}

		// For weak leveling mode the attitude setpoint is the trim value (drifts back towards "0")
		if (stabDesired.StabilizationMode[ROLL] == STABILIZATIONDESIRED_STABILIZATIONMODE_WEAKLEVELING) {
			trimmedAttitudeSetpoint.Roll = subTrim.Roll;
		}
		if (stabDesired.StabilizationMode[PITCH] == STABILIZATIONDESIRED_STABILIZATIONMODE_WEAKLEVELING) {
			trimmedAttitudeSetpoint.Pitch = subTrim.Pitch;
		}
		if (stabDesired.StabilizationMode[YAW] == STABILIZATIONDESIRED_STABILIZATIONMODE_WEAKLEVELING) {
			trimmedAttitudeSetpoint.Yaw = 0;
		}

		// Note we divide by the maximum limit here so the fraction ranges from 0 to 1 depending on
		// how much is requested.
		horizonRateFraction = bound_sym(horizonRateFraction, HORIZON_MODE_MAX_BLEND) / HORIZON_MODE_MAX_BLEND;

		// Calculate the errors in each axis. The local error is used in the following modes:
		//  ATTITUDE, HORIZON, WEAKLEVELING
		float local_attitude_error[3];
		local_attitude_error[0] = trimmedAttitudeSetpoint.Roll - attitudeActual.Roll;
		local_attitude_error[1] = trimmedAttitudeSetpoint.Pitch - attitudeActual.Pitch;
		local_attitude_error[2] = trimmedAttitudeSetpoint.Yaw - attitudeActual.Yaw;

		// Wrap yaw error to [-180,180]
		local_attitude_error[2] = circular_modulus_deg(local_attitude_error[2]);

		static float gyro_filtered[3];

		gyro_filtered[0] = gyro_filtered[0] * gyro_alpha + gyrosData.x * (1 - gyro_alpha);
		gyro_filtered[1] = gyro_filtered[1] * gyro_alpha + gyrosData.y * (1 - gyro_alpha);
		gyro_filtered[2] = gyro_filtered[2] * gyro_alpha + gyrosData.z * (1 - gyro_alpha);

		/* Maintain a second-order, lower cutof freq variant for
		 * dynamic flight modes.
		 */

		static float max_rate_filtered[3];

		// A flag to track which stabilization mode each axis is in
		static uint8_t previous_mode[MAX_AXES] = {255,255,255};

		actuatorDesired.SystemIdentCycle = 0xffff;

		uint16_t max_safe_rate = PIOS_SENSORS_GetMaxGyro() * 0.9f;

		//Run the selected stabilization algorithm on each axis:
		for(uint8_t i=0; i< MAX_AXES; i++)
		{
			// XXX TODO: Factor this body out into function.

			uint8_t mode = stabDesired.StabilizationMode[i];

			// Check whether this axis mode needs to be reinitialized
			bool reinit = (mode != previous_mode[i]);
			previous_mode[i] = mode;

			// Apply the selected control law
			switch(mode)
			{
				case STABILIZATIONDESIRED_STABILIZATIONMODE_FAILSAFE:
					PIOS_Assert(0); /* Shouldn't happen, per above */
					break;

				case STABILIZATIONDESIRED_STABILIZATIONMODE_RATE:
					if(reinit) {
						pids[PID_GROUP_RATE + i].iAccumulator = 0;
					}

					// Store to rate desired variable for storing to UAVO
					rateDesiredAxis[i] = bound_sym(raw_input[i], settings.ManualRate[i]);

					// Compute the inner loop
					actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i),  rateDesiredAxis[i],  gyro_filtered[i], dT_expected);
					actuatorDesiredAxis[i] = bound_sym(actuatorDesiredAxis[i],1.0f);

					break;

				case STABILIZATIONDESIRED_STABILIZATIONMODE_ACRODYNE:
					if(reinit) {
						pids[PID_GROUP_RATE + i].iAccumulator = 0;
						max_rate_filtered[i] = settings.ManualRate[i];
					}

					float curve_cmd = expoM(raw_input[i],
							settings.RateExpo[i],
							settings.RateExponent[i]*0.1f);

					const float break_point = settings.AcroDynamicTransition[i]/100.0f;

					uint16_t calc_max_rate = settings.ManualRate[i];

					float abs_cmd = fabsf(raw_input[i]);

					uint16_t acro_dynrate = settings.AcroDynamicRate[i];

					if (!acro_dynrate) {
						acro_dynrate = settings.ManualRate[i] + settings.ManualRate[i] / 2;
					}

					if (acro_dynrate > max_safe_rate) {
						acro_dynrate = max_safe_rate;
					}

					// Could precompute much of this...
					if (abs_cmd > break_point) {
						calc_max_rate = (settings.ManualRate[i] * (abs_cmd - 1.0f) * (2 * break_point - abs_cmd - 1.0f) + acro_dynrate * powf(break_point - abs_cmd, 2.0f)) / powf(break_point - 1.0f, 2.0f);
					}

					calc_max_rate = MIN(calc_max_rate,
							acro_dynrate);

					max_rate_filtered[i] = max_rate_filtered[i] * max_rate_alpha + calc_max_rate * (1 - max_rate_alpha);

					// Store to rate desired variable for storing to UAVO
					rateDesiredAxis[i] = bound_sym(curve_cmd * max_rate_filtered[i], max_rate_filtered[i]);

					// Compute the inner loop
					actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i), rateDesiredAxis[i], gyro_filtered[i], dT);
					actuatorDesiredAxis[i] = bound_sym(actuatorDesiredAxis[i], 1.0f);

					break;

				case STABILIZATIONDESIRED_STABILIZATIONMODE_ACROPLUS:
					// this implementation is based on the Openpilot/Librepilot Acro+ flightmode
					// and our previous MWRate flightmodes
					if(reinit) {
						pids[PID_GROUP_RATE + i].iAccumulator = 0;
					}

					// The factor for gyro suppression / mixing raw stick input into the output; scaled by raw stick input
					float factor = fabsf(raw_input[i]) * settings.AcroInsanityFactor / 100.0f;

					// Store to rate desired variable for storing to UAVO
					rateDesiredAxis[i] = bound_sym(raw_input[i] * settings.ManualRate[i], settings.ManualRate[i]);

					// Zero integral for aggressive maneuvers
					if ((i < 2 && fabsf(gyro_filtered[i]) > settings.AcroZeroIntegralGyro) ||
						(i == 0 && fabsf(raw_input[i]) > settings.AcroZeroIntegralStick / 100.0f)) {
							pids[PID_GROUP_RATE + i].iAccumulator = 0;
							}

					// Compute the inner loop
					actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i), rateDesiredAxis[i], gyro_filtered[i], dT_expected);
					actuatorDesiredAxis[i] = factor * raw_input[i] + (1.0f - factor) * actuatorDesiredAxis[i];
					actuatorDesiredAxis[i] = bound_sym(actuatorDesiredAxis[i], 1.0f);

					break;

				case STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE:
					if(reinit) {
						pids[PID_GROUP_ATT + i].iAccumulator = 0;
						pids[PID_GROUP_RATE + i].iAccumulator = 0;
					}

					// Compute the outer loop
					rateDesiredAxis[i] = pid_apply(&pids[PID_GROUP_ATT + i], local_attitude_error[i], dT_expected);
					rateDesiredAxis[i] = bound_sym(rateDesiredAxis[i], settings.MaximumRate[i]);

					// Compute the inner loop
					actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i),  rateDesiredAxis[i],  gyro_filtered[i], dT_expected);
					actuatorDesiredAxis[i] = bound_sym(actuatorDesiredAxis[i],1.0f);

					break;

				case STABILIZATIONDESIRED_STABILIZATIONMODE_VIRTUALBAR:
					// Store for debugging output
					rateDesiredAxis[i] = raw_input[i];

					// Run a virtual flybar stabilization algorithm on this axis
					stabilization_virtual_flybar(gyro_filtered[i], rateDesiredAxis[i], &actuatorDesiredAxis[i], dT_expected, reinit, i, &pids[PID_GROUP_VBAR + i], &vbar_settings);

					break;

				case STABILIZATIONDESIRED_STABILIZATIONMODE_WEAKLEVELING:
				{
					if (reinit) {
						pids[PID_GROUP_RATE + i].iAccumulator = 0;
					}

					float weak_leveling = local_attitude_error[i] * weak_leveling_kp;
					weak_leveling = bound_sym(weak_leveling, weak_leveling_max);

					// Compute desired rate as input biased towards leveling
					rateDesiredAxis[i] = raw_input[i] + weak_leveling;
					actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i),  rateDesiredAxis[i],  gyro_filtered[i], dT_expected);
					actuatorDesiredAxis[i] = bound_sym(actuatorDesiredAxis[i],1.0f);

					break;
				}
				case STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK:
					if (reinit) {
						pids[PID_GROUP_RATE + i].iAccumulator = 0;
					}

					if (fabsf(raw_input[i]) > max_axislock_rate) {
						// While getting strong commands act like rate mode
						rateDesiredAxis[i] = bound_sym(raw_input[i], settings.ManualRate[i]);

						// Reset accumulator
						axis_lock_accum[i] = 0;
					} else {
						// For weaker commands or no command simply lock (almost) on no gyro change
						axis_lock_accum[i] += (raw_input[i] - gyro_filtered[i]) * dT_expected;
						axis_lock_accum[i] = bound_sym(axis_lock_accum[i], max_axis_lock);

						// Compute the inner loop
						float tmpRateDesired = pid_apply(&pids[PID_GROUP_ATT + i], axis_lock_accum[i], dT_expected);
						rateDesiredAxis[i] = bound_sym(tmpRateDesired, settings.MaximumRate[i]);
					}

					actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i),  rateDesiredAxis[i],  gyro_filtered[i], dT_expected);
					actuatorDesiredAxis[i] = bound_sym(actuatorDesiredAxis[i],1.0f);

					break;

				case STABILIZATIONDESIRED_STABILIZATIONMODE_HORIZON:
					if(reinit) {
						pids[PID_GROUP_RATE + i].iAccumulator = 0;
					}

					// Do not allow outer loop integral to wind up in this mode since the controller
					// is often disengaged.
					pids[PID_GROUP_ATT + i].iAccumulator = 0;

					// Compute the outer loop for the attitude control
					float rateDesiredAttitude = pid_apply(&pids[PID_GROUP_ATT + i], local_attitude_error[i], dT_expected);
					// Compute the desire rate for a rate control
					float rateDesiredRate = raw_input[i] * settings.ManualRate[i];

					// Blend from one rate to another. The maximum of all stick positions is used for the
					// amount so that when one axis goes completely to rate the other one does too. This
					// prevents doing flips while one axis tries to stay in attitude mode.
					rateDesiredAxis[i] = rateDesiredAttitude * (1.0f-horizonRateFraction) + rateDesiredRate * horizonRateFraction;
					rateDesiredAxis[i] = bound_sym(rateDesiredAxis[i], settings.ManualRate[i]);

					// Compute the inner loop
					actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i),  rateDesiredAxis[i],  gyro_filtered[i], dT_expected);
					actuatorDesiredAxis[i] = bound_sym(actuatorDesiredAxis[i],1.0f);

					break;
				case STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT:
				case STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENTRATE:
					;
					static bool measuring;
					static uint32_t enter_time;

					static uint32_t measure_remaining;

					// Takes 1250ms + the time to reach
					// the '0th measurement'
					// (could be the ~600ms period time)
					const uint32_t PREPARE_TIME = 1250000;

					if (reinit) {
						pids[PID_GROUP_ATT + i].iAccumulator = 0;
						pids[PID_GROUP_RATE + i].iAccumulator = 0;

						if (i == 0) {
							enter_time = timeval;

							measuring = false;
						}
					}

					if ((i == 0) &&
							(!measuring) &&
							((timeval - enter_time) > PREPARE_TIME)) {
						if (!(iteration & ident_mask)) {
							measuring = true;
							measure_remaining = 60 / dT_expected;
							// Round down to an integer
							// number of ident cycles.
							measure_remaining &= ~ident_mask;
						}

					}

					if (mode == STABILIZATIONDESIRED_STABILIZATIONMODE_SYSTEMIDENT) {
						// Compute the outer loop
						rateDesiredAxis[i] = pid_apply(&pids[PID_GROUP_ATT + i], local_attitude_error[i], dT_expected);
						rateDesiredAxis[i] = bound_sym(rateDesiredAxis[i], settings.MaximumRate[i]);

						// Compute the inner loop
						actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i), rateDesiredAxis[i],  gyro_filtered[i], dT_expected);
					} else {
						// Get the desired rate. yaw is always in rate mode in system ident.
						rateDesiredAxis[i] = bound_sym(raw_input[i], settings.ManualRate[i]);

						// Compute the inner loop only for yaw
						actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i), rateDesiredAxis[i],  gyro_filtered[i], dT_expected);
					}

					const float scale = settings.AutotuneActuationEffort[i];

					uint32_t ident_iteration =
						iteration >> ident_shift;

					if (measuring && measure_remaining) {
						if (i == 2) {
							// Only adjust the
							// counter on one axis
							measure_remaining--;
						}

						actuatorDesired.SystemIdentCycle = (iteration & ident_mask);

						switch (ident_iteration & 0x07) {
							case 0:
								if (i == 2) {
									actuatorDesiredAxis[i] += scale;
								}
								break;
							case 1:
								if (i == 0) {
									actuatorDesiredAxis[i] += scale;
								}
								break;
							case 2:
								if (i == 2) {
									actuatorDesiredAxis[i] -= scale;
								}
								break;
							case 3:
								if (i == 0) {
									actuatorDesiredAxis[i] -= scale;
								}
								break;
							case 4:
								if (i == 2) {
									actuatorDesiredAxis[i] += scale;
								}
								break;
							case 5:
								if (i == 1) {
									actuatorDesiredAxis[i] += scale;
								}
								break;
							case 6:
								if (i == 2) {
									actuatorDesiredAxis[i] -= scale;
								}
								break;
							case 7:
								if (i == 1) {
									actuatorDesiredAxis[i] -= scale;
								}
								break;
						}

						actuatorDesiredAxis[i] = bound_sym(actuatorDesiredAxis[i],1.0f);
					}

					break;

				case STABILIZATIONDESIRED_STABILIZATIONMODE_COORDINATEDFLIGHT:
					switch (i) {
						case YAW:
							if (reinit) {
								pids[PID_COORDINATED_FLIGHT_YAW].iAccumulator = 0;
								pids[PID_RATE_YAW].iAccumulator = 0;
								axis_lock_accum[YAW] = 0;
							}

							//If we are not in roll attitude mode, trigger an error
							if (stabDesired.StabilizationMode[ROLL] != STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE)
							{
								error = true;
								break ;
							}

							if (fabsf(stabDesired.Yaw) < COORDINATED_FLIGHT_MAX_YAW_THRESHOLD) { //If yaw is within the deadband...
								if (fabsf(stabDesired.Roll) > COORDINATED_FLIGHT_MIN_ROLL_THRESHOLD) { // We're requesting more roll than the threshold
									float accelsDataY;
									AccelsyGet(&accelsDataY);

									//Reset integral if we have changed roll to opposite direction from rudder. This implies that we have changed desired turning direction.
									if ((stabDesired.Roll > 0 && actuatorDesiredAxis[YAW] < 0) ||
											(stabDesired.Roll < 0 && actuatorDesiredAxis[YAW] > 0)){
										pids[PID_COORDINATED_FLIGHT_YAW].iAccumulator = 0;
									}

									// Coordinate flight can simply be seen as ensuring that there is no lateral acceleration in the
									// body frame. As such, we use the (noisy) accelerometer data as our measurement. Ideally, at
									// some point in the future we will estimate acceleration and then we can use the estimated value
									// instead of the measured value.
									float errorSlip = -accelsDataY;

									float command = pid_apply(&pids[PID_COORDINATED_FLIGHT_YAW], errorSlip, dT_expected);
									actuatorDesiredAxis[YAW] = bound_sym(command ,1.0);

									// Reset axis-lock integrals
									pids[PID_RATE_YAW].iAccumulator = 0;
									axis_lock_accum[YAW] = 0;
								} else if (fabsf(stabDesired.Roll) <= COORDINATED_FLIGHT_MIN_ROLL_THRESHOLD) { // We're requesting less roll than the threshold
									// Axis lock on no gyro change
									axis_lock_accum[YAW] += (0 - gyro_filtered[YAW]) * dT_expected;

									rateDesiredAxis[YAW] = pid_apply(&pids[PID_ATT_YAW], axis_lock_accum[YAW], dT_expected);
									rateDesiredAxis[YAW] = bound_sym(rateDesiredAxis[YAW], settings.MaximumRate[YAW]);

									actuatorDesiredAxis[YAW] = pid_apply_setpoint(&pids[PID_RATE_YAW], NULL, rateDesiredAxis[YAW], gyro_filtered[YAW], dT_expected);
									actuatorDesiredAxis[YAW] = bound_sym(actuatorDesiredAxis[YAW],1.0f);

									// Reset coordinated-flight integral
									pids[PID_COORDINATED_FLIGHT_YAW].iAccumulator = 0;
								}
							} else { //... yaw is outside the deadband. Pass the manual input directly to the actuator.
								actuatorDesiredAxis[YAW] = bound_sym(raw_input[YAW], 1.0);

								// Reset all integrals
								pids[PID_COORDINATED_FLIGHT_YAW].iAccumulator = 0;
								pids[PID_RATE_YAW].iAccumulator = 0;
								axis_lock_accum[YAW] = 0;
							}
							break;
						case ROLL:
						case PITCH:
						default:
							//Coordinated Flight has no effect in these modes. Trigger a configuration error.
							error = true;
							break;
					}

					break;

				case STABILIZATIONDESIRED_STABILIZATIONMODE_POI:
					// The sanity check enforces this is only selectable for Yaw
					// for a gimbal you can select pitch too.
					if(reinit) {
						pids[PID_GROUP_ATT + i].iAccumulator = 0;
						pids[PID_GROUP_RATE + i].iAccumulator = 0;
					}

					float error;
					float angle;
					if (CameraDesiredHandle()) {
						switch(i) {
						case PITCH:
							CameraDesiredDeclinationGet(&angle);
							error = circular_modulus_deg(angle - attitudeActual.Pitch);
							break;
						case ROLL:
						{
							uint8_t roll_fraction = 0;

							// For ROLL POI mode we track the FC roll angle (scaled) to
							// allow keeping some motion
							CameraDesiredRollGet(&angle);
							angle *= roll_fraction / 100.0f;
							error = circular_modulus_deg(angle - attitudeActual.Roll);
						}
							break;
						case YAW:
							CameraDesiredBearingGet(&angle);
							error = circular_modulus_deg(angle - attitudeActual.Yaw);
							break;
						default:
							error = true;
						}
					} else
						error = true;

					// Compute the outer loop
					rateDesiredAxis[i] = pid_apply(&pids[PID_GROUP_ATT + i], error, dT_expected);
					rateDesiredAxis[i] = bound_sym(rateDesiredAxis[i], settings.PoiMaximumRate[i]);

					// Compute the inner loop
					actuatorDesiredAxis[i] = pid_apply_setpoint(&pids[PID_GROUP_RATE + i], get_deadband(i), rateDesiredAxis[i], gyro_filtered[i], dT_expected);
					actuatorDesiredAxis[i] = bound_sym(actuatorDesiredAxis[i],1.0f);

					break;
				case STABILIZATIONDESIRED_STABILIZATIONMODE_DISABLED:
					actuatorDesiredAxis[i] = 0.0;
					break;
				case STABILIZATIONDESIRED_STABILIZATIONMODE_MANUAL:
					actuatorDesiredAxis[i] = bound_sym(raw_input[i],1.0f);
					break;
				default:
					error = true;
					break;
			}
		}

		if (vbar_settings.VbarPiroComp == VBARSETTINGS_VBARPIROCOMP_TRUE)
			stabilization_virtual_flybar_pirocomp(gyro_filtered[2], dT_expected);

#if defined(RATEDESIRED_DIAGNOSTICS)
		RateDesiredSet(&rateDesired);
#endif

		// Save dT
		actuatorDesired.UpdateTime = dT * 1000;

		ActuatorDesiredSet(&actuatorDesired);
		// So we only fetch it above if it is modified by another module (wacky)
		actuatorDesiredUpdated = false;

		if(flightStatus.Armed != FLIGHTSTATUS_ARMED_ARMED ||
		   (lowThrottleZeroIntegral && get_throttle(&stabDesired, &airframe_type) < 0))
		{
			// Force all axes to reinitialize when engaged
			for(uint8_t i=0; i< MAX_AXES; i++)
				previous_mode[i] = 255;
		}

		// Clear or set alarms.  Done like this to prevent toggling each cycle
		// and hammering system alarms
		if (error)
			AlarmsSet(SYSTEMALARMS_ALARM_STABILIZATION, SYSTEMALARMS_ALARM_ERROR);
		else
			AlarmsClear(SYSTEMALARMS_ALARM_STABILIZATION);
	}
}


/**
 * Clear the accumulators and derivatives for all the axes
 */
static void zero_pids(void)
{
	for(uint32_t i = 0; i < PID_MAX; i++)
		pid_zero(&pids[i]);


	for(uint8_t i = 0; i < 3; i++)
		axis_lock_accum[i] = 0.0f;
}

static void calculate_pids()
{
	// Set the roll rate PID constants
	pid_configure(&pids[PID_RATE_ROLL],
	              settings.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KP],
	              settings.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KI],
	              settings.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KD],
	              settings.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_ILIMIT]);

	// Set the pitch rate PID constants
	pid_configure(&pids[PID_RATE_PITCH],
	              settings.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KP],
	              settings.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KI],
	              settings.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KD],
	              settings.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_ILIMIT]);

	// Set the yaw rate PID constants
	pid_configure(&pids[PID_RATE_YAW],
	              settings.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_KP],
	              settings.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_KI],
	              settings.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_KD],
	              settings.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_ILIMIT]);

	// Set the roll attitude PI constants
	pid_configure(&pids[PID_ATT_ROLL],
	              settings.RollPI[STABILIZATIONSETTINGS_ROLLPI_KP],
	              settings.RollPI[STABILIZATIONSETTINGS_ROLLPI_KI], 0,
	              settings.RollPI[STABILIZATIONSETTINGS_ROLLPI_ILIMIT]);

	// Set the pitch attitude PI constants
	pid_configure(&pids[PID_ATT_PITCH],
	              settings.PitchPI[STABILIZATIONSETTINGS_PITCHPI_KP],
	              settings.PitchPI[STABILIZATIONSETTINGS_PITCHPI_KI], 0,
	              settings.PitchPI[STABILIZATIONSETTINGS_PITCHPI_ILIMIT]);

	// Set the yaw attitude PI constants
	pid_configure(&pids[PID_ATT_YAW],
	              settings.YawPI[STABILIZATIONSETTINGS_YAWPI_KP],
	              settings.YawPI[STABILIZATIONSETTINGS_YAWPI_KI], 0,
	              settings.YawPI[STABILIZATIONSETTINGS_YAWPI_ILIMIT]);

	// Set the vbar roll settings
	pid_configure(&pids[PID_VBAR_ROLL],
	              vbar_settings.VbarRollPID[VBARSETTINGS_VBARROLLPID_KP],
	              vbar_settings.VbarRollPID[VBARSETTINGS_VBARROLLPID_KI],
	              vbar_settings.VbarRollPID[VBARSETTINGS_VBARROLLPID_KD],
	              0);

	// Set the vbar pitch settings
	pid_configure(&pids[PID_VBAR_PITCH],
	              vbar_settings.VbarPitchPID[VBARSETTINGS_VBARPITCHPID_KP],
	              vbar_settings.VbarPitchPID[VBARSETTINGS_VBARPITCHPID_KI],
	              vbar_settings.VbarPitchPID[VBARSETTINGS_VBARPITCHPID_KD],
	              0);

	// Set the vbar yaw settings
	pid_configure(&pids[PID_VBAR_YAW],
	              vbar_settings.VbarYawPID[VBARSETTINGS_VBARYAWPID_KP],
	              vbar_settings.VbarYawPID[VBARSETTINGS_VBARYAWPID_KI],
	              vbar_settings.VbarYawPID[VBARSETTINGS_VBARYAWPID_KD],
	              0);

	// Set the coordinated flight settings
	pid_configure(&pids[PID_COORDINATED_FLIGHT_YAW],
	              settings.CoordinatedFlightYawPI[STABILIZATIONSETTINGS_COORDINATEDFLIGHTYAWPI_KP],
	              settings.CoordinatedFlightYawPI[STABILIZATIONSETTINGS_COORDINATEDFLIGHTYAWPI_KI],
	              0, /* No derivative term */
	              settings.CoordinatedFlightYawPI[STABILIZATIONSETTINGS_COORDINATEDFLIGHTYAWPI_ILIMIT]);

	// Set up the derivative term
	pid_configure_derivative(settings.DerivativeCutoff, settings.DerivativeGamma);

#ifndef NO_CONTROL_DEADBANDS	
	if(deadbands ||
		settings.DeadbandWidth[STABILIZATIONSETTINGS_DEADBANDWIDTH_ROLL] ||
		settings.DeadbandWidth[STABILIZATIONSETTINGS_DEADBANDWIDTH_PITCH] ||
		settings.DeadbandWidth[STABILIZATIONSETTINGS_DEADBANDWIDTH_YAW])
	{
		if(!deadbands) deadbands = PIOS_malloc(sizeof(struct pid_deadband) * MAX_AXES);

		pid_configure_deadband(deadbands + PID_RATE_ROLL, (float)settings.DeadbandWidth[STABILIZATIONSETTINGS_DEADBANDWIDTH_ROLL],
			0.01f * (float)settings.DeadbandSlope[STABILIZATIONSETTINGS_DEADBANDSLOPE_ROLL]);
		pid_configure_deadband(deadbands + PID_RATE_PITCH, (float)settings.DeadbandWidth[STABILIZATIONSETTINGS_DEADBANDWIDTH_PITCH],
			0.01f * (float)settings.DeadbandSlope[STABILIZATIONSETTINGS_DEADBANDSLOPE_PITCH]);
		pid_configure_deadband(deadbands + PID_RATE_YAW, (float)settings.DeadbandWidth[STABILIZATIONSETTINGS_DEADBANDWIDTH_YAW],
			0.01f * (float)settings.DeadbandSlope[STABILIZATIONSETTINGS_DEADBANDSLOPE_YAW]);
	}
#endif
}

static void SettingsUpdatedCb(UAVObjEvent * ev, void *ctx, void *obj, int len)
{
	(void) ctx; (void) obj; (void) len;

	if (ev == NULL || ev->obj == SubTrimSettingsHandle())
	{
		SubTrimSettingsData subTrimSettings;

		SubTrimGet(&subTrim);
		SubTrimSettingsGet(&subTrimSettings);

		// Set the trim angles
		subTrim.Roll = subTrimSettings.Roll;
		subTrim.Pitch = subTrimSettings.Pitch;

		SubTrimSet(&subTrim);
	}

	if (ev == NULL || ev->obj == StabilizationSettingsHandle())
	{
		StabilizationSettingsGet(&settings);

		calculate_pids();

		// Maximum deviation to accumulate for axis lock
		max_axis_lock = settings.MaxAxisLock;
		max_axislock_rate = settings.MaxAxisLockRate;

		// Settings for weak leveling
		weak_leveling_kp = settings.WeakLevelingKp;
		weak_leveling_max = settings.MaxWeakLevelingRate;

		// Whether to zero the PID integrals while thrust is low
		lowThrottleZeroIntegral = settings.LowThrottleZeroIntegral == STABILIZATIONSETTINGS_LOWTHROTTLEZEROINTEGRAL_TRUE;

		gyro_filter_updated = true;
	}

	if (ev == NULL || ev->obj == VbarSettingsHandle())
	{
		VbarSettingsGet(&vbar_settings);

		calculate_pids();
	}
}


/**
 * @}
 * @}
 */
