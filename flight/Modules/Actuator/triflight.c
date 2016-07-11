/**
 ******************************************************************************
 * @addtogroup TriflightModule Triflight Module
 * @{
 *
 * @file       triflight.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @brief      Trifight module. Specialty mixing for tricopters.
 *
 * This module computes corrections for the tricopter's yaw servo and rear
 * motor, based on the yaw servo performance and commanded/actual angle.
 *
 * Based on algorithms and code developed here:
 *     http://rcexplorer.se/forums/topic/debugging-the-tricopter-mini-racer/
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

#include "pios_config.h"

#ifndef SMALLF1

#include "openpilot.h"
#include "triflight.h"
#include "misc_math.h"
#include "physical_constants.h"

#define TRI_TAIL_SERVO_ANGLE_MID  90.0f
#define TRI_TAIL_SERVO_ANGLE_MAX  50.0f
#define TRI_YAW_FORCE_CURVE_SIZE  100

static float tail_motor_acceleration_delay_angle;
static float tail_motor_deceleration_delay_angle;
static float tail_motor_pitch_zero_angle;
static float tail_servo_max_yaw_force = 0.0f;
static float yaw_force_curve[TRI_YAW_FORCE_CURVE_SIZE];

/**
 *
 *
 */
float getPitchCorrectionAtTailAngle(float angle, float thrust_factor)
{
    return 1.0f / (sinf(angle) - cosf(angle) / thrust_factor);
}

/**
 *
 *
 */
void triflightInit(ActuatorSettingsData  *actuatorSettings,
                   TriflightSettingsData *triflightSettings,
                   TriflightStatusData   *triflightStatus)
{
	float min_angle = TRI_TAIL_SERVO_ANGLE_MID - triflightSettings->ServoMaxAngle;
	float max_angle = TRI_TAIL_SERVO_ANGLE_MID + triflightSettings->ServoMaxAngle;

	float max_neg_force = 0.0f;
	float max_pos_force = 0.0f;

	float angle = TRI_TAIL_SERVO_ANGLE_MID - TRI_TAIL_SERVO_ANGLE_MAX;

	for (uint8_t i = 0; i < TRI_YAW_FORCE_CURVE_SIZE; i++)
	{
		float angle_rad = DEG2RAD * angle;

		yaw_force_curve[i] = (-triflightSettings->MotorThrustFactor * cosf(angle_rad) - sinf(angle_rad)) *
                              getPitchCorrectionAtTailAngle(angle_rad, triflightSettings->MotorThrustFactor);

		// Only calculate the top forces in the configured angle range
		if ((angle >= min_angle) && (angle <= max_angle))
		{
			max_neg_force = fminf(yaw_force_curve[i], max_neg_force);
			max_pos_force = fmaxf(yaw_force_curve[i], max_pos_force);
		}

		angle++;
	}

	tail_servo_max_yaw_force = fminf(fabsf(max_neg_force), fabsf(max_pos_force));

	tail_motor_pitch_zero_angle = 2.0f * (atanf(((sqrtf(triflightSettings->MotorThrustFactor *
	                                                    triflightSettings->MotorThrustFactor + 1) + 1) /
	                                                    triflightSettings->MotorThrustFactor)));

	tail_motor_acceleration_delay_angle = (triflightSettings->MotorAccelDelayMs / 1000.0f) * triflightSettings->ServoSpeed;
    tail_motor_deceleration_delay_angle = (triflightSettings->MotorDecelDelayMs / 1000.0f) * triflightSettings->ServoSpeed;
}

/**
 *
 *
 */
float getAngleFromYawCurveAtForce(float force)
{
	if (force < yaw_force_curve[0]) // No force that low
	{
		return TRI_TAIL_SERVO_ANGLE_MID - TRI_TAIL_SERVO_ANGLE_MAX;
	}
	else if (!(force < yaw_force_curve[TRI_YAW_FORCE_CURVE_SIZE - 1])) // No force that high
	{
		return TRI_TAIL_SERVO_ANGLE_MID + TRI_TAIL_SERVO_ANGLE_MAX;
	}

	// Binary search: yaw_force_curve[lower] <= force, yaw_force_curve[higher] > force
	uint8_t lower  = 0;
	uint8_t higher = TRI_YAW_FORCE_CURVE_SIZE - 1;

	while (higher > lower + 1)
    {
		uint8_t mid = (lower + higher) / 2;

		if (yaw_force_curve[mid] > force)
		{
			higher = mid;
		}
		else
		{
			lower = mid;
		}
	}

	// Interpolating
	return TRI_TAIL_SERVO_ANGLE_MID - TRI_TAIL_SERVO_ANGLE_MAX + (float)lower + (force - yaw_force_curve[lower]) / (yaw_force_curve[higher] - yaw_force_curve[lower]);
}

