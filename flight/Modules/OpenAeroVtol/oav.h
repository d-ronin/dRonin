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

#ifndef OAV_H
#define OAV_H

/////////////////////////////////////////////////////////////////////

#include "pios_config.h"
#include "openpilot.h"

#include "oavtypedefs.h"

#include "manualcontrolcommand.h"
#include "mixersettings.h"
#include "oavcurves.h"
#include "oavflightprofiles.h"
#include "oavmixerfdbks.h"
#include "oavmixervolumes.h"
#include "oavoffsets.h"
#include "oavsettings.h"
#include "oavstatus.h"

/////////////////////////////////////////////////////////////////////

extern float   LPF_lookup[8];

extern float   accel_smooth[NUMBEROFAXIS];

extern int16_t accel_adc[NUMBEROFAXIS];
extern int16_t accel_adc_p1[NUMBEROFAXIS];
extern int16_t accel_adc_pa[NUMBEROFAXIS];

extern float   accel_vertf;

extern int16_t gyro_adc[NUMBEROFAXIS];
extern int16_t gyro_adc_p1[NUMBEROFAXIS];
extern int16_t gyro_adc_p2[NUMBEROFAXIS];
extern int16_t gyro_adc_alt[NUMBEROFAXIS];

extern int16_t pid_gyros[FLIGHTMODES][NUMBEROFAXIS];
extern int16_t pid_accels[FLIGHTMODES][NUMBEROFAXIS];

extern int32_t integral_gyro[FLIGHTMODES][NUMBEROFAXIS];
extern float   integral_accel_vertf[FLIGHTMODES];

extern int32_t raw_i_constrain[FLIGHTMODES][NUMBEROFAXIS + 1];
extern int32_t raw_i_limits[FLIGHTMODES][NUMBEROFAXIS + 1];

extern int16_t rc_inputs[8];
extern int16_t monopolar_throttle;

/////////////////////////////////////////////////////////////////////

void CalculatePID(OAVFlightProfilesData *OAVFlightProfiles, OAVStatusData *OAVStatus);

void IMUUpdate(OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus, float intervalf);

void ProcessMixer(OAVCurvesData *OAVCurves, OAVMixerFdbksData *OAVMixerFdbks, OAVMixerVolumesData *OAVMixerVolumes,
                  OAVOffsetsData *OAVOffsets, OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus,
                  MixerSettingsData *MixerSettings);

void ProfileTransition(OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus);

void ResetIMU(void);

void ScaleInputs(ManualControlCommandData *ManualControlCommand, OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus);

void SensorPID(OAVFlightProfilesData *OAVFlightProfiles, OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus, float dT);

void UpdateLimits(void);

int16_t scale32(int16_t value16, int16_t multiplier16);

/////////////////////////////////////////////////////////////////////

#endif

/////////////////////////////////////////////////////////////////////