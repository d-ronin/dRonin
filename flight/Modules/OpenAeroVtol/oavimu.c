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

/////////////////////////////////////////////////////////////////////
// imu.c
//
// IMU code ported from KK2V1_1V12S1Beginner code
// by Rolf Bakke and Steveis
//
// Ported to OpenAeroVTOL by David Thompson (C)2014
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////////////

#include "pios_config.h"
#include "openpilot.h"

#include "oavtypedefs.h"

#include "oav.h"
#include "oavsettings.h"

/////////////////////////////////////////////////////////////////////
// IMU Prototypes
/////////////////////////////////////////////////////////////////////

void IMUUpdate(OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus, float intervalf);
void Rotate3dVector(float intervalf);
void ExtractEulerAngles(void);
void RotateVector(float angle);
void ResetIMU(void);

float small_sine(float angle);
float small_cos(float angle);
float thetascale(float gyro, float intervalf);
float ext2(float Vector);

/////////////////////////////////////////////////////////////////////
// Defines
/////////////////////////////////////////////////////////////////////

#define ACCSENSITIVITY		128.0f		// Calculate factor for Acc to report directly in g
										// For +/-4g FS, accelerometer sensitivity (+/-512 / +/-4g) = 1024/8 = 128

#define GYROSENSRADIANS		0.017045f	// Calculate factor for Gyros to report directly in rad/s
										// For +/-2000 deg/s FS, gyro sensitivity for 12 bits (+/-2048) = (4000/4096) = 0.97656 deg/s/lsb
										// 0.97656 * Pi/180 = 0.017044 rad/s/lsb

#define SMALLANGLEFACTOR	0.66f		// Empirically calculated to produce exactly 20 at 20 degrees. Was 0.66

										// Acc magnitude values - based on MultiWii 2.3 values
#define acc_1_15G_SQ		21668.0f	// (1.15 * ACCSENSITIVITY) * (1.15 * ACCSENSITIVITY)
#define acc_0_85G_SQ		11837.0f	// (0.85 * ACCSENSITIVITY) * (0.85 * ACCSENSITIVITY)

#define acc_1_6G_SQ			41943.0f	// (1.60 * ACCSENSITIVITY) * (1.60 * ACCSENSITIVITY)
#define acc_0_4G_SQ			2621.0f		// (0.40 * ACCSENSITIVITY) * (0.40 * ACCSENSITIVITY)

#define maxdeltaangle		0.2618f		// Limit possible instantaneous change in angle to +/-15 degrees (720 deg/s)

#define ZACCLPF				64.0f		// In HS mode, 256.0 would be about 0.4Hz


//************************************************************
// 	Globals
//************************************************************

float VectorA, VectorB;

float VectorX = 0;						// Initialise the vector to point straight up
float VectorY = 0;
float VectorZ = 1;

float VectorNewA, VectorNewB;
float GyroPitchVC, GyroRollVC, GyroYawVC;
float AccAnglePitch, AccAngleRoll, EulerAngleRoll, EulerAnglePitch;

int16_t	angle[2];						// Attitude in degrees - pitch and roll

