/**
 ******************************************************************************
 * @addtogroup Tau Labs Modules
 * @{
 * @addtogroup CharacterOSD OSD Module
 * @brief Process OSD information
 * @{
 *
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include "pios_video.h"

#include "physical_constants.h"

#include "accels.h"
#include "accessorydesired.h"
#include "airspeedactual.h"
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

bool module_enabled;

static void panel_draw (charosd_state_t state, uint8_t panel, uint8_t x, uint8_t y)
{
	if (panel <= CHARONSCREENDISPLAYSETTINGS_PANELTYPE_MAXOPTVAL) {
		if (panels[panel].update) {
			panels [panel].update(state, x, y);
		}
	}
}

static void screen_draw (charosd_state_t state, CharOnScreenDisplaySettingsData *page)
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

/**
 * Main osd task. It does not return.
 */
static void CharOnScreenDisplayTask(void *parameters)
{
	(void) parameters;

	charosd_state_t state;

	state = PIOS_malloc(sizeof(*state));
	state->dev = pios_max7456_id;

	while (1) {
		CharOnScreenDisplaySettingsData page;
		CharOnScreenDisplaySettingsGet(&page);

		screen_draw(state, &page);
		PIOS_Thread_Sleep(10);

		while (PIOS_MAX7456_poll_vsync_spi(state->dev)) {
			PIOS_Thread_Sleep(1);
		}
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

