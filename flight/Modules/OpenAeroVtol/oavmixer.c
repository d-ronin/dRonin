/////////////////////////////////////////////////////////////////////
/*
   Modified for inclusion in Dronin, J. Ihlein Feb 2017

   Copyright (c) 2016, David Thompson
   All rights reserved.

   OpenAero-VTOL (OAV) firmware is intended for use in radio controlled model aircraft. It provides flight
   control and stabilization as needed by Vertical Takeoff and Landing (VTOL) aircraft when used in
   combination with commercially available KK2 Flight Controller hardware. It is provided at no cost and no
   contractual obligation is created by its use.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided
   that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of conditions and
      the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
      and the following disclaimer in the documentation and/or other materials provided with the
      distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
   EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
   THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   See the GNU general Public License for more details at:

   <http://www.gnu.org/licenses/>
*/
/////////////////////////////////////////////////////////////////////

#include "pios_config.h"
#include "openpilot.h"

#include "oavtypedefs.h"

#include "manualcontrolcommand.h"
#include "mixersettings.h"
#include "oav.h"
#include "oavcurves.h"
#include "oavmixerfdbks.h"
#include "oavmixervolumes.h"
#include "oavoffsets.h"
#include "oavsettings.h"
#include "oavstatus.h"

/////////////////////////////////////////////////////////////////////

int16_t processCurve(uint8_t curve, uint8_t type, int16_t inputValue, OAVCurvesData *OAVCurves, OAVOffsetsData * OAVOffsets);
int16_t scale32(int16_t value16, int16_t multiplier16);
static MixerSettingsMixer1TypeOptions get_mix_type(MixerSettingsData *mixerSettings, int idx);

// Throttle volume curves
// Why 101 steps? Well, both 0% and 100% transition values are valid...

const int8_t SIN[101] =
			{ 0,  2,  3,  5,   6,   8,  10,  11,  13,  14,
			 16, 17, 19, 20,  22,  23,  25,  26,  28,  29,
			 31, 32, 34, 35,  37,  38,  40,  41,  43,  44,
			 45, 47, 48, 50,  51,  52,  54,  55,  56,  58,
			 59, 60, 61, 63,  64,  65,  66,  67,  68,  70,
			 71, 72, 73, 74,  75,  76,  77,  78,  79,  80,
			 81, 82, 83, 84,  84,  85,  86,  87,  88,  88,
			 89, 90, 90, 91,  92,  92,  93,  94,  94,  95,
			 95, 96, 96, 96,  97,  97,  98,  98,  98,  99,
			 99, 99, 99, 99, 100, 100, 100, 100, 100, 100,
			100};

const int8_t SQRTSIN[101] =
			{ 0, 13,  18,  22,  25,  28,  31,  33,  35,  38,
			 40, 41,  43,  45,  47,  48,  50,  51,  53,  54,
			 56, 57,  58,  59,  61,  62,  63,  64,  65,  66,
			 67, 68,  69,  70,  71,  72,  73,  74,  75,  76,
			 77, 77,  78,  79,  80,  81,  81,  82,  83,  83,
			 84, 85,  85,  86,  87,  87,  88,  88,  89,  89,
			 90, 90,  91,  91,  92,  92,  93,  93,  94,  94,
			 94, 95,  95,  95,  96,  96,  96,  97,  97,  97,
			 98, 98,  98,  98,  98,  99,  99,  99,  99,  99,
			 99, 99, 100, 100, 100, 100, 100, 100, 100, 100,
			100};

/////////////////////////////////////////////////////////////////////

