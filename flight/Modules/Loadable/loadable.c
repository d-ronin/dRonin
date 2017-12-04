/**
 ******************************************************************************
 * @file       loadable.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup Modules Modules
 * @{
 * @brief Implement simple interface for loadable modules.
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
#include <stdbool.h>
#include "pios_thread.h"

#include "modulesettings.h"

static bool module_enabled;

static int32_t loadable_initialize(void)
{
#ifdef MODULE_Loadable_BUILTIN
	module_enabled = true;
#else
	uint8_t module_state[MODULESETTINGS_ADMINSTATE_NUMELEM];
	ModuleSettingsAdminStateGet(module_state);
	if (module_state[MODULESETTINGS_ADMINSTATE_LOADABLE] == MODULESETTINGS_ADMINSTATE_ENABLED) {
		module_enabled = true;
	} else {
		module_enabled = false;
	}
#endif

	return 0;
}

static void loadable_task(void *parameters);

static int32_t loadable_start(void)
{
	if (!module_enabled) {
		return 0;
	}

	/* XXX: check for loadable task, requested stack size, etc */

	struct pios_thread *loadable_task_handle;

	loadable_task_handle = PIOS_Thread_Create(loadable_task, "Fault", PIOS_THREAD_STACK_SIZE_MIN, NULL, PIOS_THREAD_PRIO_HIGHEST);

	if (!loadable_task_handle) {
		return -1;
	}

        TaskMonitorAdd(TASKINFO_RUNNING_LOADABLE, loadable_task_handle);

	return 0;
}
MODULE_INITCALL(loadable_initialize, loadable_start)

static void loadable_task(void *parameters)
{
	/* XXX: invoke loadable task */
	while (1) {
		PIOS_Thread_Sleep(1000);
	}
}

/** 
  * @}
  */
