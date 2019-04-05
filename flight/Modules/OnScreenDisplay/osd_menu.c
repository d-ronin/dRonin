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

#include "osd_utils.h"
#include "onscreendisplay.h"

#ifdef OSD_USE_MENU

#include "actuatorsettings.h"
#include "flightstatus.h"
#include "flightbatterystate.h"
#include "flightbatterysettings.h"
#include "homelocation.h"
#include "manualcontrolcommand.h"
#include "manualcontrolsettings.h"
#include "rgbledsettings.h"
#include "stabilizationsettings.h"
#include "stateestimation.h"
#include "vtxinfo.h"
#include "vtxsettings.h"

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
	FSM_STATE_FAULT,            /*!< Invalid state transition occurred */
/*------------------------------------------------------------------------------------------*/
#if defined(USE_STM32F4xx_BRAINFPVRE1)
	FSM_STATE_MAIN_RE1,        /*!< RE1 Specific Settings */
#endif
	FSM_STATE_MAIN_RGBLEDS,     /*!< RGB LED Settings */
	FSM_STATE_MAIN_FILTER,      /*!< Filter Settings */
	FSM_STATE_MAIN_FMODE,       /*!< Flight Mode Settings */
	FSM_STATE_MAIN_HOMELOC,     /*!< Home Location */
	FSM_STATE_MAIN_PIDRATE,     /*!< PID Rate*/
	FSM_STATE_MAIN_PIDATT,      /*!< PID Attitude*/
	FSM_STATE_MAIN_STICKLIMITS, /*!< Stick Range and Limits */
	FSM_STATE_MAIN_VTX,         /*!< Video Transmitter */
	FSM_STATE_MAIN_STATS,       /*!< Flight Stats */
	FSM_STATE_MAIN_BATTERY,     /*!< battery menu */
/*------------------------------------------------------------------------------------------*/
#if defined(USE_STM32F4xx_BRAINFPVRE1)
	FSM_STATE_RE1_IR_PROTOCOL,    /*!< IR transponder protocol */
	FSM_STATE_RE1_IR_IDILAP,      /*!< I-Lap ID */
	FSM_STATE_RE1_IR_IDTRACKMATE, /*!< Trackmate ID */
	FSM_STATE_RE1_SAVEEXIT,       /*!< Save & Exit */
	FSM_STATE_RE1_EXIT,           /*!< Exit */
#endif
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_RGB_DEFAULTCOLOR,    /*!< RGB LED default color */
	FSM_STATE_RGB_RANGECOLOR_BASE, /*!< Range base color */
	FSM_STATE_RGB_RANGECOLOR_END,  /*!< Range end color */
	FSM_STATE_RGB_SAVEEXIT,        /*!< Save & Exit */
	FSM_STATE_RGB_EXIT,            /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_FILTER_ATT,      /*!< Attitude Filter */
	FSM_STATE_FILTER_NAV,      /*!< Navigation Filter */
	FSM_STATE_FILTER_SAVEEXIT, /*!< Save & Exit */
	FSM_STATE_FILTER_EXIT,     /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_FMODE_1,         /*!< Flight Mode Position 1 */
	FSM_STATE_FMODE_2,         /*!< Flight Mode Position 2 */
	FSM_STATE_FMODE_3,         /*!< Flight Mode Position 3 */
	FSM_STATE_FMODE_4,         /*!< Flight Mode Position 4 */
	FSM_STATE_FMODE_5,         /*!< Flight Mode Position 5 */
	FSM_STATE_FMODE_6,         /*!< Flight Mode Position 6 */
	FSM_STATE_FMODE_SAVEEXIT,  /*!< Save & Exit */
	FSM_STATE_FMODE_EXIT,      /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_HOMELOC_SET,     /*!< Set home location */
	FSM_STATE_HOMELOC_EXIT,    /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_PIDRATE_ROLLP,      /*!< Roll P Gain */
	FSM_STATE_PIDRATE_ROLLI,      /*!< Roll I Gain */
	FSM_STATE_PIDRATE_ROLLD,      /*!< Roll D Gain */
	FSM_STATE_PIDRATE_ROLLILIMIT, /*!< Roll I Limit */
	FSM_STATE_PIDRATE_PITCHP,     /*!< Pitch P Gain */
	FSM_STATE_PIDRATE_PITCHI,     /*!< Pitch I Gain */
	FSM_STATE_PIDRATE_PITCHD,     /*!< Pitch D Gain */
	FSM_STATE_PIDRATE_PITCHILIMIT,/*!< Pitch I Limit */
	FSM_STATE_PIDRATE_YAWP,       /*!< Yaw P Gain */
	FSM_STATE_PIDRATE_YAWI,       /*!< Yaw I Gain */
	FSM_STATE_PIDRATE_YAWD,       /*!< Yaw D Gain */
	FSM_STATE_PIDRATE_YAWILIMIT,  /*!< Yaw I Limit */
	FSM_STATE_PIDRATE_SAVEEXIT,   /*!< Save & Exit */
	FSM_STATE_PIDRATE_EXIT,       /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_PIDATT_ROLLP,      /*!< Roll P Gain */
	FSM_STATE_PIDATT_ROLLI,      /*!< Roll I Gain */
	FSM_STATE_PIDATT_ROLLILIMIT, /*!< Roll I Limit */
	FSM_STATE_PIDATT_PITCHP,     /*!< Pitch P Gain */
	FSM_STATE_PIDATT_PITCHI,     /*!< Pitch I Gain */
	FSM_STATE_PIDATT_PITCHILIMIT,/*!< Pitch I Limit */
	FSM_STATE_PIDATT_YAWP,       /*!< Yaw P Gain */
	FSM_STATE_PIDATT_YAWI,       /*!< Yaw I Gain */
	FSM_STATE_PIDATT_YAWILIMIT,  /*!< Yaw I Limit */
	FSM_STATE_PIDATT_SAVEEXIT,   /*!< Save & Exit */
	FSM_STATE_PIDATT_EXIT,       /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_STICKLIMITS_ROLLA,    /*!< Roll full stick angle */
	FSM_STATE_STICKLIMITS_PITCHA,   /*!< Pitch full stick angle */
	FSM_STATE_STICKLIMITS_YAWA,     /*!< Yaw full stick angle */
	FSM_STATE_STICKLIMITS_ROLLR,    /*!< Roll full stick rate */
	FSM_STATE_STICKLIMITS_PITCHR,   /*!< Pitch full stick rate */
	FSM_STATE_STICKLIMITS_YAWR,     /*!< Yaw full stick rate */
	FSM_STATE_STICKLIMITS_ROLLEXR,  /*!< Roll expo rate */
	FSM_STATE_STICKLIMITS_PITCHEXR, /*!< Pitch expo rate */
	FSM_STATE_STICKLIMITS_YAWEXR,   /*!< Yaw expo rate */
	FSM_STATE_STICKLIMITS_ROLLEXH,  /*!< Roll expo horizon */
	FSM_STATE_STICKLIMITS_PITCHEXH, /*!< Pitch expo horizon */
	FSM_STATE_STICKLIMITS_YAWEXH,   /*!< Yaw expo horizon */
	FSM_STATE_STICKLIMITS_MOTORPOWER, /*!< Motor power gain scale */
	FSM_STATE_STICKLIMITS_SAVEEXIT, /*!< Save & Exit */
	FSM_STATE_STICKLIMITS_EXIT,     /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_VTX_BAND,             /*!< Set Band */
	FSM_STATE_VTX_CH,               /*!< Set Channel */
	FSM_STATE_VTX_POWER,            /*!< Set Power */
	FSM_STATE_VTX_APPLY,            /*!< Apply Settig */
	FSM_STATE_VTX_SAVEEXIT,         /*!< Save & Exit */
	FSM_STATE_VTX_EXIT,             /*!< Exit */
/*------------------------------------------------------------------------------------------*/
	FSM_STATE_STATS_EXIT,           /*!< Exit */
/*------------------------------------------------------------------------------------------*/
        FSM_STATE_BATTERY_CAPACITY,    /*!< Battery capacity setting */
        FSM_STATE_BATTERY_CELLCOUNT,   /*!< Battery cell count setting */
        FSM_STATE_BATTERY_WARNING,     /*!< Low cell warning setting */
        FSM_STATE_BATTERY_ALARM,       /*!< Low cell alarm setting */
        FSM_STATE_BATTERY_WARN_TIME,   /*!< Low cell warning setting */
        FSM_STATE_BATTERY_ALARM_TIME,  /*!< Low cell alarm setting */
        FSM_STATE_BATTERY_SAVEEXIT,    /*!< Save & Exit */
        FSM_STATE_BATTERY_EXIT,        /*!< Exit */
