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

#include "physical_constants.h"

#include "oavtypedefs.h"

#include "accels.h"
#include "gyros.h"
#include "manualcontrolcommand.h"
#include "manualcontrolsettings.h"
#include "oav.h"
#include "oavflightprofiles.h"
#include "oavstatus.h"

/////////////////////////////////////////////////////////////////////

#if defined (PIOS_INCLUDE_BMI160)
float LPF_lookup[8] = {52.58f, 26.46f, 13.38f, 7.09f, 4.16f, 3.09f, 2.83f, 1.00f};  // 1600 Hz
#elif defined (DTFC)
float LPF_lookup[8] = {16.43f,  8.27f,  4.18f, 2.22f, 1.30f, 1.00f, 1.00f, 1.00f};  //  500 Hz
#else
float LPF_lookup[8] = {32.86f, 16.54f,  8.36f, 4.43f, 2.60f, 1.93f, 1.77f, 1.00f};  // 1000 Hz
#endif

int16_t accel_adc[NUMBEROFAXIS];
int16_t accel_adc_p1[NUMBEROFAXIS];
int16_t accel_adc_p2[NUMBEROFAXIS];

float   accel_smooth[NUMBEROFAXIS];

float   accel_vertf;

int16_t gyro_adc[NUMBEROFAXIS];
int16_t gyro_adc_p1[NUMBEROFAXIS];
int16_t gyro_adc_p2[NUMBEROFAXIS];
int16_t gyro_adc_alt[NUMBEROFAXIS];

int32_t raw_i_constrain[FLIGHTMODES][NUMBEROFAXIS + 1];
int32_t raw_i_limits[FLIGHTMODES][NUMBEROFAXIS + 1];

int16_t rc_inputs[8];
int16_t monopolar_throttle;

/////////////////////////////////////////////////////////////////////