/**
 *
 *
 */
float getServoValueAtAngle(ActuatorSettingsData  *actuatorSettings,
                           TriflightSettingsData *triflightSettings,
                           TriflightStatusData   *triflightStatus,
                           float angle)
{
	bool  pwm_min_lt_pwm_max;
	bool  pwm_in_lt_pwm_neutral;

	float angle_delta;
	float angle_range;
	float pwm_range;
	float initial_pwm;

	pwm_min_lt_pwm_max = actuatorSettings->ChannelMin[triflightStatus->ServoChannel] < actuatorSettings->ChannelMax[triflightStatus->ServoChannel];

	pwm_in_lt_pwm_neutral =  angle < TRI_TAIL_SERVO_ANGLE_MID;

	if ((pwm_in_lt_pwm_neutral && pwm_min_lt_pwm_max) || (!pwm_in_lt_pwm_neutral && !pwm_min_lt_pwm_max))
	{
		angle_delta = angle - (TRI_TAIL_SERVO_ANGLE_MID + triflightSettings->ServoMaxAngle);
		angle_range = -triflightSettings->ServoMaxAngle;
		pwm_range   = actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel] - actuatorSettings->ChannelMin[triflightStatus->ServoChannel];
		initial_pwm  = actuatorSettings->ChannelMin[triflightStatus->ServoChannel];
	}
	else
	{
		angle_delta = angle - TRI_TAIL_SERVO_ANGLE_MID;
		angle_range = -triflightSettings->ServoMaxAngle;
		pwm_range   = actuatorSettings->ChannelMax[triflightStatus->ServoChannel] - actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];
		initial_pwm  = actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];
	}

	return angle_delta / angle_range * pwm_range + initial_pwm;
}

/**
 *
 *
 */
float getCorrectedServoValue(ActuatorSettingsData  *actuatorSettings,
                             TriflightSettingsData *triflightSettings,
                             TriflightStatusData   *triflightStatus,
                             float uncorrected_servo_value)
{
	// First find the yaw force at given servo value from a linear curve
	bool  pwm_min_lt_pwm_max;
	bool  pwm_in_lt_pwm_neutral;

	float pwm_delta;
	float pwm_range;

	pwm_min_lt_pwm_max = actuatorSettings->ChannelMin[triflightStatus->ServoChannel] < actuatorSettings->ChannelMax[triflightStatus->ServoChannel];

	pwm_in_lt_pwm_neutral = uncorrected_servo_value < actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];

	if ((pwm_in_lt_pwm_neutral && pwm_min_lt_pwm_max) || (!pwm_in_lt_pwm_neutral && !pwm_min_lt_pwm_max))
	{
		pwm_delta = actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel] - uncorrected_servo_value;
		pwm_range = actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel] - actuatorSettings->ChannelMin[triflightStatus->ServoChannel];
	}
	else
	{
		pwm_delta = actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel] - uncorrected_servo_value;
		pwm_range = actuatorSettings->ChannelMax[triflightStatus->ServoChannel] - actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];
	}

	float linear_yaw_force_at_servo_value = pwm_delta / pwm_range * tail_servo_max_yaw_force;

	// Then find the corrected angle
	float corrected_angle = getAngleFromYawCurveAtForce(linear_yaw_force_at_servo_value);

	// Then return the corrected PWM value
	return getServoValueAtAngle(actuatorSettings, triflightSettings, triflightStatus, corrected_angle);
}

#if !defined(ARCH_POSIX) && !defined(ARCH_WIN32)
typedef struct filterStatePt1_s {
	float state;
	float RC;
} filterStatePt1_t;

static filterStatePt1_t servoFdbkFilter;

/**
 *
 *
 */
