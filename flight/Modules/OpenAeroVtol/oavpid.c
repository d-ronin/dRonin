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

#include "oav.h"
#include "oavflightprofiles.h"
#include "oavsettings.h"
#include "oavstatus.h"

/////////////////////////////////////////////////////////////////////

#define PID_SCALE 6           // Empirical amount to reduce the PID values by to make them most useful

#if defined (PIOS_INCLUDE_BMI160)
#define STANDARDLOOP 1600.0f  // Nominal Loop Rate 1600 Hz
#elif defined (SPARKY1) || defined (DTFC)
#define STANDARDLOOP  500.0f  // Nominal Loop Rate 1000 Hz
#else
#define STANDARDLOOP 1000.0f  // Nominal Loop Rate  500 Hz
#endif

//************************************************************
// Notes
//************************************************************
//
// Servo output range is 2500 to 5000, centered on 3750.
// RC and PID values are added to this then rescaled at the output stage to 1000 to 2000.
// As such, the maximum usable value that the PID section can output is +/-1250.
// So working backwards, prior to rescaling (/64) the max values are +/-80,000.
// Prior to this, PID_Gyro_I_actual has been divided by 32 so the values are now +/- 2,560,000
// However the I-term gain can be up to 127 which means the values are now limited to +/-20,157 for full scale authority.
// For reference, a constant gyro value of 50 would go full scale in about 1 second at max gain of 127 if incremented at 400Hz.
// This seems about right for heading hold usage.
//
// Gyros are configured to read +/-2000 deg/sec at full scale, or 16.4 deg/sec for each LSB value.
// I divide that by 16 to give 0.976 deg/sec for each digit the gyros show. So "50" is about 48.8 degrees per second.
// 360 deg/sec would give a reading of 368 on the sensor calibration screen. Full stick is about 1000 or so.
// So with no division of stick value by "Axis rate", full stick would equate to (1000/368 * 360) = 978 deg/sec.
// With axis rate set to 2, the stick amount is quartered (250) or 244 deg/sec. A value of 3 would result in 122 deg/sec.
//
// Stick rates: /64 (15.25), /32 (30.5), /16 (61*), /8 (122), /4 (244), /2 (488), /1 (976)

/////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////

void Sensor_PID(OAVStatusData *OAVStatus);
void Calculate_PID(void);

/////////////////////////////////////////////////////////////////////
// Code
/////////////////////////////////////////////////////////////////////

// PID globals for each [Profile] and [axis]

float 	gyro_smooth[NUMBEROFAXIS];                 // Filtered gyro data
float	integral_accel_vertf[FLIGHTMODES];         // Integrated Acc Z
int32_t	integral_gyro[FLIGHTMODES][NUMBEROFAXIS];  // PID I-terms (gyro) for each axis
int16_t pid_accels[FLIGHTMODES][NUMBEROFAXIS];
int16_t pid_gyros[FLIGHTMODES][NUMBEROFAXIS];