float scaleChannel(int n, int16_t value, ManualControlSettingsData *settings)
{
	int16_t min     = settings->ChannelMin[n];
	int16_t max     = settings->ChannelMax[n];
	int16_t neutral = settings->ChannelNeutral[n];

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

/////////////////////////////////////////////////////////////////////

void ScaleInputs(ManualControlCommandData *ManualControlCommand, OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus)
{
	uint8_t i, j;

	int16_t temp1, temp2;

	ManualControlSettingsData ManualControlSettings;
	ManualControlSettingsGet(&ManualControlSettings);

	if (ManualControlCommand->Connected == MANUALCONTROLCOMMAND_CONNECTED_TRUE) {

		for (i = 0; i < MAXRCCHANNELS; i++) {
			if (i < 5)  // Skip collective input
				j = i;
			else
				j = i + 1;

			if (ManualControlCommand->Channel[j] != 65535)
				rc_inputs[i] = (int16_t)(scaleChannel(j, ManualControlCommand->Channel[j], &ManualControlSettings) * 1000.0f);
		}
	}
	else {
		rc_inputs[THROTTLE] = (int16_t)-1000;
		rc_inputs[AILERON]  = (int16_t)0;
		rc_inputs[ELEVATOR] = (int16_t)0;
		rc_inputs[RUDDER]   = (int16_t)0;
		rc_inputs[GEAR]     = (int16_t)-1000;
		rc_inputs[AUX1]     = (int16_t)-1000;
		rc_inputs[AUX2]     = (int16_t)-1000;
		rc_inputs[AUX3]     = (int16_t)-1000;
	}

	if (rc_inputs[THROTTLE] < 0)
		rc_inputs[THROTTLE] = 0;

	monopolar_throttle = 2 * rc_inputs[THROTTLE];

	rc_inputs[THROTTLE] = monopolar_throttle - 1000;

	rc_inputs[ELEVATOR] *= -1;  // Convert Dronin elevator phase to OAV elevator phase.

	GyrosData Gyros;
	GyrosGet(&Gyros);

	//float gyro_scale = PIOS_MPU_GetGyroScale();

	gyro_adc_p2[ROLL]  =  (int16_t)(Gyros.x * 16.4f) >> 4; // Assumes gyro +/- 2000 DPS
	gyro_adc_p2[PITCH] =  (int16_t)(Gyros.y * 16.4f) >> 4; // Assumes gyro +/- 2000 DPS
	gyro_adc_p2[YAW]   = -(int16_t)(Gyros.z * 16.4f) >> 4; // Assumes gyro +/- 2000 DPS

	gyro_adc_p1[ROLL]  = -gyro_adc_p2[YAW];
	gyro_adc_p1[PITCH] =  gyro_adc_p2[PITCH];
	gyro_adc_p1[YAW]   =  gyro_adc_p2[ROLL];

	for (i = 0; i < NUMBEROFAXIS; i++)
	{
		// Only need to do this if the orientations different
		if (OAVSettings->P1Reference != OAVSETTINGS_P1REFERENCE_NO_ORIENT)
		{
			// Merge the two gyros per transition percentage
			gyro_adc_alt[i] = scale32(gyro_adc_p1[i], (100 - OAVStatus->OAVTransition)) + scale32(gyro_adc_p2[i], OAVStatus->OAVTransition); // Sum the two values

			// If the P1 reference is MODEL, always use the same gyros as P2
			if (OAVSettings->P1Reference != OAVSETTINGS_P1REFERENCE_MODEL)
			{
				//Use P2 orientation
				gyro_adc[i] = gyro_adc_p2[i];
			}
			// Otherwise use the merged orientation (EARTH reference)
			else
			{
				// Use merged orientation
				gyro_adc[i] = gyro_adc_alt[i];
			}
		}
		// Single-orientation models
		else
		{
			gyro_adc[i] = gyro_adc_p2[i];

			// Copy to alternate set of gyro values
			gyro_adc_alt[i] = gyro_adc[i];
		}
	}

	AccelsData Accels;
	AccelsGet(&Accels);

	//float accel_scale = PIOS_MPU_GetAccelScale();

	accel_adc_p2[ROLL]  = -(int16_t)(Accels.y * 8192.0f / GRAVITY) >> 6; // Assumes accel +/- 4G  Note X/Y axis swap
	accel_adc_p2[PITCH] =  (int16_t)(Accels.x * 8192.0f / GRAVITY) >> 6; // Assumes accel +/- 4G  Note X/Y axis swap
	accel_adc_p2[YAW]   =  (int16_t)(Accels.z * 8192.0f / GRAVITY) >> 6; // Assumes accel +/- 4G

	accel_adc_p1[ROLL]  =  accel_adc_p2[ROLL];
	accel_adc_p1[PITCH] =  accel_adc_p2[YAW];
	accel_adc_p1[YAW]   = -accel_adc_p2[PITCH];

	for (i = 0; i < NUMBEROFAXIS; i++)
	{
		if (OAVSettings->P1Reference != OAVSETTINGS_P1REFERENCE_NO_ORIENT)
		{
			accel_adc[i] = scale32(accel_adc_p1[i], (100 - OAVStatus->OAVTransition)) + scale32(accel_adc_p2[i], OAVStatus->OAVTransition);
		}
		else
		{
			accel_adc[i] = accel_adc_p2[i];
		}
	}

	// Only need to do this if the orientations differ
	if (OAVSettings->P1Reference != OAVSETTINGS_P1REFERENCE_NO_ORIENT)
	{
		temp1 = (int16_t)(accel_smooth[YAW] - 128.0f);
		temp2 = (int16_t)(accel_smooth[YAW] - 128.0f);

		accel_vertf = (float)(scale32(temp1, (100 - OAVStatus->OAVTransition)) + scale32(temp2, OAVStatus->OAVTransition));
	}
	// Just use the P2 value
	else
	{
		// Calculate the correct Z-axis data based on the orientation
		accel_vertf = (float)((int16_t)(accel_smooth[YAW] - 128.0f));
	}

	OAVStatus->OAVGyros[ROLL]  = gyro_adc[ROLL];
	OAVStatus->OAVGyros[PITCH] = gyro_adc[PITCH];
	OAVStatus->OAVGyros[YAW]   = gyro_adc[YAW];

	OAVStatus->OAVAccels[ROLL]  = accel_adc[ROLL];
	OAVStatus->OAVAccels[PITCH] = accel_adc[PITCH];
	OAVStatus->OAVAccels[YAW]   = accel_adc[YAW];
}

/////////////////////////////////////////////////////////////////////

void UpdateLimits(void)
{
	uint8_t i, j;
	int32_t temp32, gain32;

	OAVFlightProfilesData OAVFlightProfiles;
	OAVFlightProfilesGet(&OAVFlightProfiles);

	int8_t limits[FLIGHTMODES][NUMBEROFAXIS + 1] =
	{
		{OAVFlightProfiles.RollILimit[P1], OAVFlightProfiles.PitchILimit[P1], OAVFlightProfiles.YawILimit[P1], OAVFlightProfiles.AZedILimit[P1]},
		{OAVFlightProfiles.RollILimit[P2], OAVFlightProfiles.PitchILimit[P2], OAVFlightProfiles.YawILimit[P2], OAVFlightProfiles.AZedILimit[P2]}
	};

	int8_t gains[FLIGHTMODES][NUMBEROFAXIS + 1] =
	{
		{OAVFlightProfiles.RollI[P1], OAVFlightProfiles.PitchI[P1], OAVFlightProfiles.YawI[P1], OAVFlightProfiles.AZedI[P1]},
		{OAVFlightProfiles.RollI[P2], OAVFlightProfiles.PitchI[P2], OAVFlightProfiles.YawI[P2], OAVFlightProfiles.AZedI[P2]}
	};

	// Update I_term input constraints for all profiles
	for (j = 0; j < FLIGHTMODES; j++)
	{
		// Limits calculation is different for gyros and accs
		for (i = 0; i < (NUMBEROFAXIS); i++)
		{
			temp32 	= limits[j][i]; 						// Promote limit %

			// I-term output (throw). Convert from % to actual count
			// A value of 80,000 results in +/- 1250 or full throw at the output stage
			// This is because the maximum signal value is +/-1250 after division by 64. 1250 * 64 = 80,000
			raw_i_limits[j][i] = temp32 * (int32_t)640;	// 80,000 / 125% = 640

			// I-term source limits. These have to be different due to the I-term gain setting
			// I-term = (gyro * gain) / 32, so the gyro count for a particular gain and limit is
			// Gyro = (I-term * 32) / gain :)
			if (gains[j][i] != 0)
			{
				gain32 = gains[j][i];						// Promote gain value
				raw_i_constrain[j][i] = (raw_i_limits[j][i] << 5) / gain32;
			}
			else
			{
				raw_i_constrain[j][i] = 0;
			}
		}

		// Accs
		temp32 	= limits[j][ZED]; 						// Promote limit %
		// I-term output (throw). Convert from % to actual count
		// A value of 80,000 results in +/- 1250 or full throw at the output stage
		// This is because the maximum signal value is +/-1250 after division by 64. 1250 * 64 = 80,000
		raw_i_limits[j][ZED] = temp32 * (int32_t)640;	// 80,000 / 125% = 640

		// I-term source limits. These have to be different due to the I-term gain setting
		// I-term = (gyro * gain) / 4, so the gyro count for a particular gain and limit is
		// Gyro = (I-term * 4) / gain :)
		if (gains[j][ZED] != 0)
		{
			gain32 = gains[j][ZED];						// Promote gain value
			raw_i_constrain[j][ZED] = (raw_i_limits[j][ZED] << 2) / gain32;
		}
		else
		{
			raw_i_constrain[j][ZED] = 0;
		}
	}
}

/////////////////////////////////////////////////////////////////////