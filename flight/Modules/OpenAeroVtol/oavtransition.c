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
#include "oavsettings.h"

/////////////////////////////////////////////////////////////////////

#if defined (PIOS_INCLUDE_BMI160)
#define TRANSITION_COUNT 16
#elif defined (DTFC)
#define TRANSITION_COUNT  5
#else
#define TRANSITION_COUNT 10
#endif

/////////////////////////////////////////////////////////////////////

int8_t   old_flight_profile   = 3;
int8_t   old_transition       = 0;
int8_t   old_transition_mode  = 0;
uint8_t  old_transition_state = TRANS_P1;
int16_t  transition_counter   = 0;
uint8_t  transition_direction = P2;
uint8_t  transition_state     = TRANS_P1;
uint16_t transition_timeout   = 0;

// Transition matrix
// Usage: transition_state = transition_matrix[flight_profile][old_flight_profile]
// flight_profile is where you've been asked to go, and old_flight_profile is where you were.
// transition_state is where you end up :)

const int8_t transition_matrix[3][3] =
	{
		{TRANSITIONING,         TRANS_P1n_to_P1_start, TRANS_P2_to_P1_start},
		{TRANS_P1_to_P1n_start, TRANSITIONING,         TRANS_P2_to_P1n_start},
		{TRANS_P1_to_P2_start,  TRANS_P1n_to_P2_start, TRANSITIONING}
	};

/////////////////////////////////////////////////////////////////////

void UpdateTransition(uint8_t channel, OAVStatusData *OAVStatus)
{
	int16_t temp = 0;

	// Offset RC input to (approx) -250 to 2250
	temp = rc_inputs[channel] + 1000;

	// Trim lower end to zero (0 to 2250)
	if (temp < 0) temp = 0;

	// Convert 0 to 2250 to 0 to 125. Divide by 20
	// Round to avoid truncation errors
	OAVStatus->OAVTransition = (temp + 10) / 20;

	// transition now has a range of 0 to 101 for 0 to 2000 input
	// Limit extent of transition value 0 to 100 (101 steps)
	if (OAVStatus->OAVTransition > 100)
		OAVStatus->OAVTransition = 100;
}

/////////////////////////////////////////////////////////////////////