/*------------------------------------------------------------------------------------------*/

	FSM_STATE_NUM_STATES
};

#if defined(USE_STM32F4xx_BRAINFPVRE1)
	static void brainre1_menu(void);
#define FSM_STATE_TOP FSM_STATE_MAIN_RE1
#else
#define FSM_STATE_TOP FSM_STATE_MAIN_RGBLEDS
#endif /* defined(USE_STM32F4xx_BRAINFPVRE1) */

// Structure for the FSM
struct menu_fsm_transition {
	void (*menu_fn)();     /*!< Called while in a state */
	enum menu_fsm_state next_state[FSM_EVENT_NUM_EVENTS];
};

extern uint16_t frame_counter;
static enum menu_fsm_state current_state = FSM_STATE_TOP;
static enum menu_fsm_event current_event = FSM_EVENT_AUTO;
static bool held_long;


static void main_menu(void);
static void rgbled_menu(void);
static void filter_menu(void);
static void flightmode_menu(void);
static void homeloc_menu(void);
static void pidrate_menu(void);
static void pidatt_menu(void);
static void sticklimits_menu(void);
static void vtx_menu();
static void stats_menu(void);
static void battery_menu(void);



// The Menu FSM
const static struct menu_fsm_transition menu_fsm[FSM_STATE_NUM_STATES] = {
#if defined(USE_STM32F4xx_BRAINFPVRE1)
	[FSM_STATE_MAIN_RE1] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_BATTERY,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_RGBLEDS,
			[FSM_EVENT_RIGHT] = FSM_STATE_RE1_IR_PROTOCOL,
		},
	},
#endif
	[FSM_STATE_MAIN_RGBLEDS] = {
		.menu_fn = main_menu,
		.next_state = {
		#if defined(USE_STM32F4xx_BRAINFPVRE1)
					[FSM_EVENT_UP] = FSM_STATE_MAIN_RE1,
		#else
					[FSM_EVENT_UP] = FSM_STATE_MAIN_BATTERY,
		#endif
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_FILTER,
			[FSM_EVENT_RIGHT] = FSM_STATE_RGB_DEFAULTCOLOR,
		},
	},
	[FSM_STATE_MAIN_FILTER] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_RGBLEDS,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_FMODE,
			[FSM_EVENT_RIGHT] = FSM_STATE_FILTER_ATT,
		},
	},
	[FSM_STATE_MAIN_FMODE] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_FILTER,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_HOMELOC,
			[FSM_EVENT_RIGHT] = FSM_STATE_FMODE_1,
		},
	},
	[FSM_STATE_MAIN_HOMELOC] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_FMODE,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_PIDRATE,
			[FSM_EVENT_RIGHT] = FSM_STATE_HOMELOC_SET,
		},
	},
	[FSM_STATE_MAIN_PIDRATE] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_HOMELOC,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_PIDATT,
			[FSM_EVENT_RIGHT] = FSM_STATE_PIDRATE_ROLLP,
		},
	},
	[FSM_STATE_MAIN_PIDATT] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_PIDRATE,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_STICKLIMITS,
			[FSM_EVENT_RIGHT] = FSM_STATE_PIDATT_ROLLP,
		},
	},
	[FSM_STATE_MAIN_STICKLIMITS] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_PIDATT,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_VTX,
			[FSM_EVENT_RIGHT] = FSM_STATE_STICKLIMITS_ROLLA,
		},
	},
	[FSM_STATE_MAIN_VTX] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_STICKLIMITS,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_STATS,
			[FSM_EVENT_RIGHT] = FSM_STATE_VTX_BAND,
		},
	},
	[FSM_STATE_MAIN_STATS] = {
		.menu_fn = main_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_MAIN_VTX,
			[FSM_EVENT_DOWN] = FSM_STATE_MAIN_BATTERY,
			[FSM_EVENT_RIGHT] = FSM_STATE_STATS_EXIT,
		},
	},
        [FSM_STATE_MAIN_BATTERY] = {
                .menu_fn = main_menu,
                .next_state = {
                        [FSM_EVENT_UP] = FSM_STATE_MAIN_STATS,
                        [FSM_EVENT_DOWN] = FSM_STATE_TOP,
                        [FSM_EVENT_RIGHT] = FSM_STATE_BATTERY_CAPACITY,
                },
        },
/*------------------------------------------------------------------------------------------*/
#if defined(USE_STM32F4xx_BRAINFPVRE1)
	[FSM_STATE_RE1_IR_PROTOCOL] = {
		.menu_fn = brainre1_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RE1_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_RE1_IR_IDILAP,
		},
	},
	[FSM_STATE_RE1_IR_IDILAP] = {
		.menu_fn = brainre1_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RE1_IR_PROTOCOL,
			[FSM_EVENT_DOWN] = FSM_STATE_RE1_IR_IDTRACKMATE,
		},
	},
	[FSM_STATE_RE1_IR_IDTRACKMATE] = {
		.menu_fn = brainre1_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RE1_IR_IDILAP,
			[FSM_EVENT_DOWN] = FSM_STATE_RE1_SAVEEXIT,
		},
	},
	[FSM_STATE_RE1_SAVEEXIT] = {
		.menu_fn = brainre1_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RE1_IR_IDTRACKMATE,
			[FSM_EVENT_DOWN] = FSM_STATE_RE1_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_RE1,
		},
	},
	[FSM_STATE_RE1_EXIT] = {
		.menu_fn = brainre1_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RE1_SAVEEXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_RE1_IR_PROTOCOL,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_RE1,
		},
	},