//************************************************************
// Code
//
// Assume that all gyro (gyroADC[]) and acc signals (accADC[]) are
// calibrated and have the appropriate zeros removed. At rest, only the
// Z acc has a value, usually around +128, for the 1g earth gravity vector
//
// Notes:
// 1. KK code loads the MPU6050's ACCEL_XOUT data into AccY and ACCEL_YOUT into AccX (swapping X/Y axis)
// 2. In KK code GyroPitch gets GYRO_XOUT, GyroRoll gets GYRO_YOUT. GyroYaw gets GYRO_ZOUT.
//	  This means that AccX(ACCEL_YOUT) and GyroPitch are the pitch components, and AccY(ACCEL_XOUT) and GyroRoll are the roll components
//	  In other words, AccX(actually ACCEL_YOUT) and GYRO_XOUT are cludged to work together, which looks right, but by convention is wrong.
//	  By IMU convention, rotation around a gyro's X axis, for example, will cause the acc's Y axis to change.
// 3. Of course, OpenAero code does exactly the same cludge, lol. But in this case, pitch and roll *gyro* data are swapped.
//	  The result is the same, pairing opposing (X/Y/Z) axis together to the same (R/P/Y) axis name.
//
//		Actual hardware	  KK2 code		  OpenAero2 code
//		ACCEL_XOUT		= AccY*			=	accADC[ROLL]
//		ACCEL_YOUT		= AccX*			=	accADC[PITCH]
//		ACCEL_ZOUT		= AccZ			=	accADC[YAW/Z]
//		GYRO_XOUT		= GyroPitch		=	gyroADC[PITCH]*
//		GYRO_YOUT		= GyroRoll		=	gyroADC[ROLL]*
//		GYRO_ZOUT		= GyroYaw		=	gyroADC[YAW]
//
//		* = swapped axis
//
//************************************************************
//

void IMUUpdate(OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus, float intervalf)
{
	float		tempf, accADCf;
	int8_t		axis;
	uint32_t	roll_sq, pitch_sq, yaw_sq;
	uint32_t 	AccMag = 0;

	/////////////////////////////////////////////////////////////
	// Acc LPF
	/////////////////////////////////////////////////////////////

	for (axis = 0; axis < NUMBEROFAXIS; axis++)
	{
		accADCf = accel_adc[axis]; // Promote

		// Acc LPF
		if (OAVSettings->AccelLPF != OAVSETTINGS_ACCELLPF_NOFILTER)
		{
			// Lookup LPF value
			tempf = LPF_lookup[OAVSettings->AccelLPF];

			accel_smooth[axis] = ((accel_smooth[axis] * (tempf - 1.0f)) - accADCf) / tempf;
		}
		else
		{
			// Use raw accADC[axis] as source for acc values when filter off
			accel_smooth[axis] = -accADCf;
		}
	}

	// Add correction data to gyro inputs based on difference between Euler angles and acc angles
	AccAngleRoll  = accel_smooth[ROLL]  * SMALLANGLEFACTOR;  // KK2 - AccYfilter
	AccAnglePitch = accel_smooth[PITCH] * SMALLANGLEFACTOR;

	// Alter the gyro sources to the IMU as required.
	// Using gyroADCalt[] always assures that the right gyros are associated with the IMU
	GyroRollVC  = gyro_adc_alt[ROLL];
	GyroPitchVC = gyro_adc_alt[PITCH];
	GyroYawVC   = gyro_adc_alt[YAW];

	// Calculate acceleration magnitude.
	roll_sq  = (accel_adc[ROLL]  * accel_adc[ROLL]);
	pitch_sq = (accel_adc[PITCH] * accel_adc[PITCH]);
	yaw_sq   = (accel_adc[YAW]   * accel_adc[YAW]);
	AccMag   = roll_sq + pitch_sq + yaw_sq;

	// Add acc correction if inside local acceleration bounds and not inverted according to VectorZ
	// NB: new dual autolevel code needs acc correction at least temporarily when switching profiles.
	// This is actually a kind of Complementary Filter

	// New test code - only adjust when in acc mag limits and when upright or dual AL code
	if	(((AccMag > acc_0_85G_SQ) && (AccMag < acc_1_15G_SQ) && (VectorZ > 0.5f) && (OAVSettings->P1Reference == OAVSETTINGS_P1REFERENCE_NO_ORIENT)) || // Same as always when "Same"
		 ((AccMag > acc_0_4G_SQ)  && (AccMag < acc_1_6G_SQ)  && (OAVSettings->P1Reference != OAVSETTINGS_P1REFERENCE_NO_ORIENT)))
	{
		// Default Config.CF_factor is 6 (1 - 10 = 10% to 100%, 6 = 60%)
		tempf = (EulerAngleRoll - AccAngleRoll) / 10.0f;
		tempf = tempf * (12 - OAVSettings->CFFactor);
		GyroRollVC = GyroRollVC + tempf;

		tempf = (EulerAnglePitch - AccAnglePitch) / 10.0f;
		tempf = tempf * (12 - OAVSettings->CFFactor);
		GyroPitchVC = GyroPitchVC + tempf;
	}

	// Rotate up-direction 3D vector with gyro inputs
	Rotate3dVector(intervalf);
	ExtractEulerAngles();

	// Upscale to 0.01 degrees resolution and copy to angle[] for display
	OAVStatus->OAVAngles[ROLL]  = (int16_t)(EulerAngleRoll  * -100);
	OAVStatus->OAVAngles[PITCH] = (int16_t)(EulerAnglePitch * -100);
}

