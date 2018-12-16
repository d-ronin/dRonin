/**
 ******************************************************************************
 * @addtogroup BiflightModule Biflight Module
 * @{
 *
 * @file       biflight.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @brief      Bifight module. Specialty mixing for bicopters.
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

#include "openpilot.h"
#include "biflight.h"
#include "misc_math.h"
#include "physical_constants.h"

#include "pios_board.h"

#define BI_SERVO_ANGLE_MID 0

extern char *xflight_blink_string;

/**
 *
 *
 */
void biflightInit(void)
{
	// TBD
}

/**
 *
 *
 */
typedef struct filterStatePt1_s {
	float state;
	float RC;
} filterStatePt1_t;

static float filterApplyPt1(float input, filterStatePt1_t *filter, float f_cut, float dT) {

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
static filterStatePt1_t servoFdbkFilter[2];

void biServoStep(BiflightSettingsData *biflightSettings,
                 BiflightStatusData   *biflightStatus,
                 float dT)
{
    bool  adc_min_lt_adc_max;
	bool  adc_in_lt_adc_mid;

	float adc_servo_fdbk;

	float adc_delta;
	float adc_range;
	float angle_range;
	float initial_angle;

	// Servo Feedback
    for (uint8_t side = 0; side < 2; side++)
	{	
		float adc_servo_value = (float)PIOS_ADC_GetChannelRaw(biflightSettings->ServoFdbkPin[side]);

		if (adc_servo_value == -1) {
			biflightStatus->ServoAngle[side] = 0.0f;
		} else {
			adc_servo_fdbk = filterApplyPt1(adc_servo_value, &servoFdbkFilter[side], 70.0f, dT);

			biflightStatus->ADCServoFdbk[side] = adc_servo_fdbk;

			adc_min_lt_adc_max = biflightSettings->AdcServoFdbkMin[side] < biflightSettings->AdcServoFdbkMax[side];

			adc_in_lt_adc_mid = adc_servo_fdbk < biflightSettings->AdcServoFdbkMid[side];

			if ((adc_in_lt_adc_mid && adc_min_lt_adc_max) || (!adc_in_lt_adc_mid && !adc_min_lt_adc_max))
			{
				adc_delta     = adc_servo_fdbk - biflightSettings->AdcServoFdbkMin[side];
				adc_range     = biflightSettings->AdcServoFdbkMid[side] - biflightSettings->AdcServoFdbkMin[side];
				angle_range   = biflightSettings->ServoMaxAngle;
				initial_angle = BI_SERVO_ANGLE_MID + biflightSettings->ServoMaxAngle;
			}
			else
			{
				adc_delta     = adc_servo_fdbk - biflightSettings->AdcServoFdbkMid[side];
				adc_range     = biflightSettings->AdcServoFdbkMax[side] - biflightSettings->AdcServoFdbkMid[side];
				angle_range   = biflightSettings->ServoMaxAngle;
				initial_angle = BI_SERVO_ANGLE_MID;
			}

			biflightStatus->ServoAngle[side] = adc_delta / adc_range * angle_range - initial_angle;
		}
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "manualcontrolcommand.h"

typedef enum {
	SC_MODE_NONE = 0,
	SC_MODE_SERVO_SETUP,
} servoCalibrationMode_e;

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
	SS_C_S_IDLE = 0,
	SS_C_S_MIN,
	SS_C_S_MID,
	SS_C_S_MAX,
} servoSetupCalibSubState_e;

typedef struct servoCalibration_s {
	servoCalibrationMode_e mode;

	struct servoSetup_t
	{
		servoSetupState_e state;

		float servoVal;
		uint16_t *pLimitToAdjust;

		struct servoCalib_t
		{
			bool done;
			bool waitingServoToStop;

			servoSetupCalibState_e state;
			servoSetupCalibSubState_e subState;

			uint32_t timestamp_us;

			struct average_t
			{
				float    result;
				float    sum;
				uint16_t num_of;
			} avg;
		}cal;
	} ss;
} servoCalibration_t;

/**
 * @}
 * @}
 */
static void servoCalibrationSetup(ActuatorSettingsData *actuatorSettings,
                                  BiflightSettingsData *biflightSettings,
                                  BiflightStatusData   *biflightStatus,
                                  struct servoSetup_t  *pSS,
                                  float                *pLeftServoVal,
								  float                *pRightServoVal,
                                  float                dT)
{
	static uint32_t sample_delta;

	bool adc_min_lt_adc_max;

	ManualControlCommandData cmd;
	ManualControlCommandGet(&cmd);

	static uint8_t side;
	
	if ((cmd.Pitch < 0.05f) && (cmd.Pitch > -0.05f) && (cmd.Roll < -0.5f) && (pSS->state == SS_IDLE)) {
		pSS->state = SS_CALIB;
		pSS->cal.state = SS_C_IDLE;
		
		pSS->cal.avg.sum    = 0.0f;
		pSS->cal.avg.num_of = 0;
		pSS->cal.done       = false;
				
		side = LEFT;
	}

	if ((cmd.Pitch < 0.05f) && (cmd.Pitch > -0.05f) && (cmd.Roll > 0.5f) && (pSS->state == SS_IDLE)) {
		pSS->state = SS_CALIB;
		pSS->cal.state = SS_C_IDLE;
		
		pSS->cal.avg.sum    = 0.0f;
		pSS->cal.avg.num_of = 0;
		pSS->cal.done       = false;
				
		side = RIGHT;
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
					pSS->cal.state    = SS_C_CALIB_MIN_MID_MAX;
					pSS->cal.subState = SS_C_S_MIN;
					pSS->servoVal     = actuatorSettings->ChannelMin[biflightStatus->ServoChannel[side]];
				    sample_delta      = 0;
				}
				else if (pSS->cal.state == SS_C_CALIB_SPEED)
				{
					pSS->state        = SS_IDLE;
					pSS->cal.subState = SS_C_S_IDLE;
				}
				else
				{
					if (pSS->cal.state == SS_C_CALIB_MIN_MID_MAX)
					{
						switch (pSS->cal.subState)
						{
							case SS_C_S_MIN:
								biflightSettings->AdcServoFdbkMin[side] = pSS->cal.avg.result;

								pSS->cal.subState = SS_C_S_MID;
								pSS->servoVal     = actuatorSettings->ChannelNeutral[biflightStatus->ServoChannel[side]];
								sample_delta      = 0;
								break;

							case SS_C_S_MID:
								biflightSettings->AdcServoFdbkMid[side] = pSS->cal.avg.result;

								if (fabs(biflightSettings->AdcServoFdbkMin[side] - biflightSettings->AdcServoFdbkMid[side]) < 100)
								{
									// Not enough difference between min and mid feedback values.
									// Most likely the feedback signal is not connected.

									pSS->state        = SS_IDLE;
									pSS->cal.subState = SS_C_S_IDLE;

									xflight_blink_string = "G";  // "--." done fail
								}
								else
								{
									pSS->cal.subState = SS_C_S_MAX;
									pSS->servoVal     = actuatorSettings->ChannelMax[biflightStatus->ServoChannel[side]];
									sample_delta      = 0;
								}
								break;

							case SS_C_S_MAX:
								biflightSettings->AdcServoFdbkMax[side] = pSS->cal.avg.result;

								pSS->cal.state    = SS_C_CALIB_SPEED;
								pSS->cal.subState = SS_C_S_MIN;

								pSS->servoVal               = actuatorSettings->ChannelMin[biflightStatus->ServoChannel[side]];
								pSS->cal.waitingServoToStop = false;
								break;

							case SS_C_S_IDLE:
								break;
						}
					}
				}

				pSS->cal.timestamp_us = PIOS_DELAY_GetuS();
				pSS->cal.avg.sum      = 0.0f;
				pSS->cal.avg.num_of   = 0;
				pSS->cal.done         = false;
			}

			switch (pSS->cal.state)
			{
				case SS_C_IDLE:
					break;

				case SS_C_CALIB_MIN_MID_MAX:
					if (PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us) >= 500000)  // 0.5 seconds
					{
						if ((PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us) >= 500000 + sample_delta) && (sample_delta < 1e6))
						{
							pSS->cal.avg.sum += biflightStatus->ADCServoFdbk[side];
							pSS->cal.avg.num_of++;
							sample_delta += 10000;  // 0.01 seconds
						}

						if (sample_delta >= 1e6)  // 1 second
						{
							pSS->cal.avg.result = pSS->cal.avg.sum / (float)pSS->cal.avg.num_of;
							pSS->cal.done = true;
						}
					}
					break;


				case SS_C_CALIB_SPEED:
					switch (pSS->cal.subState)
					{
						case SS_C_S_MIN:
							// Wait for the servo to reach min position
							adc_min_lt_adc_max = biflightSettings->AdcServoFdbkMin[side] < biflightSettings->AdcServoFdbkMax[side];

							if (((biflightStatus->ADCServoFdbk[side] < (biflightSettings->AdcServoFdbkMin[side] + 10.0f)) &&  adc_min_lt_adc_max) ||
							    ((biflightStatus->ADCServoFdbk[side] > (biflightSettings->AdcServoFdbkMin[side] - 10.0f)) && !adc_min_lt_adc_max))
							{
								if (!pSS->cal.waitingServoToStop)
								{
									pSS->cal.avg.sum += PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us);
									pSS->cal.avg.num_of++;

									if (pSS->cal.avg.num_of >= 5)
									{
										float avgTime = (float)pSS->cal.avg.sum / (float)pSS->cal.avg.num_of / 1e6f;
										float avgServoSpeed = (2.0f * biflightSettings->ServoMaxAngle) / avgTime;
										biflightSettings->ServoSpeed[side] = avgServoSpeed;

										pSS->cal.done = true;
										pSS->servoVal = actuatorSettings->ChannelNeutral[biflightStatus->ServoChannel[side]];

										ActuatorSettingsSet(actuatorSettings);
										UAVObjSave(ActuatorSettingsHandle(), 0);

										BiflightSettingsSet(biflightSettings);
										UAVObjSave(BiflightSettingsHandle(), 0);

										xflight_blink_string = "W";  // ".--" done success
									}

									pSS->cal.timestamp_us = PIOS_DELAY_GetuS();
									pSS->cal.waitingServoToStop = true;
								}
								// Wait for the servo to fully stop before starting speed measuring
								else if  (PIOS_DELAY_GetuSSince(pSS->cal.timestamp_us) >= 200000)
								{
									pSS->cal.timestamp_us = PIOS_DELAY_GetuS();
									pSS->cal.subState = SS_C_S_MAX;
									pSS->cal.waitingServoToStop = false;
									pSS->servoVal = actuatorSettings->ChannelMax[biflightStatus->ServoChannel[side]];
								}
							}
							break;

						case SS_C_S_MAX:
							// Wait for the servo to reach max position
							adc_min_lt_adc_max = biflightSettings->AdcServoFdbkMin[side] < biflightSettings->AdcServoFdbkMax[side];

							if (((biflightStatus->ADCServoFdbk[side] > (biflightSettings->AdcServoFdbkMax[side] - 10.0f)) &&  adc_min_lt_adc_max) ||
							    ((biflightStatus->ADCServoFdbk[side] < (biflightSettings->AdcServoFdbkMax[side] + 10.0f)) && !adc_min_lt_adc_max))
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
									pSS->cal.subState = SS_C_S_MIN;
									pSS->cal.waitingServoToStop = false;
									pSS->servoVal = actuatorSettings->ChannelMin[biflightStatus->ServoChannel[side]];
								}
							}
							break;

						case SS_C_S_IDLE:
						case SS_C_S_MID:
							// Should not come here
							break;
					}
			}
			break;
	}

	if (side == LEFT) {
		*pLeftServoVal  = pSS->servoVal;
		*pRightServoVal = actuatorSettings->ChannelNeutral[biflightStatus->ServoChannel[RIGHT]];
	} else {
		*pLeftServoVal  = actuatorSettings->ChannelNeutral[biflightStatus->ServoChannel[LEFT]];
		*pRightServoVal = pSS->servoVal;
	}
}

 /**
 * @}
 * @}
 */