float filterApplyPt1(float input, filterStatePt1_t *filter, float f_cut, float dT) {

	// Pre calculate and store RC
	if (!filter->RC) {
		filter->RC = 1.0f / ( 2.0f * (float)PI * f_cut );
	}

    filter->state = filter->state + dT / (filter->RC + dT) * (input - filter->state);

    return filter->state;
}

/**
 *
 *
 */
void feedbackServoStep(TriflightSettingsData *triflightSettings,
                       TriflightStatusData   *triflightStatus,
                       float dT)
{
    bool  adc_min_lt_adc_max;
	bool  adc_in_lt_adc_mid;

	float adc_servo_fdbk;

	float adc_delta;
	float adc_range;
	float angle_range;
	float initial_angle;

	// Feedback servo
    float adc_servo_value = (float)PIOS_ADC_GetChannelRaw(triflightSettings->ServoFdbkPin);

	if (adc_servo_value == -1)
	{
		triflightStatus->ServoAngle = 90.0f;
		return;
	}

	adc_servo_fdbk = filterApplyPt1(adc_servo_value, &servoFdbkFilter, 70.0f, dT);

    triflightStatus->ADCServoFdbk = adc_servo_fdbk;

	adc_min_lt_adc_max = triflightSettings->AdcServoFdbkMin < triflightSettings->AdcServoFdbkMax;

	adc_in_lt_adc_mid = adc_servo_fdbk < triflightSettings->AdcServoFdbkMid;

	if ((adc_in_lt_adc_mid && adc_min_lt_adc_max) || (!adc_in_lt_adc_mid && !adc_min_lt_adc_max))
	{
		adc_delta     = adc_servo_fdbk - triflightSettings->AdcServoFdbkMin;
		adc_range     = triflightSettings->AdcServoFdbkMid - triflightSettings->AdcServoFdbkMin;
		angle_range   = triflightSettings->ServoMaxAngle;
		initial_angle = TRI_TAIL_SERVO_ANGLE_MID + triflightSettings->ServoMaxAngle;
	}
	else
	{
		adc_delta     = adc_servo_fdbk - triflightSettings->AdcServoFdbkMid;
		adc_range     = triflightSettings->AdcServoFdbkMax - triflightSettings->AdcServoFdbkMid;
		angle_range   = triflightSettings->ServoMaxAngle;
		initial_angle = TRI_TAIL_SERVO_ANGLE_MID;
	}

	triflightStatus->ServoAngle = initial_angle - adc_delta / adc_range * angle_range;

	return;
}
#endif

 /**
 *
 *
 */
float getServoAngle(ActuatorSettingsData  *actuatorSettings,
                    TriflightSettingsData *triflightSettings,
                    TriflightStatusData   *triflightStatus,
                    uint16_t servo_value)
{
    bool  pwm_min_lt_pwm_max;
	bool  pwm_in_lt_pwm_neutral;

	float pwm_delta;
	float pwm_range;
	float angle_range;
	float initial_angle;

	pwm_min_lt_pwm_max = actuatorSettings->ChannelMin[triflightStatus->ServoChannel] < actuatorSettings->ChannelMax[triflightStatus->ServoChannel];

	pwm_in_lt_pwm_neutral = servo_value < actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];

	if ((pwm_in_lt_pwm_neutral && pwm_min_lt_pwm_max) || (!pwm_in_lt_pwm_neutral && !pwm_min_lt_pwm_max))
	{
		pwm_delta     = servo_value - actuatorSettings->ChannelMin[triflightStatus->ServoChannel];
		pwm_range     = actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel] - actuatorSettings->ChannelMin[triflightStatus->ServoChannel];
		angle_range   = triflightSettings->ServoMaxAngle;
		initial_angle = TRI_TAIL_SERVO_ANGLE_MID + triflightSettings->ServoMaxAngle;
	}
	else
	{
		pwm_delta     = servo_value - actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];
		pwm_range     = actuatorSettings->ChannelMax[triflightStatus->ServoChannel] - actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];
		angle_range   = triflightSettings->ServoMaxAngle;
		initial_angle = TRI_TAIL_SERVO_ANGLE_MID;
	}

	return initial_angle - pwm_delta / pwm_range * angle_range;
}

/**
 *
 *
 */
