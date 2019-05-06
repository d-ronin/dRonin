/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{
 * @addtogroup OnScreenDisplay Pixel OSD
 * @{
 *
 * @brief OSD Menu
 * @file       osd_menu.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2015
 * @brief      OSD Menu
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

#include <stdbool.h>
#include "charosd.h"
#include "panel.h"

#if (1) //def CHAROSD_USE_MENU

#include "flightstats.h"
#include "flightstatus.h"
#include "manualcontrolcommand.h"
#include "manualcontrolsettings.h"
#include "vtxinfo.h"
#include "vtxsettings.h"

#if defined(TRIFLIGHT)
	#include "triflightsettings.h"
	#include "triflightstatus.h"
#endif

// These are allocated in the VTXConfig module
//extern const uint16_t *BAND_5G8_A_FREQS;
//extern const uint16_t *BAND_5G8_B_FREQS;
//extern const uint16_t *BAND_5G8_E_FREQS;
//extern const uint16_t *AIRWAVE_FREQS;
//extern const uint16_t *RACEBAND_FREQS;

static const uint16_t BAND_5G8_A_FREQS[VTXSETTINGS_BAND_5G8_A_FREQUENCY_MAXOPTVAL + 1] = {
	[VTXSETTINGS_BAND_5G8_A_FREQUENCY_CH15865] = 5865,
	[VTXSETTINGS_BAND_5G8_A_FREQUENCY_CH25845] = 5845,
	[VTXSETTINGS_BAND_5G8_A_FREQUENCY_CH35825] = 5825,
	[VTXSETTINGS_BAND_5G8_A_FREQUENCY_CH45805] = 5805,
	[VTXSETTINGS_BAND_5G8_A_FREQUENCY_CH55785] = 5785,
	[VTXSETTINGS_BAND_5G8_A_FREQUENCY_CH65765] = 5765,
	[VTXSETTINGS_BAND_5G8_A_FREQUENCY_CH75745] = 5745,
	[VTXSETTINGS_BAND_5G8_A_FREQUENCY_CH85725] = 5725
};

static const uint16_t BAND_5G8_B_FREQS[VTXSETTINGS_BAND_5G8_B_FREQUENCY_MAXOPTVAL + 1] = {
	[VTXSETTINGS_BAND_5G8_B_FREQUENCY_CH15733] = 5733,
	[VTXSETTINGS_BAND_5G8_B_FREQUENCY_CH25752] = 5752,
	[VTXSETTINGS_BAND_5G8_B_FREQUENCY_CH35771] = 5771,
	[VTXSETTINGS_BAND_5G8_B_FREQUENCY_CH45790] = 5790,
	[VTXSETTINGS_BAND_5G8_B_FREQUENCY_CH55809] = 5809,
	[VTXSETTINGS_BAND_5G8_B_FREQUENCY_CH65828] = 5828,
	[VTXSETTINGS_BAND_5G8_B_FREQUENCY_CH75847] = 5847,
	[VTXSETTINGS_BAND_5G8_B_FREQUENCY_CH85866] = 5866
};

static const uint16_t BAND_5G8_E_FREQS[VTXSETTINGS_BAND_5G8_E_FREQUENCY_MAXOPTVAL + 1] = {
	[VTXSETTINGS_BAND_5G8_E_FREQUENCY_CH15705] = 5705,
	[VTXSETTINGS_BAND_5G8_E_FREQUENCY_CH25685] = 5685,
	[VTXSETTINGS_BAND_5G8_E_FREQUENCY_CH35665] = 5665,
	[VTXSETTINGS_BAND_5G8_E_FREQUENCY_CH45645] = 5645,
	[VTXSETTINGS_BAND_5G8_E_FREQUENCY_CH55885] = 5885,
	[VTXSETTINGS_BAND_5G8_E_FREQUENCY_CH65905] = 5905,
	[VTXSETTINGS_BAND_5G8_E_FREQUENCY_CH75925] = 5925,
	[VTXSETTINGS_BAND_5G8_E_FREQUENCY_CH85945] = 5945
};

static const uint16_t AIRWAVE_FREQS[VTXSETTINGS_AIRWAVE_FREQUENCY_MAXOPTVAL + 1] = {
	[VTXSETTINGS_AIRWAVE_FREQUENCY_CH15740] = 5740,
	[VTXSETTINGS_AIRWAVE_FREQUENCY_CH25760] = 5760,
	[VTXSETTINGS_AIRWAVE_FREQUENCY_CH35780] = 5780,
	[VTXSETTINGS_AIRWAVE_FREQUENCY_CH45800] = 5800,
	[VTXSETTINGS_AIRWAVE_FREQUENCY_CH55820] = 5820,
	[VTXSETTINGS_AIRWAVE_FREQUENCY_CH65840] = 5840,
	[VTXSETTINGS_AIRWAVE_FREQUENCY_CH75860] = 5860,
	[VTXSETTINGS_AIRWAVE_FREQUENCY_CH85880] = 5880
};

static const uint16_t RACEBAND_FREQS[VTXSETTINGS_RACEBAND_FREQUENCY_MAXOPTVAL + 1] = {
	[VTXSETTINGS_RACEBAND_FREQUENCY_CH15658] = 5658,
	[VTXSETTINGS_RACEBAND_FREQUENCY_CH25695] = 5695,
	[VTXSETTINGS_RACEBAND_FREQUENCY_CH35732] = 5732,
	[VTXSETTINGS_RACEBAND_FREQUENCY_CH45769] = 5769,
	[VTXSETTINGS_RACEBAND_FREQUENCY_CH55806] = 5806,
	[VTXSETTINGS_RACEBAND_FREQUENCY_CH65843] = 5843,
	[VTXSETTINGS_RACEBAND_FREQUENCY_CH75880] = 5880,
	[VTXSETTINGS_RACEBAND_FREQUENCY_CH85917] = 5917
};

static const uint16_t VTX_POWER[VTXSETTINGS_POWER_GLOBAL_MAXOPTVAL + 1] = {
	[VTXSETTINGS_POWER_25]  = 25,
	[VTXSETTINGS_POWER_200] = 200,
	[VTXSETTINGS_POWER_500] = 500,
	[VTXSETTINGS_POWER_800] = 800,
};

// Events that can be be injected into the FSM and trigger state changes
enum menu_fsm_event {
	FSM_EVENT_AUTO,           /*!< Fake event to auto transition to the next state */
	FSM_EVENT_UP,
	FSM_EVENT_DOWN,
	FSM_EVENT_LEFT,
	FSM_EVENT_RIGHT,
	FSM_EVENT_NUM_EVENTS
};

// FSM States
enum menu_fsm_state {
	FSM_STATE_FAULT,                /*!< Invalid state transition occurred */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_MAIN_STATS,           /*!< Flight Stats */
	FSM_STATE_MAIN_TRIFLIGHT,       /*!< TriFlight */
	FSM_STATE_MAIN_VTX,             /*!< Video Transmitter */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_STATS_EXIT,           /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_TRIFLIGHT_EXIT,       /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_VTX_BAND,             /*!< Set Band */
	FSM_STATE_VTX_CH,               /*!< Set Channel */
	FSM_STATE_VTX_POWER,            /*!< Set Power */
	FSM_STATE_VTX_APPLY,            /*!< Apply Settig */
	FSM_STATE_VTX_SAVEEXIT,         /*!< Save & Exit */
	FSM_STATE_VTX_EXIT,             /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_NUM_STATES
};

#define FSM_STATE_TOP FSM_STATE_MAIN_STATS

// Structure for the FSM
struct menu_fsm_transition {
	void (*menu_fn)();     /*!< Called while in a state */
	enum menu_fsm_state next_state[FSM_EVENT_NUM_EVENTS];
};

extern uint16_t frame_counter;
static enum menu_fsm_state current_state = FSM_STATE_TOP;
static enum menu_fsm_event current_event = FSM_EVENT_AUTO;
static bool held_long;

static void main_menu(charosd_state_t state);
static void stats_menu(charosd_state_t state);
static void triflight_menu(charosd_state_t state);
static void vtx_menu(charosd_state_t state);

static char buffer[50];

// The Menu FSM
const static struct menu_fsm_transition menu_fsm[FSM_STATE_NUM_STATES] = {
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_MAIN_STATS] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_VTX,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_TRIFLIGHT,
			[FSM_EVENT_RIGHT] = FSM_STATE_STATS_EXIT,
		},
	},
	[FSM_STATE_MAIN_TRIFLIGHT] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_STATS,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_VTX,
			[FSM_EVENT_RIGHT] = FSM_STATE_TRIFLIGHT_EXIT,
		},
	},
	[FSM_STATE_MAIN_VTX] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_STATS,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_STATS,
			[FSM_EVENT_RIGHT] = FSM_STATE_VTX_BAND,
		},
	},
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_STATS_EXIT] = {
		.menu_fn = stats_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STATS_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_STATS_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_STATS,
		},
	},
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_TRIFLIGHT_EXIT] = {
		.menu_fn = triflight_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_TRIFLIGHT_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_TRIFLIGHT_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_TRIFLIGHT,
		},
	},
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_VTX_BAND] = {
		.menu_fn = vtx_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_VTX_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_VTX_CH,
		},
	},
	[FSM_STATE_VTX_CH] = {
		.menu_fn = vtx_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_VTX_BAND,
			[FSM_EVENT_DOWN] = FSM_STATE_VTX_POWER,
		},
	},
	[FSM_STATE_VTX_POWER] = {
		.menu_fn = vtx_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_VTX_CH,
			[FSM_EVENT_DOWN] = FSM_STATE_VTX_APPLY,
		},
	},
	[FSM_STATE_VTX_APPLY] = {
		.menu_fn = vtx_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_VTX_POWER,
			[FSM_EVENT_DOWN] = FSM_STATE_VTX_SAVEEXIT,
		},
	},
	[FSM_STATE_VTX_SAVEEXIT] = {
		.menu_fn = vtx_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_VTX_APPLY,
			[FSM_EVENT_DOWN] = FSM_STATE_VTX_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_VTX,
		},
	},
	[FSM_STATE_VTX_EXIT] = {
		.menu_fn = vtx_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_VTX_SAVEEXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_VTX_BAND,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_VTX,
		},
	},
