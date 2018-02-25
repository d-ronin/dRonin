/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{ 
 * @addtogroup VTXConfig Module
 * @{ 
 *
 * @file       VTXConfig.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
 * @brief      This module configures the video transmitter
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

#include "openpilot.h"
#include <pios_hal.h>
#include <pios_thread.h>

#include "tbs_smartaudio.h"

#include "vtxsettings.h"
#include "vtxinfo.h"

// ****************
// Private functions

static void vtxConfigTask(void *parameters);
//static void updateSettings();

// ****************
// Private types
enum VTXTYPE {
	VTX_NONE,
	VTX_TBS_SMARTAUDIO
};

enum STATE {
	DISCONNECTED,
	CONNECTED
};

// ****************
// Private constants
#define STACK_SIZE_BYTES   560
#define TASK_PRIORITY      PIOS_THREAD_PRIO_LOW

#define MAX_FAILS 5
#define TBS_MIN_BAUD 4600
#define TBS_MAX_BAUD 5100



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

// ****************
// Private variables

static uint32_t vtxConfigPort;
static bool module_enabled = false;
static volatile bool settings_updated = false;
static struct pios_thread *vtxConfigTaskHandle;

static enum VTXTYPE vtx_type = VTX_NONE;

#if defined(PIOS_INCLUDE_TBSVTXCONFIG)
extern uintptr_t pios_com_tbsvtxconfig_id;
#endif


// ****************
/**
 * Initialise the VTXConfig module
 * \return -1 if initialisation failed
 * \return 0 on success
 */

int32_t VTXConfigStart(void)
{
	if (module_enabled) {
		if (vtxConfigPort) {
			// Start task
			vtxConfigTaskHandle = PIOS_Thread_Create(vtxConfigTask, "VTXConfig", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
			TaskMonitorAdd(TASKINFO_RUNNING_VTXCONFIG, vtxConfigTaskHandle);
			return 0;
		}
	}
	return -1;
}

/**
 * Initialise the VTXConfig module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t VTXConfigInitialize(void)
{
#if defined(PIOS_INCLUDE_TBSVTXCONFIG)
	if (pios_com_tbsvtxconfig_id) {
		vtx_type = VTX_TBS_SMARTAUDIO;
		vtxConfigPort = pios_com_tbsvtxconfig_id;
	}
#endif /* PIOS_INCLUDE_TBSVTXCONFIG */

	if (vtx_type != VTX_NONE) {
		module_enabled = PIOS_Modules_IsEnabled(PIOS_MODULE_VTXCONFIG);
	}
	else {
		module_enabled = false;
	}

	VTXSettingsInitialize();

	if (module_enabled) {
		VTXInfoInitialize();
		VTXSettingsConnectCallbackCtx(UAVObjCbSetFlag, &settings_updated);
	}

	return 0;
}

MODULE_INITCALL(VTXConfigInitialize, VTXConfigStart);


/**
 * Main VTXConfig Task
 */

static void vtxConfigTask(void *parameters)
{
	VTXInfoData info;
	VTXSettingsData settings;

	uint16_t baud_rate = TBS_MIN_BAUD;
	uint8_t n_fails = 0;
	enum STATE state = DISCONNECTED;
	enum STATE state_prev = DISCONNECTED;
	bool update_vtx = false;

	VTXSettingsGet(&settings);

	// Wait for power to stabilize before talking to external devices
	PIOS_Thread_Sleep(1000);

	// Loop forever
	while (1) {
		// Try to connect faster when disconnected
		if (state == CONNECTED) {
			PIOS_Thread_Sleep(1000);
		}
		else {
			PIOS_Thread_Sleep(500);
		}

		if (settings_updated) {
			VTXSettingsGet(&settings);
			settings_updated = false;
			update_vtx = true;
		}

		if (tbsvtx_get_state(vtxConfigPort, &info) >= 0) {
			state = CONNECTED;
			n_fails = 0;
			info.State = VTXINFO_STATE_CONNECTED;
			VTXInfoSet(&info);
			if (state_prev == DISCONNECTED) {
				update_vtx = true;
			}
		}
		else {
			n_fails += 1;
		}

		if (n_fails > MAX_FAILS) {
			state = DISCONNECTED;
			memset((void*)&info, 0, sizeof(info));
			info.State = VTXINFO_STATE_DISCONNECTED;
			VTXInfoSet(&info);
		}

		if (state == CONNECTED) {
			PIOS_Thread_Sleep(500);
			//update_vtx = true;
			if ((settings.Mode == VTXSETTINGS_MODE_ACTIVE) && update_vtx) {

				// Get the frequency
				uint16_t frequency;
				switch (settings.Band) {
					case VTXSETTINGS_BAND_BAND5G8A:
						frequency = BAND_5G8_A_FREQS[settings.Band_5G8_A_Frequency];
						break;
					case VTXSETTINGS_BAND_BAND5G8B:
						frequency = BAND_5G8_B_FREQS[settings.Band_5G8_B_Frequency];
						break;
					case VTXSETTINGS_BAND_BAND5G8E:
						frequency = BAND_5G8_E_FREQS[settings.Band_5G8_E_Frequency];
						break;
					case VTXSETTINGS_BAND_AIRWAVE:
						frequency = AIRWAVE_FREQS[settings.Airwave_Frequency];
						break;
					case VTXSETTINGS_BAND_RACEBAND:
						frequency = RACEBAND_FREQS[settings.Raceband_Frequency];
						break;
					default:
						update_vtx = false;
				}
				if (update_vtx) {
					update_vtx = false;
					if (frequency != info.Frequency) {
						if (tbsvtx_set_freq(vtxConfigPort, frequency) < 0) {
							// something went wrong, we need to re-try again
							update_vtx = true;
						}
						PIOS_Thread_Sleep(500);
					}

					// Don't increase the power if the VTX reports 0mW. This is a special mode
					// e.g. "pit mode" for TBS and the user requested to transmit at low power.
					if ((info.Power != 0) && (VTX_POWER[settings.Power] != info.Power)) {
						if (tbsvtx_set_power(vtxConfigPort, VTX_POWER[settings.Power]) < 0) {
							// somethign went wrong, we need to re-try again
							update_vtx = true;
						}
					}
				}
			}
		} else {
			// Cycle through different baud rates. The spec says 4.8kbps, but it can deviate
			if (baud_rate < TBS_MAX_BAUD) {
				baud_rate += 50;
			}
			else {
				baud_rate = TBS_MIN_BAUD;
			}
			PIOS_COM_ChangeBaud(vtxConfigPort, baud_rate);
		}
		state_prev = state;
	}
}