float virtualServoStep(ActuatorSettingsData  *actuatorSettings,
                       TriflightSettingsData *triflightSettings,
                       TriflightStatusData   *triflightStatus,
                       float dT,
                       uint16_t servo_value)
{
    float angle_set_point = getServoAngle(actuatorSettings, triflightSettings, triflightStatus, servo_value);

	float dA = dT * triflightSettings->ServoSpeed; // Max change of an angle since last check

    if (fabsf(triflightStatus->ServoAngle - angle_set_point) < dA )
    {
        // At set-point after this moment
        triflightStatus->ServoAngle = angle_set_point;
    }
    else if (triflightStatus->ServoAngle < angle_set_point)
    {
        triflightStatus->ServoAngle += dA;
    }
    else  // current_angle > angle_set_point
    {
        triflightStatus->ServoAngle -= dA;
    }

    return triflightStatus->ServoAngle;
}

/**
 *
 *
 */
float getPitchCorrectionMaxPhaseShift(float servo_angle,
                                      float servo_setpoint_angle,
                                      float motor_acceleration_delay_angle,
                                      float motor_deceleration_delay_angle,
                                      float motor_direction_change_angle)
{
	float max_phase_shift;

	if (((servo_angle > servo_setpoint_angle) && (servo_angle >= (motor_direction_change_angle + motor_acceleration_delay_angle))) ||
	    ((servo_angle < servo_setpoint_angle) && (servo_angle <= (motor_direction_change_angle - motor_acceleration_delay_angle))))
	{
		// Motor is braking
		max_phase_shift = fabsf(servo_angle - motor_direction_change_angle) >= motor_deceleration_delay_angle ?
		                        motor_deceleration_delay_angle:
		                        fabsf(servo_angle - motor_direction_change_angle);
	}
	else
	{
		// Motor is accelerating
		max_phase_shift = motor_acceleration_delay_angle;
	}

	return max_phase_shift;
}

/**
 *
 *
 */