#endif /* defined(USE_STM32F4xx_BRAINFPVRE1) */
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_RGB_DEFAULTCOLOR] = {
		.menu_fn = rgbled_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RGB_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_RGB_RANGECOLOR_BASE,
		},
	},
	[FSM_STATE_RGB_RANGECOLOR_BASE] = {
		.menu_fn = rgbled_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RGB_DEFAULTCOLOR,
			[FSM_EVENT_DOWN] = FSM_STATE_RGB_RANGECOLOR_END,
		},
	},
	[FSM_STATE_RGB_RANGECOLOR_END] = {
		.menu_fn = rgbled_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RGB_RANGECOLOR_BASE,
			[FSM_EVENT_DOWN] = FSM_STATE_RGB_SAVEEXIT,
		},
	},
	[FSM_STATE_RGB_SAVEEXIT] = {
		.menu_fn = rgbled_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RGB_RANGECOLOR_END,
			[FSM_EVENT_DOWN] = FSM_STATE_RGB_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_RGBLEDS,
		},
	},
	[FSM_STATE_RGB_EXIT] = {
		.menu_fn = rgbled_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_RGB_SAVEEXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_RGB_DEFAULTCOLOR,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_RGBLEDS,
		},
	},
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_FILTER_ATT] = {
		.menu_fn = filter_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FILTER_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_FILTER_NAV,
		},
	},
	[FSM_STATE_FILTER_NAV] = {
		.menu_fn = filter_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FILTER_ATT,
			[FSM_EVENT_DOWN] = FSM_STATE_FILTER_SAVEEXIT,
		},
	},
	[FSM_STATE_FILTER_SAVEEXIT] = {
		.menu_fn = filter_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FILTER_NAV,
			[FSM_EVENT_DOWN] = FSM_STATE_FILTER_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_FILTER,
		},
	},
	[FSM_STATE_FILTER_EXIT] = {
		.menu_fn = filter_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FILTER_SAVEEXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_FILTER_ATT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_FILTER,
		},
	},
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_FMODE_1] = {
		.menu_fn = flightmode_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FMODE_SAVEEXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_FMODE_2,
		},
	},
	[FSM_STATE_FMODE_2] = {
		.menu_fn = flightmode_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FMODE_1,
			[FSM_EVENT_DOWN] = FSM_STATE_FMODE_3,
		},
	},
	[FSM_STATE_FMODE_3] = {
		.menu_fn = flightmode_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FMODE_2,
			[FSM_EVENT_DOWN] = FSM_STATE_FMODE_4,
		},
	},
	[FSM_STATE_FMODE_4] = {
		.menu_fn = flightmode_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FMODE_3,
			[FSM_EVENT_DOWN] = FSM_STATE_FMODE_5,
		},
	},
	[FSM_STATE_FMODE_5] = {
		.menu_fn = flightmode_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FMODE_4,
			[FSM_EVENT_DOWN] = FSM_STATE_FMODE_6,
		},
	},
	[FSM_STATE_FMODE_6] = {
		.menu_fn = flightmode_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FMODE_5,
			[FSM_EVENT_DOWN] = FSM_STATE_FMODE_SAVEEXIT,
		},
	},
	[FSM_STATE_FMODE_SAVEEXIT] = {
		.menu_fn = flightmode_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FMODE_6,
			[FSM_EVENT_DOWN] = FSM_STATE_FMODE_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_FMODE,
		},
	},
	[FSM_STATE_FMODE_EXIT] = {
		.menu_fn = flightmode_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_FMODE_SAVEEXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_FMODE_1,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_FMODE,
		},
	},
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_HOMELOC_SET] = {
		.menu_fn = homeloc_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_HOMELOC_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_HOMELOC_EXIT,
		},
	},
	[FSM_STATE_HOMELOC_EXIT] = {
		.menu_fn = homeloc_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_HOMELOC_SET,
			[FSM_EVENT_DOWN] = FSM_STATE_HOMELOC_SET,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_HOMELOC,
		},
	},
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_PIDRATE_ROLLP] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_ROLLI,
		},
	},
	[FSM_STATE_PIDRATE_ROLLI] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_ROLLP,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_ROLLD,
		},
	},
	[FSM_STATE_PIDRATE_ROLLD] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_ROLLI,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_ROLLILIMIT,
		},
	},
	[FSM_STATE_PIDRATE_ROLLILIMIT] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_ROLLD,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_PITCHP,
		},
	},
	[FSM_STATE_PIDRATE_PITCHP] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_ROLLD,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_PITCHI,
		},
	},
	[FSM_STATE_PIDRATE_PITCHI] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_PITCHP,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_PITCHD,
		},
	},
	[FSM_STATE_PIDRATE_PITCHD] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_PITCHI,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_PITCHILIMIT,
		},
	},
	[FSM_STATE_PIDRATE_PITCHILIMIT] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_PITCHD,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_YAWP,
		},
	},
	[FSM_STATE_PIDRATE_YAWP] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_PITCHD,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_YAWI,
		},
	},
	[FSM_STATE_PIDRATE_YAWI] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_YAWP,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_YAWD,
		},
	},
	[FSM_STATE_PIDRATE_YAWD] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_YAWI,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_YAWILIMIT,
		},
	},
	[FSM_STATE_PIDRATE_YAWILIMIT] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_YAWD,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_SAVEEXIT,
		},
	},
	[FSM_STATE_PIDRATE_SAVEEXIT] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_YAWILIMIT,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_PIDRATE,
		},
	},
	[FSM_STATE_PIDRATE_EXIT] = {
		.menu_fn = pidrate_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDRATE_SAVEEXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDRATE_ROLLP,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_PIDRATE,
		},
	},
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_PIDATT_ROLLP] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_ROLLI,
		},
	},
	[FSM_STATE_PIDATT_ROLLI] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_ROLLP,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_ROLLILIMIT,
		},
	},
	[FSM_STATE_PIDATT_ROLLILIMIT] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_ROLLI,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_PITCHP,
		},
	},
	[FSM_STATE_PIDATT_PITCHP] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_ROLLILIMIT,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_PITCHI,
		},
	},
	[FSM_STATE_PIDATT_PITCHI] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_PITCHP,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_PITCHILIMIT,
		},
	},
	[FSM_STATE_PIDATT_PITCHILIMIT] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_PITCHI,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_YAWP,
		},
	},
	[FSM_STATE_PIDATT_YAWP] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_PITCHILIMIT,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_YAWI,
		},
	},
	[FSM_STATE_PIDATT_YAWI] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_YAWP,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_YAWILIMIT,
		},
	},
	[FSM_STATE_PIDATT_YAWILIMIT] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_YAWI,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_SAVEEXIT,
		},
	},
	[FSM_STATE_PIDATT_SAVEEXIT] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_YAWILIMIT,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_PIDATT,
		},
	},
	[FSM_STATE_PIDATT_EXIT] = {
		.menu_fn = pidatt_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_PIDATT_SAVEEXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_PIDATT_ROLLP,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_PIDATT,
		},
	},
