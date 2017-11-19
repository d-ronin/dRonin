/**
 ******************************************************************************
 * @addtogroup Modules Modules
 * @{
 * @addtogroup CharOSD Character OSD
 * @{
 *
 * @brief Process OSD information
 * @file       characterosd.c
 * @author     dRonin, http://dronin.org Copyright (C) 2016
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

#include <openpilot.h>
#include <pios_board_info.h>
#include "pios_thread.h"
#include "misc_math.h"
#include "pios_modules.h"
#include "pios_sensors.h"

#include "modulesettings.h"

#include "physical_constants.h"

#include "accels.h"
#include "attitudeactual.h"
#include "baroaltitude.h"
#include "flightstatus.h"
#include "flightbatterystate.h"
#include "flightbatterysettings.h"
#include "flightstats.h"
#include "gpsposition.h"
#include "positionactual.h"
#include "gpstime.h"
#include "gpssatellites.h"
#include "gpsvelocity.h"
#include "homelocation.h"
#include "magnetometer.h"
#include "manualcontrolcommand.h"
#include "modulesettings.h"
#include "stabilizationdesired.h"
#include "stabilizationsettings.h"
#include "stateestimation.h"
#include "systemalarms.h"
#include "systemstats.h"
#include "tabletinfo.h"
#include "taskinfo.h"
#include "velocityactual.h"
#include "vtxinfo.h"
#include "waypoint.h"
#include "waypointactive.h"

#include "charosd.h"

#define STACK_SIZE_BYTES 3072
#define TASK_PRIORITY PIOS_THREAD_PRIO_LOW

#define SPLASH_TIME_MS (5*1000)

bool module_enabled;

static void panel_draw(charosd_state_t state, uint8_t panel, uint8_t x, uint8_t y)
{
	if (panel <= CHARONSCREENDISPLAYSETTINGS_PANELTYPE_MAXOPTVAL) {
		if (panels[panel].update &&
		    HAS_SENSOR(state->available, panels[panel].requirements)) {
			panels[panel].update(state, x, y);
		}
	}
}

static void screen_draw(charosd_state_t state, CharOnScreenDisplaySettingsData *page)
{
	PIOS_MAX7456_clear (state->dev);

	for (uint8_t i = 0; i < CHARONSCREENDISPLAYSETTINGS_PANELTYPE_NUMELEM;
		       i++) {
		panel_draw(state, page->PanelType[i], page->X[i], page->Y[i]);
	}
}

static const uint8_t charosd_font_data[] = {
#include "charosd-font.h"
};

#ifndef CHAROSD_FONT_MINIMAL
static const uint8_t charosd_font_small_data[] = {
#include "charosd-font-small.h"
};

static const uint8_t charosd_font_thin_data[] = {
#include "charosd-font-thin.h"
};
#endif

static void set_mode(charosd_state_t state, uint8_t video_std)
{
	state->video_standard = video_std;

	switch (video_std) {
		case CHARONSCREENDISPLAYSETTINGS_VIDEOSTANDARD_AUTODETECTPREFERPAL:
			PIOS_MAX7456_set_mode(state->dev, false,
					MAX7456_MODE_PAL);
			break;
		case CHARONSCREENDISPLAYSETTINGS_VIDEOSTANDARD_AUTODETECTPREFERNTSC:
			PIOS_MAX7456_set_mode(state->dev, false,
					MAX7456_MODE_NTSC);
			break;
		case CHARONSCREENDISPLAYSETTINGS_VIDEOSTANDARD_FORCEPAL:
			PIOS_MAX7456_set_mode(state->dev, true,
					MAX7456_MODE_PAL);
			break;
		case CHARONSCREENDISPLAYSETTINGS_VIDEOSTANDARD_FORCENTSC:
			PIOS_MAX7456_set_mode(state->dev, true,
					MAX7456_MODE_NTSC);
			break;
		default:
			PIOS_Assert(0);
			break;
	}
}

/* 12 x 18 x 2bpp */
#define FONT_CHAR_SIZE ((12 * 18 * 2) / 8)

static void program_characters(charosd_state_t state, uint8_t font)
{
	const char *loaded_txt = "";

	const uint8_t *font_data = charosd_font_data;
	bool changed = false;

	switch (font) {
	case CHARONSCREENDISPLAYSETTINGS_FONT_THIN:
#ifndef CHAROSD_FONT_MINIMAL
		font_data = charosd_font_thin_data;
		loaded_txt = "loaded thin font";
		break;
#endif
	case CHARONSCREENDISPLAYSETTINGS_FONT_SMALL:
#ifndef CHAROSD_FONT_MINIMAL
		font_data = charosd_font_small_data;
		loaded_txt = "loaded small font";
		break;
#endif
	case CHARONSCREENDISPLAYSETTINGS_FONT_REGULAR:
		font_data = charosd_font_data;
		loaded_txt = "loaded regular font";
		break;
	}

	/* Check on font data, replacing chars as appropriate. */
	for (int i = 0; i < 256; i++) {
		const uint8_t *this_char = &font_data[i * FONT_CHAR_SIZE];
		/* Every 8 characters, take a break, let other lowprio
		 * tasks run */
		if ((i & 0x7) == 0) {
			PIOS_Thread_Sleep(2);
		}

		uint8_t nvm_char[FONT_CHAR_SIZE];

		PIOS_MAX7456_download_char(state->dev, i, nvm_char);

		/* Only do this when necessary, because it's slow and wears
		 * NVRAM. */
		if (memcmp(nvm_char, this_char, FONT_CHAR_SIZE)) {
			PIOS_MAX7456_upload_char(state->dev, i,
					this_char);
			changed = true;
		}
	}
	if (changed) {
		PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER,
				  6, loaded_txt, 0);
		PIOS_Thread_Sleep(1000);
	}
	state->prev_font = font;
}