float triGetMotorCorrection(ActuatorSettingsData  *actuatorSettings,
                            TriflightSettingsData *triflightSettings,
                            TriflightStatusData   *triflightStatus,
                            uint16_t servo_value,
                            float throttle)
{
	// Adjust tail motor speed based on servo angle. Check how much to adjust speed from pitch force curve based on servo angle.
	// Take motor speed up lag into account by shifting the phase of the curve
	// Not taking into account the motor braking lag (yet)

	float servo_setpoint_angle = getServoAngle(actuatorSettings, triflightSettings, triflightStatus, servo_value); // commanded servo angle

	float max_phase_shift = getPitchCorrectionMaxPhaseShift(triflightStatus->ServoAngle,
	                                                        servo_setpoint_angle,
	                                                        tail_motor_acceleration_delay_angle,
	                                                        tail_motor_deceleration_delay_angle,
	                                                        tail_motor_pitch_zero_angle);

	float angle_diff = servo_setpoint_angle - triflightStatus->ServoAngle;

	if (fabsf(angle_diff) > max_phase_shift)
	{
		angle_diff = max_phase_shift * angle_diff / fabsf(angle_diff);
	}

	float future_servo_angle = bound_min_max(triflightStatus->ServoAngle + angle_diff,
	                                         TRI_TAIL_SERVO_ANGLE_MID - triflightSettings->ServoMaxAngle,
	                                         TRI_TAIL_SERVO_ANGLE_MID + triflightSettings->ServoMaxAngle);

	float correction = (throttle * getPitchCorrectionAtTailAngle(DEG2RAD * future_servo_angle, triflightSettings->MotorThrustFactor)) - throttle;

	return correction;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if !defined(ARCH_POSIX) && !defined(ARCH_WIN32)

#include "gyros.h"
#include "manualcontrolcommand.h"

#define TAIL_THRUST_FACTOR_MIN  (1.0f)
#define TAIL_THRUST_FACTOR_MAX  (40.0f)

typedef enum {
	TT_IDLE = 0,
	TT_WAIT,
	TT_ACTIVE,
	TT_WAIT_FOR_DISARM,
	TT_DONE,
	TT_FAIL,
} tailTuneState_e;

typedef enum {
	TT_MODE_NONE = 0,
	TT_MODE_THRUST_TORQUE,
	TT_MODE_SERVO_SETUP,
} tailtuneMode_e;

typedef enum {
	SS_IDLE = 0,
	SS_CALIB,
} servoSetupState_e;

typedef enum {
	SS_C_IDLE = 0,
	SS_C_CALIB_MIN_MID_MAX,
	SS_C_CALIB_SPEED,
} servoSetupCalibState_e;

typedef enum {
	SS_C_MIN = 0,
	SS_C_MID,
	SS_C_MAX,
} servoSetupCalibSubState_e;

typedef struct tailTune_s {
	tailtuneMode_e mode;

	struct thrustTorque_t
	{
		tailTuneState_e state;

		uint32_t startBeepDelay_us;
		uint32_t timestamp_us;

		uint32_t lastAdjTime_us;
		struct servoAvgAngle_t
		{
			float sum;
			float num_of;
		} servoAvgAngle;
		}tt;

	struct servoSetup_t
{
		servoSetupState_e state;

		float servoVal;

		struct servoCalib_t
		{
			bool done;
			bool waitingServoToStop;

			servoSetupCalibState_e state;
			servoSetupCalibSubState_e subState;

			uint32_t timestamp_us;

			struct average_t
			{
				float result;
				float sum;
				float num_of;
			} avg;
		}cal;
	}ss;
} tailTune_t;

static tailTune_t tailTune = {.mode = TT_MODE_NONE};

typedef enum {
	BUZZER_OFF = 0,
	BUZZER_ON,
} buzzerState_e;

typedef struct buzzer_s {
	buzzerState_e state;
	uint32_t duration;
} buzzer_t;

static buzzer_t buzzer = {.state = BUZZER_OFF, .duration = 0};

/**
 * @}
 * @}
 */
static void buzzerControl(ActuatorSettingsData  *actuatorSettings,
                          TriflightStatusData   *triflightStatus)
{
	static uint32_t buzzer_timestamp;
	static uint32_t duration;

	if (triflightStatus->BuzzerChannel == 255)
		return;

	if (buzzer.state == BUZZER_ON)
	{
		if (PIOS_DELAY_GetuSSince(buzzer_timestamp) >= duration)
		{
			PIOS_Servo_Set(triflightStatus->BuzzerChannel,
						   actuatorSettings->ChannelMin[triflightStatus->BuzzerChannel]);

			buzzer.state = BUZZER_OFF;
		}
	}

	if ((buzzer.state == BUZZER_OFF) && (buzzer.duration > 0))
	{
		buzzer_timestamp = PIOS_DELAY_GetuS();

		duration = buzzer.duration;

		buzzer.duration = 0;

		PIOS_Servo_Set(triflightStatus->BuzzerChannel,
		               actuatorSettings->ChannelMax[triflightStatus->BuzzerChannel]);

		buzzer.state = BUZZER_ON;
	}
}

/**
 * @}
 * @}
 */
static void tailTuneModeThrustTorque(TriflightSettingsData *triflightSettings,
                                     TriflightStatusData   *triflightStatus,
                                     struct thrustTorque_t *pTT,
                                     bool                  armed)
{
	ManualControlCommandData cmd;
	ManualControlCommandGet(&cmd);

	GyrosData gyrosData;
	GyrosGet(&gyrosData);

	bool is_throttle_high = cmd.Throttle > 0.1f;

	switch(pTT->state)
	{
		case TT_IDLE:
			// Calibration has been requested, only start when throttle is up
			if (is_throttle_high && armed)
			{
				buzzer.duration = 250000;
				pTT->startBeepDelay_us = 1000000;
				pTT->timestamp_us = PIOS_DELAY_GetuS();
				pTT->lastAdjTime_us = PIOS_DELAY_GetuS();
				pTT->state = TT_WAIT;
				pTT->servoAvgAngle.sum = 0;
				pTT->servoAvgAngle.num_of = 0;
			}

			break;

		case TT_WAIT:
			if (is_throttle_high && armed)
			{
				// Wait for 5 seconds before activating the tuning.
				// This is so that pilot has time to take off if the tail tune mode was activated on ground.
				if (PIOS_DELAY_GetuSSince(pTT->timestamp_us) >= 5000000)
				{
					// Longer beep when starting
					buzzer.duration = 500000;
					pTT->state = TT_ACTIVE;
					pTT->timestamp_us = PIOS_DELAY_GetuS();
				}
				else if (PIOS_DELAY_GetuSSince(pTT->timestamp_us) >= pTT->startBeepDelay_us)
				{
					// Beep every second until start
					buzzer.duration = 250000;
					pTT->startBeepDelay_us += 1000000;
				}
			}
			else
			{
				pTT->state = TT_IDLE;
			}

			break;

		case TT_ACTIVE:
			if (is_throttle_high &&
			   (fabsf(cmd.Roll)    <= 0.05f) &&
			   (fabsf(cmd.Pitch)   <= 0.05f) &&
			   (fabsf(cmd.Yaw)     <= 0.05f) &&
			   (fabsf(gyrosData.z) <= 45.0f))    // This value needs to be refined, 45 DPS seems too high - JI
			{
				if (PIOS_DELAY_GetuSSince(pTT->timestamp_us) >= 250000)
				{
					// RC commands have been within deadbands for 250 ms
					if (PIOS_DELAY_GetuSSince(pTT->lastAdjTime_us) >= 10000)
					{
						pTT->lastAdjTime_us = PIOS_DELAY_GetuS();

						pTT->servoAvgAngle.sum += triflightStatus->ServoAngle;
						pTT->servoAvgAngle.num_of++;

						triflightStatus->testFloat1 = pTT->servoAvgAngle.sum;
						triflightStatus->testFloat2 = pTT->servoAvgAngle.num_of;

						if (pTT->servoAvgAngle.num_of >= 300)
						{
							buzzer.duration = 500000;
							pTT->state = TT_WAIT_FOR_DISARM;
							pTT->timestamp_us = PIOS_DELAY_GetuS();
						}
					}
				}
			}
			else
			{
				pTT->timestamp_us = PIOS_DELAY_GetuS();
			}

			break;

		case TT_WAIT_FOR_DISARM:
			if (!armed)
			{
				float average_servo_angle = pTT->servoAvgAngle.sum / pTT->servoAvgAngle.num_of;

				// Find out the factor that gives least yaw force at the average angle
				float factor = TAIL_THRUST_FACTOR_MIN;

				float angleRad = DEG2RAD * average_servo_angle;

				float minAbsForce = 10000.0f; //FLT_MAX;

				float minFactor = TAIL_THRUST_FACTOR_MIN;

				bool done = false;

				for (factor = TAIL_THRUST_FACTOR_MIN; (done == false) && (factor < TAIL_THRUST_FACTOR_MAX); factor += 0.01f)
				{
					float absForceAtAngle = fabsf(-factor * cosf(angleRad) - sinf(angleRad)) * getPitchCorrectionAtTailAngle(angleRad, factor);

					if (absForceAtAngle < minAbsForce)
					{
						minAbsForce = absForceAtAngle;
						minFactor = factor;
					}
					else
					{
						done = true;
					}
				}

				if (done)
				{
					triflightSettings->MotorThrustFactor = minFactor;

					TriflightSettingsSet(triflightSettings);
					UAVObjSave(TriflightSettingsHandle(), 0);

					pTT->state = TT_DONE;
				}
				else
				{
					pTT->state = TT_FAIL;
				}
				pTT->timestamp_us = PIOS_DELAY_GetuS();
			}
			else
			{
				if (PIOS_DELAY_GetuSSince(pTT->timestamp_us) >= 2000000)
				{
					buzzer.duration = 250000;
					pTT->timestamp_us = PIOS_DELAY_GetuS();
				}
			}

			break;

		case TT_DONE:
			if (PIOS_DELAY_GetuSSince(pTT->timestamp_us) >= 2000000)
			{
				buzzer.duration = 250000;
				pTT->timestamp_us = PIOS_DELAY_GetuS();
			}

			break;

		case TT_FAIL:
			if (PIOS_DELAY_GetuSSince(pTT->timestamp_us) >= 2000000)
			{
				buzzer.duration = 750000;
				pTT->timestamp_us = PIOS_DELAY_GetuS();
			}

			break;
    }
}

/**
 * @}
 * @}
 */
static void tailTuneModeServoSetup(ActuatorSettingsData  *actuatorSettings,
                                   TriflightSettingsData *triflightSettings,
                                   TriflightStatusData   *triflightStatus,
                                   struct servoSetup_t   *pSS,
                                   float                 *pServoVal,
                                   float                 dT)
{
	static uint32_t sample_delta;

	bool adc_min_lt_adc_max;

	ManualControlCommandData cmd;
	ManualControlCommandGet(&cmd);

	// Check mode select
	if ((cmd.Roll < 0.05f) && (cmd.Roll > -0.05f) && (cmd.Pitch > 0.5f))
	{
		pSS->state = SS_CALIB;
		pSS->cal.state = SS_C_IDLE;
	}

	switch (pSS->state)
	{
		case SS_IDLE:
			break;

		case SS_CALIB:
			// State transition
			if ((pSS->cal.done == true) || (pSS->cal.state == SS_C_IDLE))
			{
				if (pSS->cal.state == SS_C_IDLE)
				{
					pSS->cal.state            = SS_C_CALIB_MIN_MID_MAX;
					pSS->cal.subState         = SS_C_MIN;
					pSS->servoVal             = actuatorSettings->ChannelMin[triflightStatus->ServoChannel];
				    sample_delta              = 0;
				}
				else if (pSS->cal.state == SS_C_CALIB_SPEED)
				{
					pSS->state        = SS_IDLE;
					pSS->cal.subState = SS_C_IDLE;
				}
				else
				{
					if (pSS->cal.state == SS_C_CALIB_MIN_MID_MAX)
					{
						switch (pSS->cal.subState)
						{
							case SS_C_MIN:
								triflightSettings->AdcServoFdbkMin = pSS->cal.avg.result;

								pSS->cal.subState         = SS_C_MID;
								pSS->servoVal             = actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];
								sample_delta              = 0;
								break;

							case SS_C_MID:
								if (fabs(triflightSettings->AdcServoFdbkMin - triflightSettings->AdcServoFdbkMid) < 100)
								{
									/* Not enough difference between min and mid feedback values.
									* Most likely the feedback signal is not connected.
									*/
									pSS->state        = SS_IDLE;
									pSS->cal.subState = SS_C_IDLE;
								}
								else
								{
									triflightSettings->AdcServoFdbkMid = pSS->cal.avg.result;

									pSS->cal.subState = SS_C_MAX;
									pSS->servoVal     = actuatorSettings->ChannelMax[triflightStatus->ServoChannel];
									sample_delta      = 0;
								}
								break;

							case SS_C_MAX:
								triflightSettings->AdcServoFdbkMax = pSS->cal.avg.result;

								pSS->cal.state    = SS_C_CALIB_SPEED;
								pSS->cal.subState = SS_C_MIN;

								pSS->servoVal               = actuatorSettings->ChannelMin[triflightStatus->ServoChannel];
								pSS->cal.waitingServoToStop = true;
								break;
						}
					}
				}

				pSS->cal.timestamp_us = PIOS_DELAY_GetuS();
				pSS->cal.avg.sum      = 0.0f;
				pSS->cal.avg.num_of   = 0.0f;
				pSS->cal.done         = false;
			}

			switch (pSS->cal.state)
			{
				case SS_C_IDLE:
					break;

				case SS_C_CALIB_MIN_MID_MAX:
					if (PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us) >= 500000)
					{
						if ((PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us) >= 500000 + sample_delta) && (sample_delta < 1e6))
						{
							pSS->cal.avg.sum += triflightStatus->ADCServoFdbk;
							pSS->cal.avg.num_of++;
							sample_delta += 10000;
						}

						if (sample_delta >= 1e6)
						{
							pSS->cal.avg.result = pSS->cal.avg.sum / pSS->cal.avg.num_of;
							pSS->cal.done = true;
						}
					}
					break;


				case SS_C_CALIB_SPEED:
					switch (pSS->cal.subState)
					{
						case SS_C_MIN:
							// Wait for the servo to reach min position
							adc_min_lt_adc_max = triflightSettings->AdcServoFdbkMin < triflightSettings->AdcServoFdbkMax;

							if (((triflightStatus->ADCServoFdbk < (triflightSettings->AdcServoFdbkMin + 10.0f)) &&  adc_min_lt_adc_max) ||
							    ((triflightStatus->ADCServoFdbk > (triflightSettings->AdcServoFdbkMin - 10.0f)) && !adc_min_lt_adc_max))
							{
								if (!pSS->cal.waitingServoToStop)
								{
									pSS->cal.avg.sum += PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us);
									pSS->cal.avg.num_of++;

									if (pSS->cal.avg.num_of > 5)
									{
										float avgTime = (float)pSS->cal.avg.sum / (float)pSS->cal.avg.num_of / 1e6f;
										float avgServoSpeed = (2.0f * triflightSettings->ServoMaxAngle) / avgTime;
										triflightSettings->ServoSpeed = avgServoSpeed;
										pSS->cal.done = true;
										pSS->servoVal = actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];
										TriflightSettingsSet(triflightSettings);
										UAVObjSave(TriflightSettingsHandle(), 0);
									}

									pSS->cal.timestamp_us = PIOS_DELAY_GetuS();
									pSS->cal.waitingServoToStop = true;
								}
								// Wait for the servo to fully stop before starting speed measuring
								else if  (PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us) >= 200000)
								{
									pSS->cal.timestamp_us = PIOS_DELAY_GetuS();
									pSS->cal.subState = SS_C_MAX;
									pSS->cal.waitingServoToStop = false;
									pSS->servoVal = actuatorSettings->ChannelMax[triflightStatus->ServoChannel];
								}
							}
							break;

						case SS_C_MAX:
							// Wait for the servo to reach max position
							adc_min_lt_adc_max = triflightSettings->AdcServoFdbkMin < triflightSettings->AdcServoFdbkMax;

							if (((triflightStatus->ADCServoFdbk > (triflightSettings->AdcServoFdbkMax - 10.0f)) &&  adc_min_lt_adc_max) ||
							    ((triflightStatus->ADCServoFdbk < (triflightSettings->AdcServoFdbkMax + 10.0f)) && !adc_min_lt_adc_max))
							{
								if (!pSS->cal.waitingServoToStop)
								{
									pSS->cal.avg.sum += PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us);
									pSS->cal.avg.num_of++;
									pSS->cal.timestamp_us = PIOS_DELAY_GetuS();
									pSS->cal.waitingServoToStop = true;
								}
								else if (PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us) >= 200000)
								{
									pSS->cal.timestamp_us = PIOS_DELAY_GetuS();
									pSS->cal.subState = SS_C_MIN;
									pSS->cal.waitingServoToStop = false;
									pSS->servoVal = actuatorSettings->ChannelMin[triflightStatus->ServoChannel];
								}
							}
							break;

						case SS_C_MID:
							// Should not come here
							break;
					}
			}
			break;
	}

	*pServoVal = pSS->servoVal;
}

 /**
 * @}
 * @}
 */