void ProcessMixer(OAVCurvesData       *OAVCurves,
                  OAVMixerFdbksData   *OAVMixerFdbks,
				  OAVMixerVolumesData *OAVMixerVolumes,
                  OAVOffsetsData      *OAVOffsets,
                  OAVSettingsData     *OAVSettings,
                  OAVStatusData       *OAVStatus,
                  MixerSettingsData   *MixerSettings)
{
	uint8_t i = 0;
	int16_t p1_solution = 0;
	int16_t p2_solution = 0;

	int16_t temp1 = 0;
	int16_t temp2 = 0;
	int16_t	temp3 = 0;
	int16_t	step1 = 0;

	int16_t mono_throttle = 0;

	int32_t e32temp1 = 0;
	int32_t e32temp2 = 0;
	int32_t e32temp3 = 0;
	int32_t	e32step1 = 0;

	// Curve outputs
	int16_t	p1_throttle   = 0;  // P1 Throttle curve
	int16_t	p2_throttle   = 0;  // P2 Throttle curve
	int16_t	p1_collective = 0;  // P1 Collective curve
	int16_t	p2_collective = 0;  // P2 Collective curve

	int16_t	curve_c = 0;  // Generic Curve C
	int16_t	curve_d = 0;  // Generic Curve D

	int8_t	p1_accel_roll_volume_source = 0;
	int8_t	p1_gyro_roll_volume_source  = 0;
	int8_t	p1_gyro_yaw_volume_source   = 0;

	// Process curves
	p1_throttle   = processCurve(P1THRCURVE,  MONOPOLAR, monopolar_throttle,  OAVCurves, OAVOffsets);
	p2_throttle   = processCurve(P2THRCURVE,  MONOPOLAR, monopolar_throttle,  OAVCurves, OAVOffsets);
	p1_collective = processCurve(P1COLLCURVE, BIPOLAR,   rc_inputs[THROTTLE], OAVCurves, OAVOffsets);
	p2_collective = processCurve(P2COLLCURVE, BIPOLAR,   rc_inputs[THROTTLE], OAVCurves, OAVOffsets);

	int16_t	curve_source[NUMBEROFCURVESOURCES] =
		{rc_inputs[THROTTLE], rc_inputs[AILERON], rc_inputs[ELEVATOR], rc_inputs[RUDDER], rc_inputs[GEAR],
		 rc_inputs[AUX1], rc_inputs[AUX2], rc_inputs[AUX3], pid_gyros[P2][ROLL], pid_gyros[P2][PITCH],
		 pid_gyros[P2][YAW], temp1, temp2, pid_accels[P2][ROLL], pid_accels[P2][PITCH], 0};

	 // Only process generic curves if they have a source selected
	if (OAVCurves->CurveSource[GENCURVEC] != OAVCURVES_CURVESOURCE_NOSOURCE)
		curve_c = processCurve(GENCURVEC, BIPOLAR, curve_source[OAVCurves->CurveSource[GENCURVEC]], OAVCurves, OAVOffsets);
	else
		curve_c = 0;

	if (OAVCurves->CurveSource[GENCURVED] != OAVCURVES_CURVESOURCE_NOSOURCE)
		curve_d = processCurve(GENCURVED, BIPOLAR, curve_source[OAVCurves->CurveSource[GENCURVED]], OAVCurves, OAVOffsets);
	else
		curve_d = 0;

	// Copy the universal mixer inputs to an array for easy indexing - acc data is from acc_smooth, increased to reasonable rates
	temp1 = (int16_t)accel_smooth[ROLL]  << 3;
	temp2 = (int16_t)accel_smooth[PITCH] << 3;

	// THROTTLE, CURVE A, CURVE B, COLLECTIVE, THROTTLE, AILERON, ELEVATOR, RUDDER, GEAR, AUX1,
	// AUX2, AUX3, ROLLGYRO, PITCHGYO, YAWGYRO, ACCSMOOTH, PITCHSMOOTH, ROLLACC, PITCHACC, AccZ,
	// NONE
	int16_t	source_select_p1[NUMBEROFSOURCES] =
		{p1_throttle, p1_collective, curve_c, curve_d, rc_inputs[THROTTLE], rc_inputs[AILERON],
		 rc_inputs[ELEVATOR], rc_inputs[RUDDER], rc_inputs[GEAR], rc_inputs[AUX1], rc_inputs[AUX2],
		 rc_inputs[AUX3], pid_gyros[P1][ROLL], pid_gyros[P1][PITCH], pid_gyros[P1][YAW], temp1,
		 temp2, pid_accels[P1][ROLL], pid_accels[P1][PITCH], pid_accels[P1][YAW], 0};

	int16_t	source_select_p2[NUMBEROFSOURCES] =
		{p2_throttle, p2_collective, curve_c, curve_d, rc_inputs[THROTTLE], rc_inputs[AILERON],
		 rc_inputs[ELEVATOR], rc_inputs[RUDDER], rc_inputs[GEAR], rc_inputs[AUX1], rc_inputs[AUX2],
		 rc_inputs[AUX3], pid_gyros[P2][ROLL], pid_gyros[P2][PITCH], pid_gyros[P2][YAW], temp1,
		 temp2, pid_accels[P2][ROLL], pid_accels[P2][PITCH], pid_accels[P2][YAW], 0};

	/////////////////////////////////////////////////////////////////
	// Main mix loop - sensors, RC inputs and other channels
	/////////////////////////////////////////////////////////////////

	for (i = 0; i < MIXOUTPUTS; i++)
	{
		/////////////////////////////////////////////////////////////
		// Zero each channel value to start
		/////////////////////////////////////////////////////////////

		p1_solution = 0;
		p2_solution = 0;

		/////////////////////////////////////////////////////////////
		// Mix in gyros
		/////////////////////////////////////////////////////////////

		// If the user wants earth reference for tail-sitter hover, swap the related stick sources.
		// The secret is understanding WHICH STICK is controlling movement on the AXIS in the selected REFERENCE
		// Only need to do this if the orientations differ
		if (OAVSettings->P1Reference != OAVSETTINGS_P1REFERENCE_NO_ORIENT)
		{
			// EARTH-Referenced tail-sitter
			if (OAVSettings->P1Reference == OAVSETTINGS_P1REFERENCE_EARTH)
			{
				p1_accel_roll_volume_source = OAVMixerVolumes->P1AileronVolume[i];
				p1_gyro_roll_volume_source  = OAVMixerVolumes->P1AileronVolume[i];  // These are always the same
				p1_gyro_yaw_volume_source   = OAVMixerVolumes->P1RudderVolume[i];   // These are always the same
			}
			// MODEL-Referenced tail-sitter
			else
			{
				p1_accel_roll_volume_source = OAVMixerVolumes->P1RudderVolume[i];
				p1_gyro_roll_volume_source  = OAVMixerVolumes->P1AileronVolume[i];
				p1_gyro_yaw_volume_source   = OAVMixerVolumes->P1RudderVolume[i];
			}
		}
		// Normal case
		else
		{
			p1_accel_roll_volume_source = OAVMixerVolumes->P1AileronVolume[i];
			p1_gyro_roll_volume_source  = OAVMixerVolumes->P1AileronVolume[i];
			p1_gyro_yaw_volume_source   = OAVMixerVolumes->P1RudderVolume[i];
		}

		// P1 gyros
		if (OAVStatus->OAVTransition < 100)
		{
			switch (OAVMixerFdbks->P1RollGyro[i])
			{
				case OAVMIXERFDBKS_P1ROLLGYRO_OFF:
					break;
				case OAVMIXERFDBKS_P1ROLLGYRO_ON:
					if (p1_gyro_roll_volume_source < 0 )
					{
						p1_solution = p1_solution + pid_gyros[P1][ROLL];  // Reverse if volume negative
					}
					else
					{
						p1_solution = p1_solution - pid_gyros[P1][ROLL];
					}
					break;
				case OAVMIXERFDBKS_P1ROLLGYRO_SCALEX5:
					p1_solution = p1_solution - scale32(pid_gyros[P1][ROLL], p1_gyro_roll_volume_source * 5);
					break;
				default:
					break;
			}

			switch (OAVMixerFdbks->P1PitchGyro[i])
			{
				case OAVMIXERFDBKS_P1PITCHGYRO_OFF:
					break;
				case OAVMIXERFDBKS_P1PITCHGYRO_ON:
					if (OAVMixerVolumes->P1ElevatorVolume[i] < 0 )
					{
						p1_solution = p1_solution - pid_gyros[P1][PITCH];  // Reverse if volume negative
					}
					else
					{
						p1_solution = p1_solution + pid_gyros[P1][PITCH];
					}
					break;
				case OAVMIXERFDBKS_P1PITCHGYRO_SCALEX5:
					p1_solution = p1_solution + scale32(pid_gyros[P1][PITCH], OAVMixerVolumes->P1ElevatorVolume[i] * 5);
					break;
				default:
					break;
			}

			switch (OAVMixerFdbks->P1YawGyro[i])
			{
				case OAVMIXERFDBKS_P1YAWGYRO_OFF:
					break;
				case OAVMIXERFDBKS_P1YAWGYRO_ON:
					if (p1_gyro_yaw_volume_source < 0 )
					{
						p1_solution = p1_solution - pid_gyros[P1][YAW];  // Reverse if volume negative
					}
					else
					{
						p1_solution = p1_solution + pid_gyros[P1][YAW];
					}
					break;
				case OAVMIXERFDBKS_P1YAWGYRO_SCALEX5:
					p1_solution = p1_solution + scale32(pid_gyros[P1][YAW], p1_gyro_yaw_volume_source * 5);
					break;
				default:
					break;
			}
		}

		// P2 gyros
		if (OAVStatus->OAVTransition > 0)
		{
			switch (OAVMixerFdbks->P2RollGyro[i])
			{
				case OAVMIXERFDBKS_P2ROLLGYRO_OFF:
					break;
				case OAVMIXERFDBKS_P2ROLLGYRO_ON:
					if (OAVMixerVolumes->P2AileronVolume[i] < 0 )
					{
						p2_solution = p2_solution + pid_gyros[P2][ROLL];  // Reverse if volume negative
					}
					else
					{
						p2_solution = p2_solution - pid_gyros[P2][ROLL];
					}
					break;
				case OAVMIXERFDBKS_P2ROLLGYRO_SCALEX5:
					p2_solution = p2_solution - scale32(pid_gyros[P2][ROLL], OAVMixerVolumes->P2AileronVolume[i] * 5);
					break;
				default:
					break;
			}

			switch (OAVMixerFdbks->P2PitchGyro[i])
			{
				case OAVMIXERFDBKS_P2PITCHGYRO_OFF:
					break;
				case OAVMIXERFDBKS_P2PITCHGYRO_ON:
					if (OAVMixerVolumes->P2ElevatorVolume[i] < 0 )
					{
						p2_solution = p2_solution - pid_gyros[P2][PITCH];  // Reverse if volume negative
					}
					else
					{
						p2_solution = p2_solution + pid_gyros[P2][PITCH];
					}
					break;
				case OAVMIXERFDBKS_P2PITCHGYRO_SCALEX5:
					p2_solution = p2_solution + scale32(pid_gyros[P2][PITCH], OAVMixerVolumes->P2ElevatorVolume[i] * 5);
					break;
				default:
					break;
			}

			switch (OAVMixerFdbks->P2YawGyro[i])
			{
				case OAVMIXERFDBKS_P2YAWGYRO_OFF:
					break;
				case OAVMIXERFDBKS_P2YAWGYRO_ON:
					if (OAVMixerVolumes->P2RudderVolume[i] < 0 )
					{
						p2_solution = p2_solution - pid_gyros[P2][YAW];  // Reverse if volume negative
					}
					else
					{
						p2_solution = p2_solution + pid_gyros[P2][YAW];
					}
					break;
				case OAVMIXERFDBKS_P2YAWGYRO_SCALEX5:
					p2_solution = p2_solution + scale32(pid_gyros[P2][YAW], OAVMixerVolumes->P2RudderVolume[i] * 5);
					break;
				default:
					break;
			}
		}

		//
		// Mix in accelerometers
		//
		// P1
		if (OAVStatus->OAVTransition < 100)
		{
			switch (OAVMixerFdbks->P1RollAL[i])
			{
				case OAVMIXERFDBKS_P1ROLLAL_OFF:
					break;
				case OAVMIXERFDBKS_P1ROLLAL_ON:
					if (p1_accel_roll_volume_source < 0 )
					{
						p1_solution = p1_solution + pid_accels[P1][ROLL];  // Reverse if volume negative
					}
					else
					{
						p1_solution = p1_solution - pid_accels[P1][ROLL];  // or simply add
					}
					break;
				case OAVMIXERFDBKS_P1ROLLAL_SCALEX5:
					p1_solution = p1_solution - scale32(pid_accels[P1][ROLL], p1_accel_roll_volume_source * 5);
					break;
				default:
					break;
			}

			switch (OAVMixerFdbks->P1PitchAL[i])
			{
				case OAVMIXERFDBKS_P1PITCHAL_OFF:
					break;
				case OAVMIXERFDBKS_P1PITCHAL_ON:
					if (OAVMixerVolumes->P1ElevatorVolume[i] < 0 )
					{
						p1_solution = p1_solution - pid_accels[P1][PITCH];  // Reverse if volume negative
					}
					else
					{
						p1_solution = p1_solution + pid_accels[P1][PITCH];
					}
					break;
				case OAVMIXERFDBKS_P1PITCHAL_SCALEX5:
					p1_solution = p1_solution + scale32(pid_accels[P1][PITCH], OAVMixerVolumes->P1ElevatorVolume[i] * 5);
					break;
				default:
					break;
			}

			switch (OAVMixerFdbks->P1AltDamp[i])
			{
				case OAVMIXERFDBKS_P1ALTDAMP_OFF:
					break;
				case OAVMIXERFDBKS_P1ALTDAMP_ON:
					if (OAVMixerVolumes->P1ThrottleVolume[i] < 0 )
					{
						p1_solution = p1_solution + pid_accels[P1][YAW];  // Reverse if volume negative
					}
					else
					{
						p1_solution = p1_solution - pid_accels[P1][YAW];
					}
					break;
				case OAVMIXERFDBKS_P1ALTDAMP_SCALEX5:
					p1_solution = p1_solution - scale32(pid_accels[P1][YAW], OAVMixerVolumes->P1ThrottleVolume[i] * 5);
					break;
				default:
					break;
			}
		}

		// P2
		if (OAVStatus->OAVTransition > 0)
		{
			switch (OAVMixerFdbks->P2RollAL[i])
			{
				case OAVMIXERFDBKS_P2ROLLAL_OFF:
					break;
				case OAVMIXERFDBKS_P2ROLLAL_ON:
				if (OAVMixerVolumes->P2AileronVolume[i] < 0 )
					{
						p2_solution = p2_solution + pid_accels[P2][ROLL];  // Reverse if volume negative
					}
					else
					{
						p2_solution = p2_solution - pid_accels[P2][ROLL];  // or simply add
					}
					break;
				case OAVMIXERFDBKS_P2ROLLAL_SCALEX5:
					p2_solution = p2_solution - scale32(pid_accels[P2][ROLL], OAVMixerVolumes->P2AileronVolume[i] * 5);
					break;
				default:
					break;
			}

			switch (OAVMixerFdbks->P2PitchAL[i])
			{
				case OAVMIXERFDBKS_P2PITCHAL_OFF:
					break;
				case OAVMIXERFDBKS_P2PITCHAL_ON:
					if (OAVMixerVolumes->P2ElevatorVolume[i] < 0 )
					{
						p2_solution = p2_solution - pid_accels[P2][PITCH];  // Reverse if volume negative
					}
					else
					{
						p2_solution = p2_solution + pid_accels[P2][PITCH];
					}
					break;
				case OAVMIXERFDBKS_P2PITCHAL_SCALEX5:
					p2_solution = p2_solution + scale32(pid_accels[P2][PITCH], OAVMixerVolumes->P2ElevatorVolume[i] * 5);
					break;
				default:
					break;
			}

			switch (OAVMixerFdbks->P2AltDamp[i])
			{
				case OAVMIXERFDBKS_P2ALTDAMP_OFF:
					break;
				case OAVMIXERFDBKS_P2ALTDAMP_ON:
					if (OAVMixerVolumes->P2ThrottleVolume[i] < 0 )
					{
						p2_solution = p2_solution + pid_accels[P2][YAW];  // Reverse if volume negative
					}
					else
					{
						p2_solution = p2_solution - pid_accels[P2][YAW];
					}
					break;
				case OAVMIXERFDBKS_P2ALTDAMP_SCALEX5:
					p2_solution = p2_solution - scale32(pid_accels[P2][YAW], OAVMixerVolumes->P2ThrottleVolume[i] * 5);
					break;
				default:
					break;
			}
		}

		//
		// Process mixers
		//

		// Mix in other outputs here (P1)
		if (OAVStatus->OAVTransition < 100)
		{
			// Mix in dedicated RC sources - aileron, elevator and rudder
			if (OAVMixerVolumes->P1AileronVolume[i] != 0)  // Mix in dedicated aileron
			{
				temp2 = scale32(rc_inputs[AILERON], OAVMixerVolumes->P1AileronVolume[i]);
				p1_solution = p1_solution + temp2;
			}
			if (OAVMixerVolumes->P1ElevatorVolume[i] != 0)  // Mix in dedicated elevator
			{
				temp2 = scale32(rc_inputs[ELEVATOR], OAVMixerVolumes->P1ElevatorVolume[i]);
				p1_solution = p1_solution + temp2;
			}
			if (OAVMixerVolumes->P1RudderVolume[i] != 0)  // Mix in dedicated rudder
			{
				temp2 = scale32(rc_inputs[RUDDER], OAVMixerVolumes->P1RudderVolume[i]);
				p1_solution = p1_solution + temp2;
			}

			// Other sources
			if ((OAVMixerVolumes->P1SourceAVolume[i] != 0) &&
			    (OAVMixerVolumes->P1SourceA[i] != OAVMIXERVOLUMES_P1SOURCEA_NOSOURCE))  // Mix in first extra source
			{
				temp2 = source_select_p1[OAVMixerVolumes->P1SourceA[i]];
				temp2 = scale32(temp2, OAVMixerVolumes->P1SourceAVolume[i]);
				p1_solution = p1_solution + temp2;
			}
			if ((OAVMixerVolumes->P1SourceBVolume != 0) &&
			    (OAVMixerVolumes->P1SourceB[i] != OAVMIXERVOLUMES_P1SOURCEB_NOSOURCE))  // Mix in second extra source
			{
				temp2 = source_select_p1[OAVMixerVolumes->P1SourceB[i]];
				temp2 = scale32(temp2, OAVMixerVolumes->P1SourceBVolume[i]);
				p1_solution = p1_solution + temp2;
			}
		}

		// Mix in other outputs here (P2)
		if (OAVStatus->OAVTransition > 0)
		{
			// Mix in dedicated RC sources - aileron, elevator and rudder
			if (OAVMixerVolumes->P2AileronVolume[i] != 0)  // Mix in dedicated aileron
			{
				temp2 = scale32(rc_inputs[AILERON], OAVMixerVolumes->P2AileronVolume[i]);
				p2_solution = p2_solution + temp2;
			}
			if (OAVMixerVolumes->P2ElevatorVolume[i] != 0)  // Mix in dedicated elevator
			{
				temp2 = scale32(rc_inputs[ELEVATOR], OAVMixerVolumes->P2ElevatorVolume[i]);
				p2_solution = p2_solution + temp2;
			}
			if (OAVMixerVolumes->P2RudderVolume[i] != 0)  // Mix in dedicated rudder
			{
				temp2 = scale32(rc_inputs[RUDDER], OAVMixerVolumes->P2RudderVolume[i]);
				p2_solution = p2_solution + temp2;
			}

			// Other sources
			if ((OAVMixerVolumes->P2SourceAVolume[i] != 0) &&
			    (OAVMixerVolumes->P2SourceA[i] != OAVMIXERVOLUMES_P2SOURCEA_NOSOURCE))  // Mix in first extra source
			{
				temp2 = source_select_p2[OAVMixerVolumes->P2SourceA[i]];
				temp2 = scale32(temp2, OAVMixerVolumes->P2SourceAVolume[i]);
				p2_solution = p2_solution + temp2;
			}
			if ((OAVMixerVolumes->P2SourceBVolume[i] != 0) &&
			    (OAVMixerVolumes->P2SourceB[i] != OAVMIXERVOLUMES_P2SOURCEA_NOSOURCE))   // Mix in second extra source
			{
				temp2 = source_select_p2[OAVMixerVolumes->P2SourceB[i]];
				temp2 = scale32(temp2, OAVMixerVolumes->P2SourceBVolume[i]);
				p2_solution = p2_solution + temp2;
			}
		}

		// Save solution for this channel. Note that this contains cross-mixed data from the *last* cycle
		OAVStatus->P1Value[i] = p1_solution;
		OAVStatus->P2Value[i] = p2_solution;

	} // Mixer loop: for (i = 0; i < MIX_OUTPUTS; i++)

	//
	// Mixer transition code
	//

	// Recalculate P1 values based on transition stage
	for (i = 0; i < MIXOUTPUTS; i++)
	{
		// Speed up the easy ones :)
		if (OAVStatus->OAVTransition == 0)
		{
			temp1 = OAVStatus->P1Value[i];
		}
		else if (OAVStatus->OAVTransition >= 100)
		{
			temp1 = OAVStatus->P2Value[i];
		}
		else
		{
			// Get source channel value
			temp1 = OAVStatus->P1Value[i];
			temp1 = scale32(temp1, (100 - OAVStatus->OAVTransition));

			// Get destination channel value
			temp2 = OAVStatus->P2Value[i];
			temp2 = scale32(temp2, OAVStatus->OAVTransition);

			// Sum the mixers
			temp1 = temp1 + temp2;
		}
		// Save transitioned solution into P1
		OAVStatus->P1Value[i] = temp1;
	}

	//
	// P1/P2 throttle curve handling
	// Work out the resultant Monopolar throttle value based on
	// p1_throttle, p2_throttle and the transition number
	//

	// Only process if there is a difference
	if (p1_throttle != p2_throttle)
	{
		// Speed up the easy ones :)
		if (OAVStatus->OAVTransition == 0)
		{
			e32temp3 = p1_throttle;
		}
		else if (OAVStatus->OAVTransition >= 100)
		{
			e32temp3 = p2_throttle;
		}
		else
		{
			// Calculate step difference in 1/100ths and round
			e32temp1 = (p2_throttle - p1_throttle);
			e32temp1 = e32temp1 << 16; 						// Multiply by 65536 so divide gives reasonable step values
			e32step1 = e32temp1 / (int32_t)100;

			// Set start (P1) point
			e32temp2 = p1_throttle;							// Promote to 32 bits
			e32temp2 = e32temp2 << 16;

			// Multiply [transition] steps (0 to 100)
			e32temp3 = e32temp2 + (e32step1 * OAVStatus->OAVTransition);

			// Round, then rescale to normal value
			e32temp3 = e32temp3 + (int32_t)32768;
			e32temp3 = e32temp3 >> 16;
		}
	}

	// No curve
	else
	{
		// Just use the value of p1_throttle as there is no curve
		e32temp3 = p1_throttle; // Promote to 16 bits
	}

	// Copy to monopolar throttle
	mono_throttle = (int16_t)e32temp3;

	//
	// Groovy transition curve handling. Must be after the transition.
	// Uses the transition value, but is not part of the transition
	// mixer. Linear or Sine curve. Reverse Sine done automatically
	//

	for (i = 0; i < MIXOUTPUTS; i++)
	{
		// Ignore if both throttle volumes are 0% (no throttle)
		if 	(!((OAVMixerVolumes->P1ThrottleVolume[i] == 0) && (OAVMixerVolumes->P2ThrottleVolume[i] == 0)))
		{
			// Only process if there is a curve
			if (OAVMixerVolumes->P1ThrottleVolume[i] != OAVMixerVolumes->P2ThrottleVolume[i])
			{
				// Calculate step difference in 1/100ths and round
				temp1 = (OAVMixerVolumes->P2ThrottleVolume[i] - OAVMixerVolumes->P1ThrottleVolume[i]);
				temp1 = temp1 << 7;  // Multiply by 128 so divide gives reasonable step values
				step1 = temp1 / 100;

				// Set start (P1) point
				temp2 = OAVMixerVolumes->P1ThrottleVolume[i]; // Promote to 16 bits
				temp2 = temp2 << 7;

				// Linear vs. Sinusoidal calculation
				if (OAVMixerVolumes->ThrottleCurve[i] == OAVMIXERVOLUMES_THROTTLECURVE_LINEAR)
				{
					// Multiply [transition] steps (0 to 100)
					temp3 = temp2 + (step1 * OAVStatus->OAVTransition);
				}

				// SINE
				else if (OAVMixerVolumes->ThrottleCurve[i] == OAVMIXERVOLUMES_THROTTLECURVE_SINE)
				{
					// Choose between SINE and COSINE
					// If P2 less than P1, COSINE (reverse SINE) is the one we want
					if (step1 < 0)
					{
						// Multiply SIN[100 - transition] steps (0 to 100)
						temp3 = 100 - SIN[100 - (int8_t)OAVStatus->OAVTransition];
					}
					// If P2 greater than P1, SINE is the one we want
					else
					{
						// Multiply SIN[transition] steps (0 to 100)

					temp3 = SIN[(int8_t)OAVStatus->OAVTransition];
					}

					// Get SINE% (temp2) of difference in volumes (step1)
					// step1 is already in 100ths of the difference * 128
					// temp1 is the start volume * 128
					temp3 = temp2 + (step1 * temp3);
				}
				// SQRT SINE
				else
				{
					// Choose between SQRT SINE and SQRT COSINE
					// If P2 less than P1, COSINE (reverse SINE) is the one we want
					if (step1 < 0)
					{
						// Multiply SQRTSIN[100 - transition] steps (0 to 100)
						temp3 = 100 - SQRTSIN[100 - (int8_t)OAVStatus->OAVTransition];
					}
					// If P2 greater than P1, SINE is the one we want
					else
					{
						// Multiply SQRTSIN[transition] steps (0 to 100)
						temp3 = SQRTSIN[(int8_t)OAVStatus->OAVTransition];
					}

					// Get SINE% (temp2) of difference in volumes (step1)
					// step1 is already in 100ths of the difference * 128
					// temp1 is the start volume * 128
					temp3 = temp2 + (step1 * temp3);
				}

				// Round, then rescale to normal value
				temp3 = temp3 + 64;
				temp3 = temp3 >> 7;
			}

			// No curve
			else
			{
				// Just use the value of P1 volume as there is no curve
				temp3 = OAVMixerVolumes->P1ThrottleVolume[i]; // Promote to 16 bits
			}

			// Calculate actual throttle value to the curve
			temp3 = scale32(mono_throttle, temp3);

			// At this point, the throttle values are 0 to 2000
			// Re-scale throttle values back to neutral-centered system values (+/-1250)
			temp3 = (int16_t)((float)(temp3 - THROTTLEMIN) * 1.25f);

			// Add offset to channel value
			OAVStatus->P1Value[i] += temp3;

		} // No throttle

		// No throttles, so clamp to THROTTLEMIN if flagged as a motor
		else if (get_mix_type(MixerSettings, i) == MIXERSETTINGS_MIXER1TYPE_MOTOR)
		{
			OAVStatus->P1Value[i] = -THROTTLEOFFSET;  // THROTTLEOFFSET = 1250
		}
	}

	//
	// Per-channel 7-point offset needs to be after the transition
	// loop as it is non-linear, unlike the transition.
	//

	for (i = 0; i < MIXOUTPUTS; i++)
	{
		// The input to the curves will be the transition number, altered to appear as -1000 to 1000.
		temp1 = (OAVStatus->OAVTransition - 50) * 20; // 0 - 100 -> -1000 to 1000

		// Process as 7-point offset curve. All are BIPOLAR types.
		// Temporarily add NUMBEROFCURVES to the curve number to identify
		// them to Process_curve() as being offsets, not the other curves.
		temp2 = processCurve(i + NUMBEROFCURVES, BIPOLAR, temp1, OAVCurves, OAVOffsets);

		// Add offset to channel value
		OAVStatus->P1Value[i] += temp2;
	}
} // ProcessMixer()