/*------------------------------------------------------------------------------------------*/
	[FSM_STATE_STICKLIMITS_ROLLA] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_PITCHA,
		},
	},
	[FSM_STATE_STICKLIMITS_PITCHA] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_ROLLA,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_YAWA,
		},
	},
	[FSM_STATE_STICKLIMITS_YAWA] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_PITCHA,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_ROLLR,
		},
	},
	[FSM_STATE_STICKLIMITS_ROLLR] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_YAWA,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_PITCHR,
		},
	},
	[FSM_STATE_STICKLIMITS_PITCHR] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_ROLLR,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_YAWR,
		},
	},
	[FSM_STATE_STICKLIMITS_YAWR] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_PITCHR,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_ROLLEXR,
		},
	},
	[FSM_STATE_STICKLIMITS_ROLLEXR] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_YAWR,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_PITCHEXR,
		},
	},
	[FSM_STATE_STICKLIMITS_PITCHEXR] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_ROLLEXR,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_YAWEXR,
		},
	},
	[FSM_STATE_STICKLIMITS_YAWEXR] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_PITCHEXR,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_ROLLEXH,
		},
	},
	[FSM_STATE_STICKLIMITS_ROLLEXH] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_YAWEXR,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_PITCHEXH,
		},
	},
	[FSM_STATE_STICKLIMITS_PITCHEXH] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_ROLLEXH,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_YAWEXH,
		},
	},
	[FSM_STATE_STICKLIMITS_YAWEXH] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_PITCHEXH,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_MOTORPOWER,
		},
	},
	[FSM_STATE_STICKLIMITS_MOTORPOWER] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_YAWEXH,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_SAVEEXIT,
		},
	},
	[FSM_STATE_STICKLIMITS_SAVEEXIT] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_MOTORPOWER,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_STICKLIMITS,
		},
	},
	[FSM_STATE_STICKLIMITS_EXIT] = {
		.menu_fn = sticklimits_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STICKLIMITS_SAVEEXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_STICKLIMITS_ROLLA,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_STICKLIMITS,
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
	[FSM_STATE_STATS_EXIT] = {
		.menu_fn = stats_menu,
		.next_state = {
			[FSM_EVENT_UP] = FSM_STATE_STATS_EXIT,
			[FSM_EVENT_DOWN] = FSM_STATE_STATS_EXIT,
			[FSM_EVENT_RIGHT] = FSM_STATE_MAIN_STATS,
		},
	},
/*------------------------------------------------------------------------------------------*/
        [FSM_STATE_BATTERY_CAPACITY] = {
                .menu_fn = battery_menu,
                .next_state = {
                        [FSM_EVENT_UP] = FSM_STATE_BATTERY_EXIT,
                        [FSM_EVENT_DOWN] = FSM_STATE_BATTERY_CELLCOUNT,
                },
        },
        [FSM_STATE_BATTERY_CELLCOUNT] = {
                .menu_fn = battery_menu,
                .next_state = {
                        [FSM_EVENT_UP] = FSM_STATE_BATTERY_CAPACITY,
                        [FSM_EVENT_DOWN] = FSM_STATE_BATTERY_WARNING,
                },
        },
        [FSM_STATE_BATTERY_WARNING] = {
                .menu_fn = battery_menu,
                .next_state = {
                        [FSM_EVENT_UP] = FSM_STATE_BATTERY_CELLCOUNT,
                        [FSM_EVENT_DOWN] = FSM_STATE_BATTERY_ALARM,
                },
        },
        [FSM_STATE_BATTERY_ALARM] = {
                .menu_fn = battery_menu,
                .next_state = {
                        [FSM_EVENT_UP] = FSM_STATE_BATTERY_WARNING,
                        [FSM_EVENT_DOWN] = FSM_STATE_BATTERY_WARN_TIME,
                },
        },
        [FSM_STATE_BATTERY_WARN_TIME] = {
                .menu_fn = battery_menu,
                .next_state = {
                        [FSM_EVENT_UP] = FSM_STATE_BATTERY_ALARM,
                        [FSM_EVENT_DOWN] = FSM_STATE_BATTERY_ALARM_TIME,
                },
        },
        [FSM_STATE_BATTERY_ALARM_TIME] = {
                .menu_fn = battery_menu,
                .next_state = {
                        [FSM_EVENT_UP] = FSM_STATE_BATTERY_WARN_TIME,
                        [FSM_EVENT_DOWN] = FSM_STATE_BATTERY_SAVEEXIT,
                },
        },
        [FSM_STATE_BATTERY_SAVEEXIT] = {
                .menu_fn = battery_menu,
                .next_state = {
                        [FSM_EVENT_UP] = FSM_STATE_BATTERY_ALARM,
                        [FSM_EVENT_DOWN] = FSM_STATE_BATTERY_EXIT,
                        [FSM_EVENT_RIGHT] = FSM_STATE_MAIN_BATTERY,
                },
        },
        [FSM_STATE_BATTERY_EXIT] = {
                .menu_fn = battery_menu,
                .next_state = {
                        [FSM_EVENT_UP] = FSM_STATE_BATTERY_SAVEEXIT,
                        [FSM_EVENT_DOWN] = FSM_STATE_BATTERY_CAPACITY,
                        [FSM_EVENT_RIGHT] = FSM_STATE_MAIN_BATTERY,
                },
        },

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
void render_osd_menu()
{
	uint8_t tmp;
	static enum menu_fsm_event last_fsm_event;
	static uint32_t event_repeats = 0;

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
		menu_fsm[current_state].menu_fn();

	// transition to the next state, but if it's the first right event (effectively
	// debounces the exit from sub-menu to main menu transition)
	if ((!event_repeats || current_event != FSM_EVENT_RIGHT) && menu_fsm[current_state].next_state[current_event])
		current_state = menu_fsm[current_state].next_state[current_event];

	ManualControlSettingsArmingGet(&tmp);
	switch(tmp){
		case MANUALCONTROLSETTINGS_ARMING_ROLLLEFTTHROTTLE:
		case MANUALCONTROLSETTINGS_ARMING_ROLLRIGHTTHROTTLE:
			write_string("Do not use roll for arming.", GRAPHICS_X_MIDDLE, GRAPHICS_BOTTOM - 25, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, FONT8X10);
			break;
		default:
			write_string("Use roll and pitch to navigate", GRAPHICS_X_MIDDLE, GRAPHICS_BOTTOM - 25, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, FONT8X10);
	}

	FlightStatusArmedGet(&tmp);
	if (tmp != FLIGHTSTATUS_ARMED_DISARMED)
		write_string("WARNING: ARMED", GRAPHICS_X_MIDDLE, GRAPHICS_BOTTOM - 12, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, FONT8X10);
}


#define MENU_LINE_SPACING 11
#define MENU_LINE_Y 40
#define MENU_LINE_X (GRAPHICS_LEFT + 10)
#define MENU_FONT FONT8X10

void draw_menu_title(char* title)
{
	write_string(title, GRAPHICS_X_MIDDLE, 10, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, FONT12X18);
}

void draw_selected_icon(int x, int y)
{
	draw_image(x - 5, y - image_menu_icon.height / 2, &image_menu_icon);
}

void main_menu(void)
{
	int y_pos = MENU_LINE_Y;

	draw_menu_title("Main Menu");

	for (enum menu_fsm_state s=FSM_STATE_TOP; s <= FSM_STATE_MAIN_BATTERY; s++) {
		if (s == current_state) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
		}
		switch (s) {
#if defined(USE_STM32F4xx_BRAINFPVRE1)
			case FSM_STATE_MAIN_RE1:
				write_string("BrainFPV RE1 Settings", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
#endif /* defined(USE_STM32F4xx_BRAINFPVRE1) */
			case FSM_STATE_MAIN_RGBLEDS:
				write_string("RGB LED Settings", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_MAIN_FILTER:
				write_string("Filter Settings", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_MAIN_FMODE:
				write_string("Flight Modes", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_MAIN_HOMELOC:
				write_string("Home Location", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_MAIN_PIDRATE:
				write_string("PID - Rate", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_MAIN_PIDATT:
				write_string("PID - Attitude", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_MAIN_STICKLIMITS:
				write_string("Stick Limits and Expo", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_MAIN_VTX:
				write_string("Video Transmitter", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_MAIN_STATS:
				write_string("Flight Statistics", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_MAIN_BATTERY:
                                write_string("Battery Settings", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
                                break;
			default:
				break;
		}
		y_pos += MENU_LINE_SPACING;
	}
}

#if defined(USE_STM32F4xx_BRAINFPVRE1)
#include "hwbrainre1.h"

const char * ir_protocol_names[HWBRAINRE1_IRPROTOCOL_MAXOPTVAL + 1] = {
	[HWBRAINRE1_IRPROTOCOL_DISABLED] = "OFF",
	[HWBRAINRE1_IRPROTOCOL_ILAP] = "I-Lap",
	[HWBRAINRE1_IRPROTOCOL_TRACKMATE] = "Trackmate"
};

#define MAX_ID_ILAP 9999999
#define MAX_ID_TRACKMATE 0xfff

// Get the next valid trackmate id
// It seems like the ID has some weird requirements:
// 1st nibble is 0, 2nd nibble is not 0, 1, 8, or F
// 3rd and 4th nibbles are not 0 or F
uint16_t next_valid_trackmateid(uint16_t trackmate_id, int16_t step)
{
	uint8_t nibble;
	while (1){
		trackmate_id += step;
		if (trackmate_id > MAX_ID_TRACKMATE) {
			if (step > 0) {
				trackmate_id = 0;
			}
			else {
				trackmate_id = MAX_ID_TRACKMATE;
			}
		}
		// Test 2nd nibble
		nibble = (trackmate_id & 0x0F00) >> 8;
		if ((nibble == 0x00) || (nibble == 0x01) || (nibble == 0x08) || (nibble == 0x0F)) {
			// step through these quickly
			if (step > 0) {
				trackmate_id += 256;
			}
			else {
				trackmate_id -= 256;
			}
			continue;
		}
		// Test 3rd nibble
		nibble = (trackmate_id & 0x00F0) >> 4;
		if ((nibble == 0x00) || (nibble == 0x0F)) {
			continue;
		}
		// Test 4th nibble
		nibble = trackmate_id & 0x000F;
		if ((nibble == 0x00) || (nibble == 0x0F)) {
			continue;
		}
		// We have a valid ID
		break;
	}

	return trackmate_id;
}

void brainre1_menu(void)
{
	HwBrainRE1Data data;
	int y_pos = MENU_LINE_Y;
	char tmp_str[100] = {0};
	bool data_changed = false;

	HwBrainRE1Get(&data);

	draw_menu_title("BrainFPV RE1 Settings");

	for (enum menu_fsm_state s=FSM_STATE_RE1_IR_PROTOCOL; s <= FSM_STATE_RE1_EXIT; s++) {
		if (s == current_state) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
		}
		switch (s) {
			case FSM_STATE_RE1_IR_PROTOCOL:
				sprintf(tmp_str, "IR Transponder Protocol: %s", ir_protocol_names[data.IRProtocol]);
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_RE1_IR_IDILAP:
				sprintf(tmp_str, "IR Transponder ID I-Lap:  %07d", (int)data.IRIDILap);
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_RE1_IR_IDTRACKMATE:
				sprintf(tmp_str, "IR Transponder ID Trackmate: %04d", (int)data.IRIDTrackmate);
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_RE1_SAVEEXIT:
				write_string("Save and Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_RE1_EXIT:
				write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			default:
				break;
		}
		y_pos += MENU_LINE_SPACING;
	}

	switch (current_state) {
		case FSM_STATE_RE1_IR_PROTOCOL:
			if (current_event == FSM_EVENT_RIGHT) {
				if (data.IRProtocol == HWBRAINRE1_IRPROTOCOL_GLOBAL_MAXOPTVAL)
					data.IRProtocol = 0;
				else
					data.IRProtocol += 1;
				data_changed = true;
			}
			if (current_event == FSM_EVENT_LEFT) {
				if (data.IRProtocol == 0)
					data.IRProtocol = HWBRAINRE1_IRPROTOCOL_GLOBAL_MAXOPTVAL;
				else
					data.IRProtocol -= 1;
				data_changed = true;
			}
			break;
		case FSM_STATE_RE1_IR_IDILAP:
			{
				int step = held_long ? 1000 : 1;
				if (current_event == FSM_EVENT_RIGHT) {
					data.IRIDILap += step;
					if (data.IRIDILap > MAX_ID_ILAP) {
						data.IRIDILap = 0;
					}
				}
				if (current_event == FSM_EVENT_LEFT) {
					data.IRIDILap -= step;
					if (data.IRIDILap > MAX_ID_ILAP) {
						data.IRIDILap = MAX_ID_ILAP;
					}
				}

				data_changed = true;
			}
			break;
		case FSM_STATE_RE1_IR_IDTRACKMATE:
			{
				if ((current_event == FSM_EVENT_RIGHT) ||  (current_event == FSM_EVENT_LEFT)) {
					int16_t step = held_long ? 100 : 1;
					if  (current_event == FSM_EVENT_LEFT) {
						step *= -1;
					}
					data.IRIDTrackmate = next_valid_trackmateid(data.IRIDTrackmate, step);
					data_changed = true;
				}
			}
			break;
		default:
			break;
	}

	if (data_changed) {
		HwBrainRE1Set(&data);
	}

	if ((current_state == FSM_STATE_RE1_SAVEEXIT) && (current_event == FSM_EVENT_RIGHT)) {
		// Save and exit
		UAVObjSave(HwBrainRE1Handle(), 0);
	}
}
#endif /* defined(USE_STM32F4xx_BRAINFPVRE1) */



enum RGBColor {
	COLOR_OFF,
	COLOR_WHITE,
	COLOR_RED,
	COLOR_ORANGE,
	COLOR_YELLOW,
	COLOR_GREEN,
	COLOR_AQUA,
	COLOR_BLUE,
	COLOR_PURPLE,
	COLOR_CUSTOM,
	COLOR_MAXCOLOR
};


const uint8_t RGBLED_COLOR_VALUES[COLOR_MAXCOLOR + 1][3] = {
	[COLOR_OFF]    = {0,   0,   0},
	[COLOR_WHITE]  = {255, 255, 255},
	[COLOR_RED]    = {255, 0,   0},
	[COLOR_ORANGE] = {255, 69,   0},
	[COLOR_YELLOW] = {255, 255, 0},
	[COLOR_GREEN]  = {0,   255, 0},
	[COLOR_AQUA]   = {0,   255, 255},
	[COLOR_BLUE]   = {0,   0,   255},
	[COLOR_PURPLE] = {255, 0,   255},
	[COLOR_CUSTOM] = {0,   0,   0}
};

const char * RGBLED_COLOR_NAMES[COLOR_MAXCOLOR + 1] = {
	[COLOR_OFF]    = "OFF",
	[COLOR_WHITE]  = "White",
	[COLOR_RED]    = "Red",
	[COLOR_ORANGE] = "Orange",
	[COLOR_YELLOW] = "Yellow",
	[COLOR_GREEN]  = "Green",
	[COLOR_AQUA]   = "Aqua",
	[COLOR_BLUE]   = "Blue",
	[COLOR_PURPLE] = "Purple",
	[COLOR_CUSTOM] = "Custom"
};


enum RGBColor get_color(uint8_t * rgb)
{
	enum RGBColor color;
	for (color=COLOR_OFF; color<COLOR_MAXCOLOR; color++) {
		if ((rgb[0] == RGBLED_COLOR_VALUES[color][0]) && (rgb[1] == RGBLED_COLOR_VALUES[color][1])
			&& (rgb[2] == RGBLED_COLOR_VALUES[color][2])) {
			return color;
		}
	}
	return COLOR_CUSTOM;
}

enum RGBColor get_next_color(enum RGBColor color)
{
	if ((color == COLOR_PURPLE) || (color == COLOR_CUSTOM)) {
		return COLOR_OFF;
	}
	return color + 1;
}

enum RGBColor get_previous_color(enum RGBColor color)
{
	if (color == COLOR_OFF) {
		return COLOR_PURPLE;
	}
	return color - 1;
}

void rgbled_menu(void)
{
	int y_pos = MENU_LINE_Y;
	char tmp_str[100] = {0};
	bool data_changed = false;

	draw_menu_title("RGB LED Settings");

	if (RGBLEDSettingsHandle()) {
		RGBLEDSettingsData data;
		RGBLEDSettingsGet(&data);
		for (enum menu_fsm_state s=FSM_STATE_RGB_DEFAULTCOLOR; s <= FSM_STATE_RGB_EXIT; s++) {
			if (s == current_state) {
				draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
			}
			switch (s) {
				case FSM_STATE_RGB_DEFAULTCOLOR:
					sprintf(tmp_str, "Default color:   %s", RGBLED_COLOR_NAMES[get_color(data.DefaultColor)]);
					write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
					break;
				case FSM_STATE_RGB_RANGECOLOR_BASE:
					sprintf(tmp_str, "Range color base: %s", RGBLED_COLOR_NAMES[get_color(data.RangeBaseColor)]);
					write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
					break;
				case FSM_STATE_RGB_RANGECOLOR_END:
					sprintf(tmp_str, "Range color end: %s", RGBLED_COLOR_NAMES[get_color(data.RangeEndColor)]);
					write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
					break;
				case FSM_STATE_RGB_SAVEEXIT:
					write_string("Save and Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
					break;
				case FSM_STATE_RGB_EXIT:
					write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
					break;
				default:
					break;
			}
			y_pos += MENU_LINE_SPACING;
		}

		uint8_t * target = NULL;
		enum RGBColor color;
		switch (current_state) {
			case FSM_STATE_RGB_DEFAULTCOLOR:
				target = data.DefaultColor;
			case FSM_STATE_RGB_RANGECOLOR_BASE:
				if (target == NULL) {
					target = data.RangeBaseColor;
				}
			case FSM_STATE_RGB_RANGECOLOR_END:
				if (target == NULL) {
					target = data.RangeEndColor;
				}
				if (current_event == FSM_EVENT_RIGHT) {
					color = get_next_color(get_color(target));
					data_changed = true;
				}
				if (current_event == FSM_EVENT_LEFT) {
					color = get_previous_color(get_color(target));
					data_changed = true;
				}
				break;
			default:
				break;
		}

		if (data_changed) {
			if (target != NULL) {
				target[0] = RGBLED_COLOR_VALUES[color][0];
				target[1] = RGBLED_COLOR_VALUES[color][1];
				target[2] = RGBLED_COLOR_VALUES[color][2];
			}
			RGBLEDSettingsSet(&data);
		}

		if ((current_state == FSM_STATE_RGB_SAVEEXIT) && (current_event == FSM_EVENT_RIGHT)) {
			// Save and exit
			UAVObjSave(RGBLEDSettingsHandle(), 0);
		}
	}
	else {
		write_string("RGB LED not supported", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		current_state = FSM_STATE_RGB_EXIT;
	}
}


void filter_menu(void)
{
	StateEstimationData data;
	int y_pos = MENU_LINE_Y;
	int tmp;
	char tmp_str[100] = {0};
	bool data_changed = false;
	const char * att_filter_strings[4] = {
		[STATEESTIMATION_ATTITUDEFILTER_COMPLEMENTARY] = "Complementary",
		[STATEESTIMATION_ATTITUDEFILTER_COMPLEMENTARYVELCOMPASS] = "Comp+Velcomp",
		[STATEESTIMATION_ATTITUDEFILTER_INSINDOOR] = "INSIndoor",
		[STATEESTIMATION_ATTITUDEFILTER_INSOUTDOOR] = "INSOutdoor",
	};
	const char * nav_filter_strings[3] = {
		[STATEESTIMATION_NAVIGATIONFILTER_NONE] = "None",
		[STATEESTIMATION_NAVIGATIONFILTER_RAW] = "Raw",
		[STATEESTIMATION_NAVIGATIONFILTER_INS] = "INS",
	};

	draw_menu_title("Filter Settings");
	StateEstimationGet(&data);

	for (enum menu_fsm_state s=FSM_STATE_FILTER_ATT; s <= FSM_STATE_FILTER_EXIT; s++) {
		if (s == current_state) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
		}
		switch (s) {
			case FSM_STATE_FILTER_ATT:
				sprintf(tmp_str, "Attitude Filter:   %s", att_filter_strings[data.AttitudeFilter]);
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FILTER_NAV:
				sprintf(tmp_str, "Navigation Filter: %s", nav_filter_strings[data.NavigationFilter]);
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FILTER_SAVEEXIT:
				write_string("Save and Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FILTER_EXIT:
				write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			default:
				break;
		}
		y_pos += MENU_LINE_SPACING;
	}

	switch (current_state) {
		case FSM_STATE_FILTER_ATT:
			if (current_event == FSM_EVENT_RIGHT) {
				data.AttitudeFilter = ((int)(data.AttitudeFilter) + 1) % NELEMENTS(att_filter_strings);;
				data_changed = true;
			}
			if (current_event == FSM_EVENT_LEFT) {
				tmp = (int)(data.AttitudeFilter) - 1;
				if (tmp < 0)
					tmp = NELEMENTS(att_filter_strings) - 1;
				data.AttitudeFilter = tmp;
				data_changed = true;
			}
			break;
		case FSM_STATE_FILTER_NAV:
			if (current_event == FSM_EVENT_RIGHT) {
				data.NavigationFilter = (data.NavigationFilter + 1) % NELEMENTS(nav_filter_strings);
				data_changed = true;
			}
			if (current_event == FSM_EVENT_LEFT) {
				tmp = (int)(data.NavigationFilter) - 1;
				if (tmp < 0)
					tmp =  NELEMENTS(nav_filter_strings) - 1;
				data.NavigationFilter = tmp;
				data_changed = true;
			}
			break;
		default:
			break;
	}

	if (data_changed) {
		StateEstimationSet(&data);
	}

	if ((current_state == FSM_STATE_FILTER_SAVEEXIT) && (current_event == FSM_EVENT_RIGHT)) {
		// Save and exit
		UAVObjSave(StateEstimationHandle(), 0);
	}
}

static const char *nullstrwrap(const char *str)
{
	if (str)
		return str;

	return "";
}

void flightmode_menu(void)
{
	int y_pos = MENU_LINE_Y;
	int tmp;
	char tmp_str[100] = {0};
	bool data_changed = false;
	const char* fmode_strings[] = {
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_MANUAL] = "Manual",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_ACRO] = "Acro",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_ACRODYNE] = "AcroDyne",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_ACROPLUS] = "AcroPlus",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_LEVELING] = "Leveling",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_HORIZON] = "Horizon",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_AXISLOCK] = "Axis Lock",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_VIRTUALBAR] = "Virtualbar",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_STABILIZED1] = "Stabilized1",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_STABILIZED2] = "Stabilized2",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_STABILIZED3] = "Stabilized3",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_AUTOTUNE] = "Autotune",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_ALTITUDEHOLD] = "Altitude Hold",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_POSITIONHOLD] = "Position Hold",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_RETURNTOHOME] = "Return to Home",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_PATHPLANNER] = "Path Planner",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_TABLETCONTROL] = "Tablet Control",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_LQG] = "LQG",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_LQGLEVELING] = "LQG Leveling",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_FLIPREVERSED] = "Flip Reverse",
		[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_FAILSAFE] = "Failsafe",
	};

	uint8_t FlightModePosition[MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_NUMELEM];

	draw_menu_title("Flight Modes");

	ManualControlSettingsFlightModePositionGet(FlightModePosition);
	for (enum menu_fsm_state s=FSM_STATE_FMODE_1; s <= FSM_STATE_FMODE_EXIT; s++) {
		if (s == current_state) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
		}
		switch (s) {
			case FSM_STATE_FMODE_1:
				sprintf(tmp_str, "Position 1: %s", nullstrwrap(fmode_strings[FlightModePosition[0]]));
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FMODE_2:
				sprintf(tmp_str, "Position 2: %s", nullstrwrap(fmode_strings[FlightModePosition[1]]));
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FMODE_3:
				sprintf(tmp_str, "Position 3: %s", nullstrwrap(fmode_strings[FlightModePosition[2]]));
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FMODE_4:
				sprintf(tmp_str, "Position 4: %s", nullstrwrap(fmode_strings[FlightModePosition[3]]));
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FMODE_5:
				sprintf(tmp_str, "Position 5: %s", nullstrwrap(fmode_strings[FlightModePosition[4]]));
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FMODE_6:
				sprintf(tmp_str, "Position 6: %s", nullstrwrap(fmode_strings[FlightModePosition[5]]));
				write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FMODE_SAVEEXIT:
				write_string("Save and Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			case FSM_STATE_FMODE_EXIT:
				write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
				break;
			default:
				break;
		}
		y_pos += MENU_LINE_SPACING;
	}
	
	for (int i=0; i < MANUALCONTROLSETTINGS_FLIGHTMODEPOSITION_NUMELEM; i++) {
		if (current_state == FSM_STATE_FMODE_1 + i) {
			if (current_event == FSM_EVENT_RIGHT) {
				FlightModePosition[i] = ((int)(FlightModePosition[i]) + 1) % NELEMENTS(fmode_strings);
				data_changed = true;
			}
			if (current_event == FSM_EVENT_LEFT) {
				tmp = (int)(FlightModePosition[i]) -1;
				if (tmp < 0)
					tmp = NELEMENTS(fmode_strings) - 1;
				FlightModePosition[i] = tmp;
				data_changed = true;
			}
			
		}
	}

	if (data_changed) {
		ManualControlSettingsFlightModePositionSet(FlightModePosition);
	}

	if ((current_state == FSM_STATE_FMODE_SAVEEXIT) && (current_event == FSM_EVENT_RIGHT)) {
		// Save and exit
		UAVObjSave(ManualControlSettingsHandle(), 0);
	}
}