// Run each loop to average gyro data and also accVert data
void SensorPID(OAVFlightProfilesData *OAVFlightProfiles, OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus, float dT)
{
	float   factor    = 0.0f;
	float   gyro_adcf = 0.0f;
	float   tempf1    = 0.0f;
	int8_t  i         = 0;
	int8_t	axis      = 0;
	int16_t	stick_P1  = 0;
	int16_t	stick_P2  = 0;
	int32_t P1_temp   = 0;
	int32_t P2_temp   = 0;

	// Cross-reference table for actual RCinput elements
	// Note that axes are reversed here with respect to their gyros
	// So why is AILERON different? Well on the KK hardware the sensors are arranged such that
	// RIGHT roll = +ve gyro, UP pitch = +ve gyro and LEFT yaw = +ve gyro.
	// However the way we have organised stick polarity, RIGHT roll and yaw are +ve, and DOWN elevator is too.
	// When combining with the gyro signals, the sticks have to be in the opposite polarity as the gyros.
	// As described above, pitch and yaw are already opposed, but roll needs to be reversed.

	int16_t	rc_inputs_axis[NUMBEROFAXIS] = {-rc_inputs[AILERON], rc_inputs[ELEVATOR], rc_inputs[RUDDER]};

	int8_t stick_rates[FLIGHTMODES][NUMBEROFAXIS] =
	{
		{OAVFlightProfiles->RollIRate[P1], OAVFlightProfiles->PitchIRate[P1], OAVFlightProfiles->YawIRate[P1]},
		{OAVFlightProfiles->RollIRate[P2], OAVFlightProfiles->PitchIRate[P2], OAVFlightProfiles->YawIRate[P2]}
	};

	for (axis = 0; axis <= YAW; axis ++)
	{
		/////////////////////////////////////////////////////////////
		// Work out stick rate divider. 0 is slowest, 7 is fastest.
		// /64 (15.25), /32 (30.5), /16 (61*), /8 (122), /4 (244), /2 (488), /1 (976), *2 (1952)
		/////////////////////////////////////////////////////////////

		if (stick_rates[P1][axis] <= 6)
		{
			stick_P1 = rc_inputs_axis[axis] >> (4 - (stick_rates[P1][axis] - 2));
		}
		else
		{
			stick_P1 = rc_inputs_axis[axis] << ((stick_rates[P1][axis]) - 6);
		}

		if (stick_rates[P2][axis] <= 6)
		{
			stick_P2 = rc_inputs_axis[axis] >> (4 - (stick_rates[P2][axis] - 2));
		}
		else
		{
			stick_P2 = rc_inputs_axis[axis] << ((stick_rates[P2][axis]) - 6);
		}

		/////////////////////////////////////////////////////////////
		// Gyro LPF
		/////////////////////////////////////////////////////////////

		gyro_adcf = gyro_adc[axis]; // Promote

		if (OAVSettings->GyroLPF != OAVSETTINGS_GYROLPF_NOFILTER)
		{
			// Lookup LPF value
			tempf1 = LPF_lookup[OAVSettings->GyroLPF];

			// Gyro LPF
			gyro_smooth[axis] = ((gyro_smooth[axis] * (tempf1 - 1.0f)) + gyro_adcf) / tempf1;
		}
		else
		{
			// Use raw gyro_adc[axis] as source for gyro values when filter is off
			gyro_smooth[axis] = gyro_adcf;
		}

		// Demote back to int16_t
		gyro_adc[axis] = (int16_t)gyro_smooth[axis];

		/////////////////////////////////////////////////////////////
		// Magically correlate the I-term value with the loop rate.
		// This keeps the I-term and stick input constant over varying
		// loop rates
		/////////////////////////////////////////////////////////////

		P1_temp = gyro_adc[axis] + stick_P1;  //  This is the error term
		P2_temp = gyro_adc[axis] + stick_P2;  //  This is the error term

		// Work out multiplication factor compared to standard loop time
		factor = dT * STANDARDLOOP;

		// Adjust gyro and stick values based on factor
		tempf1 = P1_temp;           // Promote int32_t to float
		tempf1 = tempf1 * factor;
		P1_temp = (int32_t)tempf1;  // Demote to int32_t

		tempf1 = P2_temp;
		tempf1 = tempf1 * factor;
		P2_temp = (int32_t)tempf1;

		/////////////////////////////////////////////////////////////
		// Increment gyro I-terms
		/////////////////////////////////////////////////////////////

		// Calculate I-term from gyro and stick data
		// These may look similar, but they are constrained quite differently.
		integral_gyro[P1][axis] += P1_temp;
		integral_gyro[P2][axis] += P2_temp;

		/////////////////////////////////////////////////////////////
		// Limit the I-terms to the user-set limits
		/////////////////////////////////////////////////////////////

		for (i = P1; i <= P2; i++)
		{
			if (integral_gyro[i][axis] > raw_i_constrain[i][axis])
			{
				integral_gyro[i][axis] = raw_i_constrain[i][axis];
			}

			if (integral_gyro[i][axis] < -raw_i_constrain[i][axis])
			{
				integral_gyro[i][axis] = -raw_i_constrain[i][axis];
			}
		}

	} // for (axis = 0; axis <= YAW; axis ++)

	/////////////////////////////////////////////////////////////////
	// Calculate the Z-acc I-term
	// accel_vertf is already smoothed by acc_smooth, but needs DC
	// offsets removed to minimize drift.
	// Also, shrink the integral by a small fraction to temper
	// remaining offsets.
	/////////////////////////////////////////////////////////////////

	integral_accel_vertf[P1] += accel_vertf;
	integral_accel_vertf[P2] += accel_vertf;

	tempf1 = OAVSettings->AccelVertFilter;  // Promote acc_vertfilter (0 to 127)
	tempf1 = tempf1 / 10000.0f;
	tempf1 = 1.0f - tempf1;

	integral_accel_vertf[P1] = integral_accel_vertf[P1] * tempf1;  // Decimator. Shrink integrals by user-set amount
	integral_accel_vertf[P2] = integral_accel_vertf[P2] * tempf1;

		/////////////////////////////////////////////////////////////////
	// Limit the Z-acc I-terms to the user-set limits
	/////////////////////////////////////////////////////////////////
	for (i = P1; i <= P2; i++)
	{
		tempf1 = raw_i_constrain[i][ZED];	// Promote

		if (integral_accel_vertf[i] > tempf1)
		{
			integral_accel_vertf[i] = tempf1;
		}

		if (integral_accel_vertf[i] < -tempf1)
		{
			integral_accel_vertf[i] = -tempf1;
		}
	}
}