/*------------------------------------------------------------------------------------------*/
};

#define INPUT_THRESHOLD (0.2f)

enum menu_fsm_event get_controller_event()
{
	float roll, pitch;
	ManualControlCommandRollGet(&roll);
	ManualControlCommandPitchGet(&pitch);

	// pitch has priority
	if (pitch < -1 * INPUT_THRESHOLD)
		return FSM_EVENT_UP;
	if (pitch > INPUT_THRESHOLD)
		return FSM_EVENT_DOWN;
	if (roll < -1 * INPUT_THRESHOLD)
		return FSM_EVENT_LEFT;
	if (roll > INPUT_THRESHOLD)
		return FSM_EVENT_RIGHT;

	return FSM_EVENT_AUTO;
}

#define HELD_LONG_THRESHOLD 20

void render_charosd_menu(charosd_state_t state)
{
//	uint8_t tmp;
	static enum menu_fsm_event last_fsm_event;
	static uint32_t event_repeats = 0;

	PIOS_MAX7456_clear(state->dev);
	
	if (frame_counter % 6 == 0) {
		current_event = get_controller_event();
		if (last_fsm_event == current_event) {
			event_repeats += 1;
		}
		else {
			event_repeats = 0;
		}
		last_fsm_event = current_event;
		held_long = (event_repeats > HELD_LONG_THRESHOLD);
	}
	else {
		current_event = FSM_EVENT_AUTO;
	}

	if (menu_fsm[current_state].menu_fn)
		menu_fsm[current_state].menu_fn(state);
	
	// transition to the next state, but if it's the first right event (effectively
	// debounces the exit from sub-menu to main menu transition)
	if ((!event_repeats || current_event != FSM_EVENT_RIGHT) && menu_fsm[current_state].next_state[current_event])
		current_state = menu_fsm[current_state].next_state[current_event];

//	ManualControlSettingsArmingGet(&tmp);

//	switch(tmp){
//		case MANUALCONTROLSETTINGS_ARMING_ROLLLEFTTHROTTLE:
//		case MANUALCONTROLSETTINGS_ARMING_ROLLRIGHTTHROTTLE:
//			strcpy(buffer, "Do not use roll for arming!!");
//			terminate_buffer();
//			PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 12, buffer, 0);
//			break;
//		default:
//			strcpy(buffer, "Use pitch and roll to select");
//			terminate_buffer();
//			PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 12, buffer, 0);
//	}

//	FlightStatusArmedGet(&tmp);
//	if (tmp != FLIGHTSTATUS_ARMED_DISARMED) {
//			strcpy(buffer, "WARNING: ARMED!!");
//			terminate_buffer();
//			PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 13, buffer, 0);
//	}
}