void homeloc_menu(void)
{
	int y_pos = MENU_LINE_Y;
	char tmp_str[100] = {0};
	HomeLocationData data;
	uint8_t home_set;

	draw_menu_title("Home Location");

	if (HomeLocationHandle()){
		HomeLocationSetGet(&home_set);

		if (home_set == HOMELOCATION_SET_TRUE) {
			HomeLocationGet(&data);
			sprintf(tmp_str, "Home: %0.5f %0.5f Alt: %0.1fm", (double)data.Latitude / 10000000.0, (double)data.Longitude / 10000000.0, (double)data.Altitude);
			write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		}
		else {
			write_string("Home: Not Set", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		}

		y_pos += MENU_LINE_SPACING;
		write_string("Set to current location", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		if (current_state == FSM_STATE_HOMELOC_SET) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
			if (current_event == FSM_EVENT_RIGHT) {
				home_set = HOMELOCATION_SET_FALSE;
				HomeLocationSetSet(&home_set);
			}

		}
	}
	else {
		write_string("Home Location not available", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	}

	y_pos += MENU_LINE_SPACING;
	write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_HOMELOC_EXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
	}
}


void draw_hscale(int x1, int x2, int y, float val_min, float val_max, float val)
{
	int width = x2 - x1;
	int width2;
	write_filled_rectangle_lm(x1, y, width, 6, 0, 1);
	width2 = LIMIT((float)(width - 2) * (val - val_min) / (val_max - val_min), 0, width - 2);
	write_filled_rectangle_lm(x1 + 1, y + 1, width2, 4, 1, 1);
}

const char * axis_strings[] = {"Roll ",
							   "Pitch",
							   "Yaw  "};
const char * pid_strings[] = {"P    ",
							  "I    ",
							  "D    ",
							  "I-Lim"};

void pidrate_menu(void)
{
	const float limits_high[] = {.01f, .05f, .01f, 1.f};
	const float increments[] = {1e-4f, 1e-4f, 1e-4f, 1e-2f};
	
	float pid_arr[STABILIZATIONSETTINGS_ROLLRATEPID_NUMELEM];
	int y_pos = MENU_LINE_Y;
	enum menu_fsm_state my_state = FSM_STATE_PIDRATE_ROLLP;
	bool data_changed = false;
	char tmp_str[100] = {0};

	draw_menu_title("PID Rate");

	for (int i = 0; i < 3; i++) {
		data_changed = false;
		switch (i) {
			case 0:
				StabilizationSettingsRollRatePIDGet(pid_arr);
				break;
			case 1:
				StabilizationSettingsPitchRatePIDGet(pid_arr);
				break;
			case 2:
				StabilizationSettingsYawRatePIDGet(pid_arr);
				break;
		}
		for (int j = 0; j < 4; j++) {
			sprintf(tmp_str, "%s %s: %0.6f", axis_strings[i], pid_strings[j], (double)pid_arr[j]);
			write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
			float h_lim = ceilf(pid_arr[j] / limits_high[j]) * limits_high[j];
			draw_hscale(180, GRAPHICS_RIGHT - 5, y_pos + 2, 0.f, h_lim, pid_arr[j]);
			if (my_state == current_state) {
				draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
				if (current_event == FSM_EVENT_RIGHT) {
					pid_arr[j] = pid_arr[j] + increments[j];
					data_changed = true;
				}
				if (current_event == FSM_EVENT_LEFT) {
					pid_arr[j] = MAX(0.f, pid_arr[j] - increments[j]);
					data_changed = true;
				}
				if (data_changed) {
					switch (i) {
						case 0:
							StabilizationSettingsRollRatePIDSet(pid_arr);
							break;
						case 1:
							StabilizationSettingsPitchRatePIDSet(pid_arr);
							break;
						case 2:
							StabilizationSettingsYawRatePIDSet(pid_arr);
							break;
					}
				}
			}
			y_pos += MENU_LINE_SPACING;
			my_state++;
		}
	}

	write_string("Save and Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_PIDRATE_SAVEEXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
		if (current_event == FSM_EVENT_RIGHT)
			UAVObjSave(StabilizationSettingsHandle(), 0);
	}
	
	y_pos += MENU_LINE_SPACING;
	write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_PIDRATE_EXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
	}
}

