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

#ifndef OAVTYPEDEF_H
#define OAVTYPEDEF_H

#define	FLIGHTMODES           2  // Number of flight profiles
#define MAXRCCHANNELS         8  // Maximum input channels from RX
#define MIXOUTPUTS           10
#define	NOISETHRESH           5  // Max RX noise threshold
#define NUMBEROFAXIS          3  // Number of axis (Roll, Pitch, Yaw)
#define NUMBEROFCURVES        6  // Number of curves available
#define NUMBEROFCURVESOURCES 16  // Number of curve c and d input sources
#define NUMBEROFSOURCES      21  // Number of universal input sources
#define THROTTLEMIN        1000  // Minimum throttle input offset value. 3750-1000 = 2750 or 1.1ms.
#define THROTTLEOFFSET     1250  // Mixer offset needed to reduce the output center to MOTORMIN

/////////////////////////////////////////////////////////////////////

enum CURVES         {P1THRCURVE = 0, P2THRCURVE, P1COLLCURVE, P2COLLCURVE, GENCURVEC, GENCURVED};
enum PROFILES       {P1 = 0, P2};
enum PSYCH          {MONOPOLAR = 0, BIPOLAR};
enum RCCHANNELS     {THROTTLE = 0, AILERON, ELEVATOR, RUDDER, GEAR, AUX1, AUX2, AUX3, NOCHAN};
enum RPYARRAYINDEX  {ROLL = 0, PITCH, YAW, ZED};
enum TRANSITSTATE	{TRANS_P1 = 0, TRANS_P1_to_P1n_start, TRANS_P1n_to_P1_start, TRANS_P1_to_P2_start, TRANS_P1n, TRANSITIONING,
                     TRANS_P2_to_P1_start, TRANS_P1n_to_P2_start, TRANS_P2_to_P1n_start, TRANS_P2};

/////////////////////////////////////////////////////////////////////

#endif

/////////////////////////////////////////////////////////////////////