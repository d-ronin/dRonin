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

#include "loadable_extension.h"

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

static void invoke_one_loadable(struct loadable_extension *ext)
{
	/* XXX check payload CRC. */
}

static void invoke_loadables()
{
	uintptr_t part_id;

	if (PIOS_FLASH_find_partition_id(FLASH_PARTITION_LABEL_LOADABLE_EXTENSION,
				&part_id)) {
		/* No loadable partition */
		return;
	}

	uint32_t part_len;
	uint8_t *part_beginning;

	part_beginning = PIOS_FLASH_get_address(part_id, &part_len);

	if (!part_beginning) {
		return;
	}

	uint32_t part_pos = 0;

	while ((part_pos + sizeof(struct loadable_extension)) < part_len) {
		struct loadable_extension *ext =
			(void *) (part_beginning + part_pos);

		/* Validate header before trying to go further */

		if (ext->magic != LOADABLE_EXTENSION_MAGIC) {
			return;
		}

		if (ext->length < sizeof(struct loadable_extension)) {
			return;
		}

		if (ext->require_version == LOADABLE_REQUIRE_VERSION_INVALID) {
			return;
		}

		if (ext->entry_offset >= ext->length) {
			return;
		}

		if (ext->entry_offset < sizeof(struct loadable_extension)) {
			return;
		}

		/* XXX alignment requirements */

		/* XXX check CRC */

		invoke_one_loadable(ext);

		part_pos += ext->length;
	}

}

static int32_t loadable_start(void)
{
	if (!module_enabled) {
		return 0;
	}

	/* XXX 	for now do stuff in a task, in an infinite loop */
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
	while (1) {
		PIOS_Thread_Sleep(1000);
		invoke_loadables();
	}
}

/** 
  * @}
  */