const char * pid_strings_att[] = {"P    ",
								  "I    ",
								  "I-Lim"};
void pidatt_menu(void)
{
	const float limits_high[] = {20.f, 20.f, 100.f};
	const float increments[] = {0.1f, 0.1f, 1.f};

	float pid_arr[STABILIZATIONSETTINGS_ROLLPI_NUMELEM];
	int y_pos = MENU_LINE_Y;
	enum menu_fsm_state my_state = FSM_STATE_PIDATT_ROLLP;
	bool data_changed = false;
	char tmp_str[100] = {0};

	draw_menu_title("PID Attitude");

	for (int i = 0; i < 3; i++) {
		data_changed = false;
		switch (i) {
			case 0:
				StabilizationSettingsRollPIGet(pid_arr);
				break;
			case 1:
				StabilizationSettingsPitchPIGet(pid_arr);
				break;
			case 2:
				StabilizationSettingsYawPIGet(pid_arr);
				break;
		}
		for (int j = 0; j < 3; j++) {
			sprintf(tmp_str, "%s %s: %2.1f", axis_strings[i], pid_strings_att[j], (double)pid_arr[j]);
			write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
			float h_lim = ceilf(pid_arr[j] / limits_high[j]) * limits_high[j];
			draw_hscale(170, GRAPHICS_RIGHT - 5, y_pos + 2, 0.f, h_lim, pid_arr[j]);
			if (my_state == current_state) {
				draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
				if (current_event == FSM_EVENT_RIGHT) {
					pid_arr[j] = pid_arr[j] + increments[j];
					data_changed = true;
				}
				if (current_event == FSM_EVENT_LEFT) {
					pid_arr[j] = MAX(0.f, pid_arr[j] - increments[j]);
					data_changed = true;
				}
				if (data_changed) {
					switch (i) {
						case 0:
							StabilizationSettingsRollPISet(pid_arr);
							break;
						case 1:
							StabilizationSettingsPitchPISet(pid_arr);
							break;
						case 2:
							StabilizationSettingsYawPISet(pid_arr);
							break;
					}
				}
			}
			y_pos += MENU_LINE_SPACING;
			my_state++;
		}
	}

	write_string("Save and Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_PIDATT_SAVEEXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
		if (current_event == FSM_EVENT_RIGHT) {
			UAVObjSave(StabilizationSettingsHandle(), 0);
		}
	}

	y_pos += MENU_LINE_SPACING;
	write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_PIDATT_EXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
	}
}