static void update_availability(charosd_state_t state)
{
	state->available = 0;

	ModuleSettingsData module_settings;
	ModuleSettingsGet(&module_settings);

	// TODO(dsal): Figure out how to determine whether this is
	// configured meaningfully
	state->available |= HAS_RSSI;

	if (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_BARO)) {
		state->available |= HAS_ALT;
	}
	if (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_MAG)) {
		state->available |= HAS_COMPASS;
	}
	if (PIOS_Modules_IsEnabled(PIOS_MODULE_GPS)) {
		state->available |= HAS_GPS;
	}
	if (module_settings.AdminState[MODULESETTINGS_ADMINSTATE_BATTERY] &&
	    FlightBatteryStateHandle()) {
		state->available |= HAS_BATT;
	}
	if (module_settings.AdminState[MODULESETTINGS_ADMINSTATE_AIRSPEED]) {
		state->available |= HAS_PITOT;
	}

	uint8_t filter;
	StateEstimationNavigationFilterGet(&filter);
	if (filter != STATEESTIMATION_NAVIGATIONFILTER_NONE) {
		state->available |= HAS_NAV;
	}

}

static void update_telemetry(charosd_state_t state)
{
	SystemStatsFlightTimeGet(&state->telemetry.system.flight_time);
	AttitudeActualGet(&state->telemetry.attitude_actual);
	PositionActualGet(&state->telemetry.position_actual);
	StabilizationDesiredThrustGet(&state->telemetry.manual.thrust);
	FlightStatusArmedGet(&state->telemetry.flight_status.arm_status);
	FlightStatusFlightModeGet(&state->telemetry.flight_status.mode);

	if (HAS_SENSOR(state->available, HAS_BATT)) {
		FlightBatteryStateGet(&state->telemetry.battery);
	}
	if (HAS_SENSOR(state->available, HAS_GPS)) {
		GPSPositionGet(&state->telemetry.gps_position);
	}
	if (HAS_SENSOR(state->available, HAS_RSSI)) {
		ManualControlCommandRssiGet(&state->telemetry.manual.rssi);
	}
	if (HAS_SENSOR(state->available, HAS_ALT)
	    || HAS_SENSOR(state->available, HAS_GPS)) {
		VelocityActualGet(&state->telemetry.velocity_actual);
	}
}

static void splash_screen(charosd_state_t state)
{
	PIOS_MAX7456_clear (state->dev);

	const char *welcome_msg = "Welcome to dRonin";

	SystemAlarmsData alarm;
	SystemAlarmsGet(&alarm);
	const char *boot_reason = AlarmBootReason(alarm.RebootCause);
	PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 4, welcome_msg, 0);
	PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 6, boot_reason, 0);

	PIOS_Thread_Sleep(SPLASH_TIME_MS);
}

/**
 * Main osd task. It does not return.
 */
static void CharOnScreenDisplayTask(void *parameters)
{
	(void) parameters;

	charosd_state_t state;

	state = PIOS_malloc(sizeof(*state));
	state->dev = pios_max7456_id;
	state->prev_font = 0xff;

	state->video_standard = 0xff;

	bzero(&state->telemetry, sizeof(state->telemetry));

	CharOnScreenDisplaySettingsData page;
	CharOnScreenDisplaySettingsGet(&page);

	program_characters(state, page.Font);

	splash_screen(state);

	while (1) {
		update_availability(state);
		update_telemetry(state);

		CharOnScreenDisplaySettingsGet(&page);

		if (state->prev_font != page.Font) {
			program_characters(state, page.Font);
		}

		if (state->video_standard != page.VideoStandard) {
			set_mode(state, page.VideoStandard);
		}

		state->custom_text = (char*)page.CustomText;

		screen_draw(state, &page);

		if (PIOS_MAX7456_stall_detect(state->dev)) {
			PIOS_MAX7456_puts(state->dev, MAX7456_FMT_H_CENTER, 6, "... STALLED ...", 0);
			PIOS_Thread_Sleep(10000);
		}

		PIOS_MAX7456_wait_vsync(state->dev);
	}
}

/**
 * Initialize the osd module
 */
int32_t CharOnScreenDisplayInitialize(void)
{
	CharOnScreenDisplaySettingsInitialize();

	if (pios_max7456_id) {
		module_enabled = true;
	}

	return 0;
}

int32_t CharOnScreenDisplayStart(void)
{
	if (module_enabled) {
		struct pios_thread *taskHandle;

		taskHandle = PIOS_Thread_Create(CharOnScreenDisplayTask, "OnScreenDisplay", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
		TaskMonitorAdd(TASKINFO_RUNNING_ONSCREENDISPLAY, taskHandle);

		return 0;
	}
	return -1;
}

MODULE_INITCALL(CharOnScreenDisplayInitialize, CharOnScreenDisplayStart);

/**
  * @}
  * @}
  */