void triTailTuneStep(ActuatorSettingsData  *actuatorSettings,
                     FlightStatusData      *flightStatus,
                     TriflightSettingsData *triflightSettings,
                     TriflightStatusData   *triflightStatus,
                     float                 *pServoVal,
                     bool armed,
                     float dT)
{
	ManualControlCommandData cmd;
	ManualControlCommandGet(&cmd);

	if (flightStatus->FlightMode == FLIGHTSTATUS_FLIGHTMODE_TAILTUNE)
	{
		if (tailTune.mode == TT_MODE_NONE)
		{
			if (armed)
			{
				tailTune.mode = TT_MODE_THRUST_TORQUE;
				tailTune.tt.state = TT_IDLE;
			}
			else
			{
				tailTune.mode = TT_MODE_SERVO_SETUP;
				tailTune.ss.servoVal = actuatorSettings->ChannelNeutral[triflightStatus->ServoChannel];
			}
		}
	}
	else
	{
		tailTune.mode = TT_MODE_NONE;
		return;
	}

	switch (tailTune.mode)
	{
		case TT_MODE_NONE:
			break;

		case TT_MODE_THRUST_TORQUE:
			tailTuneModeThrustTorque(triflightSettings,
			                         triflightStatus,
			                         &tailTune.tt,
			                         armed);
			break;

		case TT_MODE_SERVO_SETUP:
			tailTuneModeServoSetup(actuatorSettings,
			                       triflightSettings,
			                       triflightStatus,
			                       &tailTune.ss,
			                       pServoVal,
			                       dT);
			break;
    }

	buzzerControl(actuatorSettings, triflightStatus);
}

#endif

/**
 * @}
 * @}
 */

#endif // ifndef SMALLF1