void ProfileTransition(OAVSettingsData *OAVSettings, OAVStatusData *OAVStatus)
{
	// Flags
	bool transition_updated = false;

	// Locals
	int8_t   flight_profile;
	uint16_t transition_time = 0;

	/////////////////////////////////////////////////////////////////
	// Flight profile / transition state selection
	//
	// When transitioning, the flight profile is a moving blend of
	// Flight profiles P1 to P2. The transition speed is controlled
	// by the Config.TransitionSpeed setting.
	// The transition will hold at P1n position if directed to.
	/////////////////////////////////////////////////////////////////

	// P2 transition point hard-coded to 50% above center
	if 	(rc_inputs[OAVSettings->ProfileChannel] > 500)
	{
		flight_profile = 2;  // Flight mode 2 (P2)
	}
	// P1.n transition point hard-coded to 50% below center
	else if (rc_inputs[OAVSettings->ProfileChannel] > -500)
	{
		flight_profile = 1;  // Flight mode 1 (P1.n)
	}
	// Otherwise the default is P1
	else
	{
		flight_profile = 0;  // Flight mode 0 (P1)
	}

	// Reset update request each loop
	transition_updated = false;

	/////////////////////////////////////////////////////////////////
	// Transition state setup/reset
	//
	// Set up the correct state for the current setting.
	// Check for initial startup - the only time that old_flight_profile should be "3".
	// Also, re-initialise if the transition setting is changed
	/////////////////////////////////////////////////////////////////

	if ((old_flight_profile == 3) || (old_transition_mode != OAVSettings->OutboundTransition))
	{
		switch(flight_profile)
		{
			case 0:
				transition_state = TRANS_P1;
				transition_counter = OAVSettings->TransitionLow;
				break;
			case 1:
				transition_state = TRANS_P1n;
				transition_counter = OAVSettings->TransitionMid; // Set transition point to the user-selected point
				break;
			case 2:
				transition_state = TRANS_P2;
				transition_counter = OAVSettings->TransitionHigh;
				break;
			default:
				break;
		}
		old_flight_profile = flight_profile;
		old_transition_mode = OAVSettings->OutboundTransition;
	}

	/////////////////////////////////////////////////////////////////
	// Transition state handling
	/////////////////////////////////////////////////////////////////

	// Update timed transition when changing flight modes
	if (flight_profile != old_flight_profile)
	{
		// Flag that update is required if mode changed
		transition_updated = true;
	}

	// Work out transition number when manually transitioning
	// Convert number to percentage (0 to 100%)
	if (OAVSettings->OutboundTransition == 0)
	{
		// Update the transition variable based on the selected RC channel
		UpdateTransition(OAVSettings->ProfileChannel, OAVStatus);
	}
	else
	{
		// transition_counter counts from 0 to 100 (101 steps)
		OAVStatus->OAVTransition = transition_counter;
	}

	// Update transition state change when control value or flight mode changes
	if (transition_updated)
	{
		// Update transition state from matrix
		transition_state = transition_matrix[flight_profile][old_flight_profile];
	}

	// Always in the TRANSITIONING state when OutboundTransition is 0
	// This prevents state changes when controlled by a channel
	if (OAVSettings->OutboundTransition == 0)
	{
		transition_state = TRANSITIONING;
	}

	// Calculate transition time from user's setting based on the direction of travel
	if (transition_direction == P2)
	{
		transition_time = OAVSettings->OutboundTransition * TRANSITION_COUNT; // Outbound transition speed
	}
	else
	{
		transition_time = OAVSettings->InboundTransition * TRANSITION_COUNT;  // Inbound transition speed
	}

	// Update state, values and transition_counter at required intervals
	if (((OAVSettings->OutboundTransition != 0) && (transition_timeout > transition_time)) || transition_updated)
	{
		transition_timeout = 0;
		transition_updated = false;

		// Fixed, end-point states
		if (transition_state == TRANS_P1)
		{
			transition_counter = OAVSettings->TransitionLow;
		}
		else if (transition_state == TRANS_P1n)
		{
			transition_counter = OAVSettings->TransitionMid;
		}
		else if (transition_state == TRANS_P2)
		{
			transition_counter = OAVSettings->TransitionHigh;
		}

		// Handle timed transition towards P1
		if ((transition_state == TRANS_P1n_to_P1_start) || (transition_state == TRANS_P2_to_P1_start))
		{
			if (transition_counter > OAVSettings->TransitionLow)
			{
				transition_counter--;

				// Check end point
				if (transition_counter <= OAVSettings->TransitionLow)
				{
					transition_counter = OAVSettings->TransitionLow;
					transition_state = TRANS_P1;
				}
			}
			else
			{
				transition_counter++;

				// Check end point
				if (transition_counter >= OAVSettings->TransitionLow)
				{
					transition_counter = OAVSettings->TransitionLow;
					transition_state = TRANS_P1;
				}
			}

			transition_direction = P1;
		}

		// Handle timed transition between P1 and P1.n
		if (transition_state == TRANS_P1_to_P1n_start)
		{
			if (transition_counter > OAVSettings->TransitionMid)
			{
				transition_counter--;

				// Check end point
				if (transition_counter <= OAVSettings->TransitionMid)
				{
					transition_counter = OAVSettings->TransitionMid;
					transition_state = TRANS_P1n;
				}
			}
			else
			{
				transition_counter++;

				// Check end point
				if (transition_counter >= OAVSettings->TransitionMid)
				{
					transition_counter = OAVSettings->TransitionMid;
					transition_state = TRANS_P1n;
				}
			}

			transition_direction = P2;
		}

		// Handle timed transition between P2 and P1.n
		if (transition_state == TRANS_P2_to_P1n_start)
		{
			if (transition_counter > OAVSettings->TransitionMid)
			{
				transition_counter--;

				// Check end point
				if (transition_counter <= OAVSettings->TransitionMid)
				{
					transition_counter = OAVSettings->TransitionMid;
					transition_state = TRANS_P1n;
				}
			}
			else
			{
				transition_counter++;

				// Check end point
				if (transition_counter >= OAVSettings->TransitionMid)
				{
					transition_counter = OAVSettings->TransitionMid;
					transition_state = TRANS_P1n;
				}
			}

			transition_direction = P1;
		}

		// Handle timed transition towards P2
		if ((transition_state == TRANS_P1n_to_P2_start) || (transition_state == TRANS_P1_to_P2_start))
		{
			if (transition_counter > OAVSettings->TransitionHigh)
			{
				transition_counter--;

				// Check end point
				if (transition_counter <= OAVSettings->TransitionHigh)
				{
					transition_counter = OAVSettings->TransitionHigh;
					transition_state = TRANS_P2;
				}
			}
			else
			{
				transition_counter++;

				// Check end point
				if (transition_counter >= OAVSettings->TransitionHigh)
				{
					transition_counter = OAVSettings->TransitionHigh;
					transition_state = TRANS_P2;
				}
			}

			transition_direction = P2;
		}

	} // Update transition_counter

	transition_timeout++;

	// Zero the I-terms of the opposite state so as to ensure a bump-less OAVStatus->OAVTransition
	if ((transition_state == TRANS_P1) || (OAVStatus->OAVTransition == OAVSettings->TransitionLow))
	{
		// Clear P2 I-term while fully in P1
		memset(&integral_gyro[P2][ROLL], 0, sizeof(int32_t) * NUMBEROFAXIS);
		integral_accel_vertf[P2] = 0.0;
	}
	else if ((transition_state == TRANS_P2) || (OAVStatus->OAVTransition == OAVSettings->TransitionHigh))
	{
		// Clear P1 I-term while fully in P2
		memset(&integral_gyro[P1][ROLL], 0, sizeof(int32_t) * NUMBEROFAXIS);
		integral_accel_vertf[P1] = 0.0;
	}

	/////////////////////////////////////////////////////////////////
	// Reset the IMU when using two orientations and just leaving
	// P1 or P2
	/////////////////////////////////////////////////////////////////

	if (OAVSettings->P1Reference != OAVSETTINGS_P1REFERENCE_NO_ORIENT)
	{
		// If Config.FlightSel has changed (switch based) and TransitionSpeed not set to zero, the transition state will change.
		if ((OAVSettings->OutboundTransition != 0) && (transition_state != old_transition_state) &&
		    ((old_transition_state == TRANS_P1) || (old_transition_state == TRANS_P2)))
		{
			ResetIMU();
		}

		// If TransitionSpeed = 0, the state is always TRANSITIONING so we can't use the old/new state changes.
		// If user is using a knob or TX-slowed switch, TransitionSpeed will be 0.
		else if ((OAVSettings->OutboundTransition == 0) &&  // Manual transition mode and...
				 (((old_transition == OAVSettings->TransitionLow)  && (OAVStatus->OAVTransition > OAVSettings->TransitionLow)) ||  // Was in P1 or P2
				  ((old_transition == OAVSettings->TransitionHigh) && (OAVStatus->OAVTransition < OAVSettings->TransitionHigh))))  // Is not somewhere in-between.
		{
			ResetIMU();
		}
	}

	// Save current flight mode
	old_flight_profile = flight_profile;

	// Save old transition state;
	old_transition_state = transition_state;

	// Save last transition value
	old_transition = OAVStatus->OAVTransition;
}

/////////////////////////////////////////////////////////////////////