DONT_BUILD_IF(STABILIZATIONSETTINGS_MAXLEVELANGLE_ROLL != 0,
		LevelAngleConstantRoll);
DONT_BUILD_IF(STABILIZATIONSETTINGS_MAXLEVELANGLE_PITCH != 1,
		LevelAngleConstantPitch);
DONT_BUILD_IF(STABILIZATIONSETTINGS_MAXLEVELANGLE_YAW != 2,
		LevelAngleConstantYaw);

#define MAX_STICK_RATE 1440
void sticklimits_menu(void)
{
	int y_pos = MENU_LINE_Y;
	enum menu_fsm_state my_state = FSM_STATE_STICKLIMITS_ROLLA;
	bool data_changed = false;
	char tmp_str[100] = {0};

	draw_menu_title("Stick Limits and Expo");

	uint8_t level_angles[STABILIZATIONSETTINGS_MAXLEVELANGLE_NUMELEM];

	StabilizationSettingsMaxLevelAngleGet(level_angles);

	// Full Stick Angle
	for (int i = 0; i < 3; i++) {
		data_changed = false;

		uint8_t angle = level_angles[i];

		sprintf(tmp_str, "Max Stick Angle %s: %d", axis_strings[i], angle);
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 0, 90, angle);
		if (my_state == current_state) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
			if (current_event == FSM_EVENT_RIGHT) {
				level_angles[i] = MIN(angle + 1, 90);
				data_changed = true;
			}
			if (current_event == FSM_EVENT_LEFT) {
				level_angles[i] = MAX((int)angle - 1, 0);
				data_changed = true;
			}
			if (data_changed) {
				StabilizationSettingsMaxLevelAngleSet(level_angles);
			}
		}
		my_state++;
		y_pos += MENU_LINE_SPACING;
	}

	// Rate Limits
	{
		float rate_arr[STABILIZATIONSETTINGS_MANUALRATE_NUMELEM];
		StabilizationSettingsManualRateGet(rate_arr);
		data_changed = false;
		for (int i = 0; i < 3; i++) {
			sprintf(tmp_str, "Max Stick Rate %s:  %d", axis_strings[i], (int)rate_arr[i]);
			write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
			draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 0, MAX_STICK_RATE, rate_arr[i]);
			if (my_state == current_state) {
				draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
				if (current_event == FSM_EVENT_RIGHT) {
					rate_arr[i] = MIN(rate_arr[i] + 10, MAX_STICK_RATE);
					data_changed = true;
				}
				if (current_event == FSM_EVENT_LEFT) {
					rate_arr[i] = MAX(rate_arr[i] - 10, 0);
					data_changed = true;
				}
			}
			my_state++;
			y_pos += MENU_LINE_SPACING;

		}
		if (data_changed)
			StabilizationSettingsManualRateSet(rate_arr);
	}

	// Rate Expo
	{
		uint8_t expo_arr[STABILIZATIONSETTINGS_RATEEXPO_NUMELEM];
		StabilizationSettingsRateExpoGet(expo_arr);
		data_changed = false;
		for (int i = 0; i < 3; i++) {
			sprintf(tmp_str, "Expo Rate %s:  %d", axis_strings[i], (int)expo_arr[i]);
			write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
			draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 0, 100, expo_arr[i]);
			if (my_state == current_state) {
				draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
				if (current_event == FSM_EVENT_RIGHT) {
					expo_arr[i] = MIN(expo_arr[i] + 1, 100);
					data_changed = true;
				}
				if (current_event == FSM_EVENT_LEFT) {
					expo_arr[i] = MAX(expo_arr[i] - 1, 0);
					data_changed = true;
				}
			}
			my_state++;
			y_pos += MENU_LINE_SPACING;

		}
		if (data_changed)
			StabilizationSettingsRateExpoSet(expo_arr);
	}

	// Horizon Expo
	{
		uint8_t expo_arr[STABILIZATIONSETTINGS_HORIZONEXPO_NUMELEM];
		StabilizationSettingsHorizonExpoGet(expo_arr);
		data_changed = false;
		for (int i = 0; i < 3; i++) {
			sprintf(tmp_str, "Expo Horizon %s:  %d", axis_strings[i], (int)expo_arr[i]);
			write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
			draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 0, 100, expo_arr[i]);
			if (my_state == current_state) {
				draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
				if (current_event == FSM_EVENT_RIGHT) {
					expo_arr[i] = MIN(expo_arr[i] + 1, 100);
					data_changed = true;
				}
				if (current_event == FSM_EVENT_LEFT) {
					expo_arr[i] = MAX(expo_arr[i] - 1, 0);
					data_changed = true;
				}
			}
			my_state++;
			y_pos += MENU_LINE_SPACING;

		}
		if (data_changed)
			StabilizationSettingsHorizonExpoSet(expo_arr);
	}

	// Motor power scaling
	{
			float motor_gain;
			ActuatorSettingsMotorInputOutputGainGet(&motor_gain);
			motor_gain *= 100;

			data_changed = false;
			sprintf(tmp_str, "Motor power scale: %d", (int)motor_gain);
			write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
			draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 50, 100, motor_gain);
			if (my_state == current_state) {
				draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
				if (current_event == FSM_EVENT_RIGHT) {
					motor_gain = MIN(motor_gain + 1, 100);
					data_changed = true;
				}
				if (current_event == FSM_EVENT_LEFT) {
					motor_gain = MAX(motor_gain - 1, 50);
					data_changed = true;
				}
			}
			my_state++;
			y_pos += MENU_LINE_SPACING;

		if (data_changed) {
			motor_gain /= 100;
			ActuatorSettingsMotorInputOutputGainSet(&motor_gain);
		}
	}

	write_string("Save and Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_STICKLIMITS_SAVEEXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
		if (current_event == FSM_EVENT_RIGHT) {
			UAVObjSave(StabilizationSettingsHandle(), 0);
			UAVObjSave(ActuatorSettingsHandle(), 0);
		}
	}

	y_pos += MENU_LINE_SPACING;
	write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_STICKLIMITS_EXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
	}
}