void main_menu(charosd_state_t state)
{
	int y_pos = 2;

	strcpy(buffer, "Main Menu");
	terminate_buffer();
	PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 1, buffer, 0);

	for (enum menu_fsm_state s=FSM_STATE_TOP; s <= FSM_STATE_MAIN_VTX; s++) {
		if (s == current_state) {
			strcpy(buffer, ">");
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
		}

		switch (s) {
			case FSM_STATE_MAIN_STATS:
				strcpy(buffer, "Flight Statistics");
				terminate_buffer();
				PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);
				break;
			case FSM_STATE_MAIN_TRIFLIGHT:
				strcpy(buffer, "TriFlight");
				terminate_buffer();
				PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);
				break;
			case FSM_STATE_MAIN_VTX:
				strcpy(buffer, "Video Transmitter");
				terminate_buffer();
				PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);
				break;
			default:
				break;
		}
		y_pos++;
	}
}

void stats_menu(charosd_state_t state)
{
	float tmp;
	char buffer[100] = { 0 };
	uint8_t y_pos = 2;

	strcpy(buffer, "Flight Statistics");
	terminate_buffer();
	PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 1, buffer, 0);

	if (FlightStatsHandle() == NULL)
	{
		y_pos++;
		
		strcpy(buffer, "Flight Statistics Module");
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, y_pos, buffer, 0);
		y_pos++;
		
		strcpy(buffer, "Not Running!");
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, y_pos, buffer, 0);
		y_pos++;
	}
	else
	{
		FlightStatsData stats;
		FlightStatsGet(&stats);
	
		if (state->available & HAS_NAV) {
			tmp = convert_distance * stats.DistanceTravelled;
			if (tmp < convert_distance_divider)
				sprintf(buffer, "Distance traveled:        %d %s", (int)tmp, dist_unit_short);
			else
				sprintf(buffer, "Distance traveled:        %0.2f %s", (double)(tmp / convert_distance_divider), dist_unit_long);

			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
		
			y_pos++;

			tmp = convert_distance * stats.MaxDistanceToHome;
			if (tmp < convert_distance_divider)
				sprintf(buffer, "Maximum distance to home: %d %s", (int)tmp, dist_unit_short);
			else
				sprintf(buffer, "Maximum distance to home: %0.2f %s", (double)(tmp / convert_distance_divider), dist_unit_long);

			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
		
			y_pos++;

			tmp = convert_distance * stats.MaxAltitude;
			sprintf(buffer, "Maximum altitude:         %0.2f %s", (double)tmp, dist_unit_short);

			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
		
			y_pos++;

			tmp = convert_speed * stats.MaxGroundSpeed;
			sprintf(buffer, "Maximum ground speed:     %0.2f %s", (double)tmp, speed_unit);

			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
		
			y_pos++;
		}

		if (state->available & HAS_BATT) {
			sprintf(buffer, "Energy used:     %d mAh", stats.ConsumedEnergy);

			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
		
			y_pos++;

			sprintf(buffer, "Init batt volts: %0.1f V", (double)stats.InitialBatteryVoltage / 1000.);

			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
		
			y_pos++;
		}
	}
	
	current_state = FSM_STATE_STATS_EXIT;

	y_pos++;

	strcpy(buffer, ">Exit");
	terminate_buffer();
	PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
}