/////////////////////////////////////////////////////////////////////

// Run just before PWM output, using averaged data
void CalculatePID(OAVFlightProfilesData *OAVFlightProfiles, OAVStatusData *OAVStatus)
{
	int32_t PID_gyro_temp1     = 0;  // P1
	int32_t PID_gyro_temp2     = 0;  // P2
	int32_t PID_acc_temp1      = 0;  // P
	int32_t PID_acc_temp2      = 0;  // I
	int32_t PID_Gyro_I_actual1 = 0;  // Actual unbound i-terms P1
	int32_t PID_Gyro_I_actual2 = 0;  // P2
	int8_t	axis               = 0;

	int8_t i = 0;


	// Check for throttle reset
	if (monopolar_throttle < 50)  // THROTTLEIDLE = 50
	{
		// Reset I-terms at throttle cut. Using memset saves code space
		memset(&integral_gyro[P1][ROLL], 0, sizeof(int32_t) * 6);
		integral_accel_vertf[P1] = 0.0f;
		integral_accel_vertf[P2] = 0.0f;
	}

	// Initialise arrays with gain values.
	int8_t 	P_gain[FLIGHTMODES][NUMBEROFAXIS] =
		{
			{OAVFlightProfiles->RollP[P1], OAVFlightProfiles->PitchP[P1], OAVFlightProfiles->YawP[P1]},
			{OAVFlightProfiles->RollP[P2], OAVFlightProfiles->PitchP[P2], OAVFlightProfiles->YawP[P2]}
		};

	int8_t 	I_gain[FLIGHTMODES][NUMBEROFAXIS+1] =
		{
			{OAVFlightProfiles->RollI[P1], OAVFlightProfiles->PitchI[P1], OAVFlightProfiles->YawI[P1], OAVFlightProfiles->AZedI[P1]},
			{OAVFlightProfiles->RollI[P2], OAVFlightProfiles->PitchI[P2], OAVFlightProfiles->YawI[P2], OAVFlightProfiles->AZedI[P2]}
		};

	int8_t 	L_gain[FLIGHTMODES][NUMBEROFAXIS] =
		{
			{OAVFlightProfiles->RollAutoLvl[P1], OAVFlightProfiles->PitchAutoLvl[P1], OAVFlightProfiles->AZedP[P1]},
			{OAVFlightProfiles->RollAutoLvl[P2], OAVFlightProfiles->PitchAutoLvl[P2], OAVFlightProfiles->AZedP[P2]}
		};

	// Only for roll and pitch acc trim
	int16_t	L_trim[FLIGHTMODES][2] =
		{
			{OAVFlightProfiles->RollTrim[P1], OAVFlightProfiles->PitchTrim[P1]},
			{OAVFlightProfiles->RollTrim[P2], OAVFlightProfiles->PitchTrim[P2]}
		};

	/////////////////////////////////////////////////////////////////
	// PID loop
	/////////////////////////////////////////////////////////////////

	for (axis = 0; axis <= YAW; axis ++)
	{
		/////////////////////////////////////////////////////////////
		// Add in gyro Yaw trim
		/////////////////////////////////////////////////////////////

		if (axis == YAW)
		{
			PID_gyro_temp1 = (int32_t)(OAVFlightProfiles->YawTrim[P1] << 6);
			PID_gyro_temp2 = (int32_t)(OAVFlightProfiles->YawTrim[P2] << 6);
		}
		// Reset PID_gyro variables to that data does not accumulate cross-axis
		else
		{
			PID_gyro_temp1 = 0;
			PID_gyro_temp2 = 0;
		}

		/////////////////////////////////////////////////////////////
		// Calculate PID gains
		/////////////////////////////////////////////////////////////

		// Gyro P-term                                                    // Profile P1
		PID_gyro_temp1 += gyro_adc[axis] * P_gain[P1][axis];              // Multiply P-term (Max gain of 127)
		PID_gyro_temp1 = PID_gyro_temp1 * (int32_t)3;                     // Multiply by 3

		// Gyro I-term
		PID_Gyro_I_actual1 = integral_gyro[P1][axis] * I_gain[P1][axis];  // Multiply I-term (Max gain of 127)
		PID_Gyro_I_actual1 = PID_Gyro_I_actual1 >> 5;                     // Divide by 32

		// Gyro P-term
		PID_gyro_temp2 += gyro_adc[axis] * P_gain[P2][axis];              // Profile P2
		PID_gyro_temp2 = PID_gyro_temp2 * (int32_t)3;

		// Gyro I-term
		PID_Gyro_I_actual2 = integral_gyro[P2][axis] * I_gain[P2][axis];
		PID_Gyro_I_actual2 = PID_Gyro_I_actual2 >> 5;

		/////////////////////////////////////////////////////////////
		// I-term output limits
		/////////////////////////////////////////////////////////////

		// P1 limits
		if (PID_Gyro_I_actual1 > raw_i_limits[P1][axis])
		{
			PID_Gyro_I_actual1 = raw_i_limits[P1][axis];
		}
		else if (PID_Gyro_I_actual1 < -raw_i_limits[P1][axis])
		{
			PID_Gyro_I_actual1 = -raw_i_limits[P1][axis];
		}

		// P2 limits
		if (PID_Gyro_I_actual2 > raw_i_limits[P2][axis])
		{
			PID_Gyro_I_actual2 = raw_i_limits[P2][axis];
		}
		else if (PID_Gyro_I_actual2 < -raw_i_limits[P2][axis])
		{
			PID_Gyro_I_actual2 = -raw_i_limits[P2][axis];
		}

		/////////////////////////////////////////////////////////////
		// Sum Gyro P, I and D terms and rescale
		/////////////////////////////////////////////////////////////

		pid_gyros[P1][axis] = (int16_t)((PID_gyro_temp1 + PID_Gyro_I_actual1) >> PID_SCALE); // Currently PID_SCALE = 6 so /64
		pid_gyros[P2][axis] = (int16_t)((PID_gyro_temp2 + PID_Gyro_I_actual2) >> PID_SCALE);

		/////////////////////////////////////////////////////////////
		// Calculate error from angle data and trim (roll and pitch only)
		/////////////////////////////////////////////////////////////

		if (axis < YAW)
		{
			// Do for P1 and P2
			for (i = P1; i <= P2; i++)
			{
				PID_acc_temp1 = OAVStatus->OAVAngles[axis] - L_trim[i][axis];  // Offset angle with trim
				PID_acc_temp1 *= L_gain[i][axis];                              // P-term of accelerometer (Max gain of 127)
				pid_accels[i][axis] = (int16_t)(PID_acc_temp1 >> 8);           // Reduce and convert to integer
			}
		}

	} // PID loop (axis)

	/////////////////////////////////////////////////////////////////
	// Calculate an Acc-Z PI value
	/////////////////////////////////////////////////////////////////

	// Do for P1 and P2
	for (i = P1; i <= P2; i++)
	{
		// P-term
		PID_acc_temp1 = (int32_t)-accel_vertf;       // Zeroed acc_smooth signal. Negate to oppose G
		PID_acc_temp1 *= L_gain[i][YAW];             // Multiply P-term (Max gain of 127)
		PID_acc_temp1 = PID_acc_temp1 * (int32_t)3;  // Multiply by 3

		// I-term
		PID_acc_temp2 = (int32_t)-integral_accel_vertf[i];  // Get and copy integrated Z-acc value. Negate to oppose G
		PID_acc_temp2 *= I_gain[i][ZED];                    // Multiply I-term (Max gain of 127)
		PID_acc_temp2 = PID_acc_temp2 >> 2;                 // Divide by 4

		if (PID_acc_temp2 > raw_i_limits[i][ZED])           // Limit I-term outputs to user-set percentage
		{
			PID_acc_temp2 = raw_i_limits[i][ZED];
		}
		if (PID_acc_temp2 < -raw_i_limits[i][ZED])
		{
			PID_acc_temp2 = -raw_i_limits[i][ZED];
		}

		// Formulate PI value and scale
		pid_accels[i][YAW] = (int16_t)((PID_acc_temp1 + PID_acc_temp2) >> PID_SCALE);  // Copy to global values
	}
}

/////////////////////////////////////////////////////////////////////