void vtx_menu()
{
	const char *vtx_strings[] = {
		[VTXINFO_MODEL_NONE] = "No VTX detected",
		[VTXINFO_MODEL_TBSUNIFYPRO5G8] = "TBS Unify Pro 5G8",
		[VTXINFO_MODEL_TBSUNIFYPRO5G8HV] = "TBS Unify Pro 5G8 HV",
	};

	const char *band_strings[] = {
		[VTXSETTINGS_BAND_BAND5G8A] = "A",
		[VTXSETTINGS_BAND_BAND5G8B] = "B",
		[VTXSETTINGS_BAND_BAND5G8E] = "E",
		[VTXSETTINGS_BAND_AIRWAVE] = "Airwave",
		[VTXSETTINGS_BAND_RACEBAND] = "Raceband",
	};

	// this is a special case, we need a local copy as the user has to use "apply" to set the new settings
	static VTXSettingsData settings;
	static bool settings_read = false;

	draw_menu_title("Video Transmitter");
	int y_pos = MENU_LINE_Y;

	if ((VTXSettingsHandle() == NULL) || (VTXInfoHandle() == NULL)) {
		write_string("VTX Config Module not running!", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		y_pos += MENU_LINE_SPACING;
		current_state = FSM_STATE_VTX_EXIT;
	}
	else {
		VTXInfoData vtxInfo;
		VTXInfoGet(&vtxInfo);
		char tmp_str[100] = {0};

		if (!settings_read) {
			VTXSettingsGet(&settings);
			settings_read = true;
		}

		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "VTX Type: %s", vtx_strings[vtxInfo.Model]);
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);

		y_pos += 2 * MENU_LINE_SPACING;
		write_string("Current:", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);

		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "   Frequency: %d MHz", vtxInfo.Frequency);
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);

		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "   Power:     %d mW", vtxInfo.Power);
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);

		y_pos += 2 * MENU_LINE_SPACING;
		write_string("New:", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);

		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "    Band:    %s", band_strings[settings.Band]);
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		if (current_state == FSM_STATE_VTX_BAND) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);

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
		y_pos += MENU_LINE_SPACING;

		sprintf(tmp_str, "    Channel: CH %d %d MHz", *target + 1, band_ptr[*target]);
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);

		if (current_state == FSM_STATE_VTX_CH) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);

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

		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "    Power:   %d mW", VTX_POWER[settings.Power]);
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);

		if (current_state == FSM_STATE_VTX_POWER) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);

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
	}

	y_pos += 2 * MENU_LINE_SPACING;
	write_string("Apply", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_VTX_APPLY) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
		if (current_event == FSM_EVENT_RIGHT) {
			VTXSettingsSet(&settings);
		}
	}

	y_pos += MENU_LINE_SPACING;
	write_string("Save and Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_VTX_SAVEEXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
		if (current_event == FSM_EVENT_RIGHT) {
			VTXSettingsSet(&settings);
			UAVObjSave(VTXSettingsHandle(), 0);
			settings_read = false;
		}
	}

	y_pos += MENU_LINE_SPACING;
	write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_VTX_EXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);

		if (current_event == FSM_EVENT_RIGHT)
			settings_read = false;
		
	}

}


void stats_menu()
{
	int y_pos = render_stats();
	write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
	if (current_state == FSM_STATE_STATS_EXIT) {
		draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
	}
}

void battery_menu(void)
{
	int y_pos = MENU_LINE_Y;
	bool settings_changed = false;
	char tmp_str[100] = {0};
	static FlightBatterySettingsData batterySettings;
	static bool settings_read = false;


	draw_menu_title("Battery Settings");

	if (FlightBatterySettingsHandle()) {
		if (!settings_read) {
			FlightBatterySettingsGet(&batterySettings);
			settings_read = true;
		}

		sprintf(tmp_str, "Capacity (mAh): %d", (int)batterySettings.Capacity );
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 0, 15000, batterySettings.Capacity);
		if (current_state == FSM_STATE_BATTERY_CAPACITY) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
			if (current_event == FSM_EVENT_RIGHT) {
				batterySettings.Capacity = MIN(15000, batterySettings.Capacity + 50);
				settings_changed = true;
			}
			else if (current_event == FSM_EVENT_LEFT) {
				batterySettings.Capacity = MAX(500, batterySettings.Capacity - 50);
				settings_changed = true;
			}
		}

		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "Cell Count : %d", (int)batterySettings.NbCells );
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 1, 10, batterySettings.NbCells);
		if (current_state == FSM_STATE_BATTERY_CELLCOUNT) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
			if (current_event == FSM_EVENT_RIGHT) {
				batterySettings.NbCells = MIN(10, batterySettings.NbCells + 1);
				settings_changed = true;
			}
			else if (current_event == FSM_EVENT_LEFT) {
				batterySettings.NbCells = MAX(1, batterySettings.NbCells - 1);
				settings_changed = true;
			}
		}
	
		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "Low Cell Warn V: %1.1f", (double) batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] );
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 0.0f, 4.5f, batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING]);
		if (current_state == FSM_STATE_BATTERY_WARNING) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
			if (current_event == FSM_EVENT_RIGHT) {
				batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] = MIN(4.5f, batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] + 0.1f);
				settings_changed = true;
			}
			else if (current_event == FSM_EVENT_LEFT) {
				batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] = MAX(3.0f, batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] - 0.1f);
				settings_changed = true;
			}
		}
	
	
		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "Low Cell Alarm V: %1.1f", (double) batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] );
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 0.0f, 4.5f, batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM]);
		if (current_state == FSM_STATE_BATTERY_ALARM) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
			if (current_event == FSM_EVENT_RIGHT) {
				batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] = MIN(4.5f, batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] + 0.1f);
				settings_changed = true;
			}
			else if (current_event == FSM_EVENT_LEFT) {
				batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] = MAX(3.0f, batterySettings.CellVoltageThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] - 0.1f);
				settings_changed = true;
			}
		}
	
	
		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "Flight Warn Time : %d", batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] );
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 0, 300, batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING]);
		if (current_state == FSM_STATE_BATTERY_WARN_TIME) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
			if (current_event == FSM_EVENT_RIGHT) {
				batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] = MIN(4.5f, batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] + 1);
				settings_changed = true;
			}
			else if (current_event == FSM_EVENT_LEFT) {
				batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] = MIN(4.5f, batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_WARNING] - 1);
				settings_changed = true;
			}
		}
	
	
		y_pos += MENU_LINE_SPACING;
		sprintf(tmp_str, "Flight Alarm Time : %d", batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] );
		write_string(tmp_str, MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		draw_hscale(225, GRAPHICS_RIGHT - 5, y_pos + 2, 0, 300, batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM]);
		if (current_state == FSM_STATE_BATTERY_ALARM_TIME) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);
			if (current_event == FSM_EVENT_RIGHT) {
				batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] = MIN(4.5f, batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] + 1);
				settings_changed = true;
			}
			else if (current_event == FSM_EVENT_LEFT) {
				batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] = MIN(4.5f, batterySettings.FlightTimeThresholds[FLIGHTBATTERYSETTINGS_CELLVOLTAGETHRESHOLDS_ALARM] - 1);
				settings_changed = true;
			}
		}

		if (settings_changed)
			FlightBatterySettingsSet(&batterySettings);

	
		y_pos += MENU_LINE_SPACING;
		write_string("Save and Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		if (current_state == FSM_STATE_BATTERY_SAVEEXIT) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);

			if (current_event == FSM_EVENT_RIGHT ) {
				UAVObjSave(FlightBatterySettingsHandle(), 0);
				settings_read = false;
			}
		}
		
		y_pos += MENU_LINE_SPACING;
		write_string("Exit", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		if (current_state == FSM_STATE_BATTERY_EXIT) {
			draw_selected_icon(MENU_LINE_X - 4, y_pos + 4);

			if (current_event == FSM_EVENT_RIGHT)
				settings_read = false;
		}
	}
	else {
		write_string("Battery module not running", MENU_LINE_X, y_pos, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, MENU_FONT);
		current_state = FSM_STATE_BATTERY_EXIT;
	}
}

#endif /* OSD_USE_MENU */

/**
 * @}
 * @}
 */