static servoCalibration_t servoCalibration = {.mode = SC_MODE_NONE};

void biServoCalibrateStep(ActuatorSettingsData *actuatorSettings,
                          FlightStatusData     *flightStatus,
                          BiflightSettingsData *biflightSettings,
                          BiflightStatusData   *biflightStatus,
                          float                *pLeftServoVal,
						  float                *pRightServoVal,
                          bool armed,
                          float dT)
{
	if (flightStatus->FlightMode == FLIGHTSTATUS_FLIGHTMODE_SERVOCAL)
	{
		if (servoCalibration.mode == SC_MODE_NONE)
		{
			if (armed)
			{
				// TBD
			}
			else
			{
				servoCalibration.mode = SC_MODE_SERVO_SETUP;
			}
		}
	}
	else
	{
		servoCalibration.mode = SC_MODE_NONE;
		xflight_blink_string = NULL;
		return;
	}

	switch (servoCalibration.mode)
	{
		case SC_MODE_NONE:
			break;

		case SC_MODE_SERVO_SETUP:
			servoCalibrationSetup(actuatorSettings,
			                      biflightSettings,
			                      biflightStatus,
			                      &servoCalibration.ss,
			                      pLeftServoVal,
								  pRightServoVal,
			                      dT);
			break;
    }
}

/**
 * @}
 * @}
 */