void triflight_menu(charosd_state_t state)
{
	uint8_t y_pos = 2;

	strcpy(buffer, "TriFlight");
	terminate_buffer();
	PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 1, buffer, 0);

#if !defined(TRIFLIGHT)

	y_pos++;
		
	strcpy(buffer, "Not a TriFlight Target");
	terminate_buffer();
	PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, y_pos, buffer, 0);
	y_pos++;	

#else

	if ((TriflightSettingsHandle() == NULL) || (TriflightStatusHandle() == NULL))
	{
		y_pos++;
		
		strcpy(buffer, "TriFlight Module(s)");
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, y_pos, buffer, 0);
		y_pos++;
		
		strcpy(buffer, "Not Running!");
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, y_pos, buffer, 0);
		y_pos++;
	}
	else
	{
		TriflightSettingsData tf_settings;
		TriflightSettingsGet(&tf_settings);
		
		TriflightStatusData tf_status;
		TriflightStatusGet(&tf_status);
	
		if (tf_status.Initialized == TRIFLIGHTSTATUS_INITIALIZED_FALSE)
		{
			y_pos++;
		
			strcpy(buffer, "TriFlight Not");
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, y_pos, buffer, 0);
			y_pos++;

			strcpy(buffer, "Initialized");
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, y_pos, buffer, 0);
			y_pos++;		
		}
		else
		{
			sprintf(buffer, "Motor Thrust Factor: %3.1f", (double)tf_settings.MotorThrustFactor);
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
			y_pos++;
		
			sprintf(buffer, "Servo Angle: %3d Degrees", (int)tf_status.ServoAngle);
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
			y_pos++;
			
			sprintf(buffer, "Servo Speed: %d DPS", (int)tf_settings.ServoSpeed);
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
			y_pos++;
		}
	}