void Rotate3dVector(float intervalf)
{
	float theta;

	// Rotate around X axis (pitch)
	theta = thetascale(GyroPitchVC, intervalf);
	VectorA = VectorY;
	VectorB = VectorZ;
	RotateVector(theta);
	VectorY = VectorNewA;
	VectorZ = VectorNewB;

	// Rotate around Y axis (roll)
	theta = thetascale (GyroRollVC, intervalf);
	VectorA = VectorX;
	VectorB = VectorZ;
	RotateVector(theta);
	VectorX = VectorNewA;
	VectorZ = VectorNewB;

	// Rotate around Z axis (yaw)
	theta = thetascale(GyroYawVC, intervalf);
	VectorA = VectorX;
	VectorB = VectorY;
	RotateVector(theta);
	VectorX = VectorNewA;
	VectorY = VectorNewB;
}

void RotateVector(float angle)
{
	VectorNewA = VectorA * small_cos(angle) - VectorB * small_sine(angle);
	VectorNewB = VectorA * small_sine(angle) + VectorB * small_cos(angle);
}

float thetascale(float gyro, float intervalf)
{
	float theta;

	// intervalf = time in seconds since last measurement
	// GYROSENSRADIANS = conversion from raw gyro data to rad/s
	// theta = actual number of radians moved

	theta = (gyro * GYROSENSRADIANS * intervalf);

	// The sin() and cos() functions don't appreciate large
	// input values. Limit the input values to +/-15 degrees.

	if (theta > maxdeltaangle)
	{
		theta = maxdeltaangle;
	}

	if (theta < -maxdeltaangle)
	{
		theta = -maxdeltaangle;
	}

	return theta;
}

// Small angle approximations of Sine, Cosine
// NB:	These *only* work for small input values.
//		Larger values will produce fatal results
float small_sine(float angle)
{
	// sin(angle) = angle
	return angle;
}

float small_cos(float angle)
{
	// cos(angle) = (1 - (angle^2 / 2))
	float temp;

	temp = (angle * angle) / 2;
	temp = 1 - temp;

	return temp;
}

void ExtractEulerAngles(void)
{
	EulerAngleRoll = ext2(VectorX);
	EulerAnglePitch = ext2(VectorY);
}

float ext2(float Vector)
{
	float temp;

	// Rough translation to Euler angles
	temp = Vector * 90;

	// Change 0-90-0 to 0-90-180 so that
	// swap happens at 100% inverted
	if (VectorZ < 0)
	{
		// CW rotations
		if (temp > 0)
		{
			temp = 180 - temp;
		}
		// CCW rotations
		else
		{
			temp = -180 - temp;
		}
	}

	return (temp);
}

void ResetIMU(void)
{
	// Initialise the vector to point straight up
	VectorX = 0;
	VectorY = 0;
	VectorZ = 1;

	// Initialise internal vectors and attitude
	VectorA = 0;
	VectorB = 0;
	EulerAngleRoll = 0;
	EulerAnglePitch = 0;
}