/////////////////////////////////////////////////////////////////////

// 32 bit multiply/scale
// Returns immediately if multiplier is 100, 0 or -100

int16_t scale32(int16_t value16, int16_t multiplier16)
{
	int32_t temp32 = 0;

	// No change if 100% (no scaling)
	if (multiplier16 == 100)
	{
		return value16;
	}

	// Reverse if -100%
	else if (multiplier16 == -100)
	{
		return -value16;
	}

	// Zero if 0%
	else if (multiplier16 == 0)
	{
		return 0;
	}

	// Only do the multiply and scaling if necessary
	else
	{
		temp32 = value16 * multiplier16;

		// Divide by 100 and round to get scaled value
		if (temp32 >= 0)
			temp32 = (temp32 + (int32_t)50) / (int32_t)100; // Constants need to be cast up to 32 bits
		else
			temp32 = (temp32 - (int32_t)50) / (int32_t)100; // Constants need to be cast up to 32 bits

		value16 = (int16_t)temp32;
	}

	return value16;
}

/////////////////////////////////////////////////////////////////////

// Scale curve percentages to relative position (Bipolar internal units)
// Input values are -100 to 100. Output values are -1000 to 1000.

float scaleThrottleCurvePercentBipolar(float value)
{
	return value * 10.0f;
}