#endif
	
	current_state = FSM_STATE_TRIFLIGHT_EXIT;
	
	y_pos++;

	strcpy(buffer, ">Exit");
	terminate_buffer();
	PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
}

void vtx_menu(charosd_state_t state)
{
	uint8_t y_pos = 2;
	
	const char *vtx_strings[] = {
		[VTXINFO_MODEL_NONE]             = "No VTX",
		[VTXINFO_MODEL_TBSUNIFYPRO5G8]   = "TBS UPro 5G8",
		[VTXINFO_MODEL_TBSUNIFYPRO5G8HV] = "TBS UPro 5G8 HV",
	};

	const char *band_strings[] = {
		[VTXSETTINGS_BAND_BAND5G8A] = "A",
		[VTXSETTINGS_BAND_BAND5G8B] = "B",
		[VTXSETTINGS_BAND_BAND5G8E] = "E",
		[VTXSETTINGS_BAND_AIRWAVE]  = "Airwave",
		[VTXSETTINGS_BAND_RACEBAND] = "Raceband",
	};

	// this is a special case, we need a local copy as the user has to use "apply" to set the new settings
	static VTXSettingsData settings;
	static bool settings_read = false;

	strcpy(buffer, "Video Transmitter");
	terminate_buffer();
	PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 1, buffer, 0);

	if ((VTXSettingsHandle() == NULL) || (VTXInfoHandle() == NULL))
	{
		y_pos++;
		
		strcpy(buffer, "VTX Module Not Running!");
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 3, buffer, 0);
		y_pos++;
		
		current_state = FSM_STATE_VTX_EXIT;
	}
	else
	{
		VTXInfoData vtxInfo;
		VTXInfoGet(&vtxInfo);
		
		if (!settings_read) {
			VTXSettingsGet(&settings);
			settings_read = true;
		}

		sprintf(buffer, "VTX Type:  %s", vtx_strings[vtxInfo.Model]);
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);
		y_pos++;

		sprintf(buffer, "Frequency: %d MHz", vtxInfo.Frequency);
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);
		y_pos++;

		sprintf(buffer, "Power:     %d mW", vtxInfo.Power);
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);
		y_pos++;
		y_pos++;

		sprintf(buffer, "New Band:    %s", band_strings[settings.Band]);
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);

		if (current_state == FSM_STATE_VTX_BAND) {
			strcpy(buffer, ">");
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);

			if (current_event == FSM_EVENT_RIGHT) {
				if (settings.Band == VTXSETTINGS_BAND_MAXOPTVAL) {
					settings.Band = 0;
				}
				else {
					settings.Band += 1;
				}
			}
			if (current_event == FSM_EVENT_LEFT) {
				if (settings.Band == 0) {
					settings.Band = VTXSETTINGS_BAND_MAXOPTVAL;
				}
				else {
					settings.Band -= 1;
				}
			}
		}
		y_pos++;

		uint8_t *target;
		uint8_t max_val;
		const uint16_t *band_ptr;
		switch(settings.Band) {
			case VTXSETTINGS_BAND_BAND5G8A:
				band_ptr = BAND_5G8_A_FREQS;
				max_val = VTXSETTINGS_BAND_5G8_A_FREQUENCY_MAXOPTVAL;
				target = &settings.Band_5G8_A_Frequency;
				break;
			case VTXSETTINGS_BAND_BAND5G8B:
				band_ptr = BAND_5G8_B_FREQS;
				max_val = VTXSETTINGS_BAND_5G8_B_FREQUENCY_MAXOPTVAL;
				target = &settings.Band_5G8_B_Frequency;
				break;
			case VTXSETTINGS_BAND_BAND5G8E:
				band_ptr = BAND_5G8_E_FREQS;
				max_val = VTXSETTINGS_BAND_5G8_E_FREQUENCY_MAXOPTVAL;
				target = &settings.Band_5G8_E_Frequency;
				break;
			case VTXSETTINGS_BAND_AIRWAVE:
				band_ptr = AIRWAVE_FREQS;
				max_val = VTXSETTINGS_AIRWAVE_FREQUENCY_MAXOPTVAL;
				target = &settings.Airwave_Frequency;
				break;
			default:
			case VTXSETTINGS_BAND_RACEBAND:
				band_ptr = RACEBAND_FREQS;
				max_val = VTXSETTINGS_RACEBAND_FREQUENCY_MAXOPTVAL;
				target = &settings.Raceband_Frequency;
				break;
		}
		
		sprintf(buffer, "New Channel: CH%d %d MHz", *target + 1, band_ptr[*target]);
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);

		if (current_state == FSM_STATE_VTX_CH) {
			strcpy(buffer, ">");
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);

			if (current_event == FSM_EVENT_RIGHT) {
				if (*target == max_val) {
					*target = 0;
				}
				else {
					*target += 1;
				}
			}
			if (current_event == FSM_EVENT_LEFT) {
				if (*target == 0) {
					*target = max_val;
				}
				else {
					*target -= 1;
				}
			}
		}
		y_pos++;

		sprintf(buffer, "New Power:   %d mW", VTX_POWER[settings.Power]);
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);

		if (current_state == FSM_STATE_VTX_POWER) {
			strcpy(buffer, ">");
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);

			if (current_event == FSM_EVENT_RIGHT) {
				if (settings.Power == VTXSETTINGS_POWER_MAXOPTVAL) {
					settings.Power = 0;
				}
				else {
					settings.Power += 1;
				}
			}
			if (current_event == FSM_EVENT_LEFT) {
				if (settings.Power == 0) {
					settings.Power = VTXSETTINGS_POWER_MAXOPTVAL;
				}
				else {
					settings.Power -= 1;
				}
			}
		}
		y_pos++;

		strcpy(buffer, "Apply");
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);

		if (current_state == FSM_STATE_VTX_APPLY) {
			strcpy(buffer, ">");
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);

			if (current_event == FSM_EVENT_RIGHT) {
				VTXSettingsSet(&settings);
			}
		}
		y_pos++;

		strcpy(buffer, "Save and Exit");
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);

		if (current_state == FSM_STATE_VTX_SAVEEXIT) {
			strcpy(buffer, ">");
			terminate_buffer();
			PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);

			if (current_event == FSM_EVENT_RIGHT) {
				VTXSettingsSet(&settings);
				UAVObjSave(VTXSettingsHandle(), 0);
				settings_read = false;
			}
		}
	}
	y_pos++;

	strcpy(buffer, "Exit");
	terminate_buffer();
	PIOS_MAX7456_puts(state->dev, 2, y_pos, buffer, 0);

	if (current_state == FSM_STATE_VTX_EXIT) {
		strcpy(buffer, ">");
		terminate_buffer();
		PIOS_MAX7456_puts(state->dev, 1, y_pos, buffer, 0);
		
		if (current_event == FSM_EVENT_RIGHT)
			settings_read = false;
	}
}

#endif /* CHAROSD_USE_MENU */

/**
 * @}
 * @}
 */