/////////////////////////////////////////////////////////////////////

// Scale curve percentages to relative position (Monopolar internal units)
// Note that a curve percentage is a percentage of the input value.
// Input values are 0 to 100% of full throttle. Output values are 0 to 2000 (full throttle).

float scaleThrottleCurvePercentMono(float value)
{
	return value * 20.0f;
}

/////////////////////////////////////////////////////////////////////

// Process curves. Maximum input values are +/-1000 for Bipolar curves and 0-2000 for monopolar curves.
// Curve number > NUMBEROFCURVES are the offset curves.
// Seven points    0%, 17%,  33%, 50%, 67%, 83%, 100% (Monopolar)
// Seven points -100%, 67%, -33%,  0%, 33%, 67%, 100% (Bipolar)

int16_t processCurve(uint8_t curve, uint8_t type, int16_t inputValue, OAVCurvesData *OAVCurves, OAVOffsetsData *OAVOffsets)
{
	int8_t interval = 0;

	float b, m, x1, x2, y, y1, y2;

	if (type == BIPOLAR)
	{
		// Limit input value to +/-100% (+/-1000)
		if (inputValue < -1000)
		{
			inputValue = -1000;
		}
		if (inputValue > 1000)
		{
			inputValue = 1000;
		}
	}
	else // Monopolar
	{
		// Limit input value to 0 to 100% (0 to 2000)
		if (inputValue < 0)
		{
			inputValue = 0;
		}
		if (inputValue > 2000)
		{
			inputValue = 2000;
		}
	}

	if (type == BIPOLAR)
	{
		// Work out which interval we are in
		if (inputValue <= -670)
		{
			interval = 0;
			x1       = -1000.0f;
			x2       = -670.0f;
		}
		else if (inputValue <= -330)
		{
			interval = 1;
			x1       = -670.0f;
			x2       = -330.0f;
		}
		else if (inputValue <= 0)
		{
			interval = 2;
			x1       = -330.0f;
			x2       = -0.0f;
		}
		else if (inputValue <= 330)
		{
			interval = 3;
			x1       = 0.0f;
			x2       = 330.0f;
		}
		else if (inputValue <= 670)
		{
			interval = 4;
			x1       = 330.0f;
			x2       = 670.0f;
		}
		else if (inputValue <= 1000)
		{
			interval = 5;
			x1       = 670.0f;
			x2       = 1000.0f;
		}
	}
	else // Monopolar
	{
		// Work out which interval we are in
		if (inputValue <= 340)
		{
			interval = 0;
			x1       = 0.0f;
			x2       = 340.0f;
		}
		else if (inputValue <= 660)
		{
			interval = 1;
			x1       = 340.0f;
			x2       = 660.0f;
		}
		else if (inputValue <= 1000)
		{
			interval = 2;
			x1       = 660.0f;
			x2       = 1000.0f;
		}
		else if (inputValue <= 1340)
		{
			interval = 3;
			x1       = 1000.0f;
			x2       = 1340.0f;
		}
		else if (inputValue <= 1660)
		{
			interval = 4;
			x1       = 1340.0f;
			x2       = 1660.0f;
		}
		else if (inputValue <= 2000)
		{
			interval = 5;
			x1       = 1660.0f;
			x2       = 2000.0f;
		}
	}

	// Find y1/y2 points of interval
	// Normal curves
	if (curve < NUMBEROFCURVES)
	{
		switch(interval)
		{
			case 0:
				y1 = OAVCurves->CurvePoint1[curve];
				y2 = OAVCurves->CurvePoint2[curve];
				break;
			case 1:
				y1 = OAVCurves->CurvePoint2[curve];
				y2 = OAVCurves->CurvePoint3[curve];
				break;
			case 2:
				y1 = OAVCurves->CurvePoint3[curve];
				y2 = OAVCurves->CurvePoint4[curve];
				break;
			case 3:
				y1 = OAVCurves->CurvePoint4[curve];
				y2 = OAVCurves->CurvePoint5[curve];
				break;
			case 4:
				y1 = OAVCurves->CurvePoint5[curve];
				y2 = OAVCurves->CurvePoint6[curve];
				break;
			case 5:
				y1 = OAVCurves->CurvePoint6[curve];
				y2 = OAVCurves->CurvePoint7[curve];
				break;
			default:
				break;
		}
	}
	// Offsets
	else
	{
		// Correct curve number
		curve = curve - NUMBEROFCURVES;

		switch(interval)
		{
			case 0:
				y1 = OAVOffsets->OffsetPoint1[curve];
				y2 = OAVOffsets->OffsetPoint2[curve];
				break;
			case 1:
				y1 = OAVOffsets->OffsetPoint2[curve];
				y2 = OAVOffsets->OffsetPoint3[curve];;
				break;
			case 2:
				y1 = OAVOffsets->OffsetPoint3[curve];
				y2 = OAVOffsets->OffsetPoint4[curve];
				break;
			case 3:
				y1 = OAVOffsets->OffsetPoint4[curve];
				y2 = OAVOffsets->OffsetPoint5[curve];
				break;
			case 4:
				y1 = OAVOffsets->OffsetPoint5[curve];
				y2 = OAVOffsets->OffsetPoint6[curve];
				break;
			case 5:
				y1 = OAVOffsets->OffsetPoint6[curve];
				y2 = OAVOffsets->OffsetPoint7[curve];
				break;
			default:
				break;
		}
	}

	// Work out distance to cover
	// Convert percentages to positions
	if (type == BIPOLAR)
	{
		y1 = scaleThrottleCurvePercentBipolar(y1);
		y2 = scaleThrottleCurvePercentBipolar(y2);
	}
	else
	{
		y1 = scaleThrottleCurvePercentMono(y1);
		y2 = scaleThrottleCurvePercentMono(y2);
	}

	m = (y2 - y1) / (x2 - x1);
	b = y2 - m * x2;

	y = (float)inputValue * m + b;

	return (int16_t)y;
}

/////////////////////////////////////////////////////////////////////

static MixerSettingsMixer1TypeOptions get_mix_type(MixerSettingsData *mixerSettings, int idx)
{
	switch (idx) {
	case 0:
		return mixerSettings->Mixer1Type;
		break;
	case 1:
		return mixerSettings->Mixer2Type;
		break;
	case 2:
		return mixerSettings->Mixer3Type;
		break;
	case 3:
		return mixerSettings->Mixer4Type;
		break;
	case 4:
		return mixerSettings->Mixer5Type;
		break;
	case 5:
		return mixerSettings->Mixer6Type;
		break;
	case 6:
		return mixerSettings->Mixer7Type;
		break;
	case 7:
		return mixerSettings->Mixer8Type;
		break;
	case 8:
		return mixerSettings->Mixer9Type;
		break;
	case 9:
		return mixerSettings->Mixer10Type;
		break;
	default:
		// We can never get here unless there are mixer channels not handled in the above. Fail out.
		PIOS_Assert(0);
	}
}

/////////////////////////////////////////////////////////////////////