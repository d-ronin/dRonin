/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup Sim POSIX Simulator
 * @{
 *
 * @file       simulation/fw/pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      Simulation of the board specific initialization routines
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
 */

#include <pios.h>
#include "pios_board.h"
#include <pios_com_priv.h>
#include <pios_tcp_priv.h>
#include <openpilot.h>
#include <uavobjectsinit.h>
#include <pios_thread.h>

#include "accels.h"
#include "baroaltitude.h"
#include "gpsposition.h"
#include "gyros.h"
#include "gyrosbias.h"
#include "hwsparky.h"
#include "hwsimulation.h"
#include "magnetometer.h"
#include "manualcontrolsettings.h"

#include "pios_hal.h"
#include "pios_rcvr_priv.h"
#include "pios_queue.h"

void Stack_Change() {
}

const struct pios_tcp_cfg pios_tcp_telem_cfg = {
  .ip = "0.0.0.0",
  .port = 9000,
};

#define PIOS_COM_TELEM_TCP_RX_BUF_LEN 384
#define PIOS_COM_TELEM_TCP_TX_BUF_LEN 384
#define PIOS_COM_GPS_RX_BUF_LEN 96

/**
 * Simulation of the flash filesystem
 */
#include "../../../tests/logfs/unittest_init.c"

uintptr_t pios_uavo_settings_fs_id;

/*
 * Board specific number of devices.
 */

uintptr_t pios_com_debug_id;
uintptr_t pios_com_openlog_id;
uintptr_t pios_com_telem_usb_id;

/**
 * PIOS_Board_Init()
 * initializes all the core systems on this specific hardware
 * called from System/openpilot.c
 */
void PIOS_Board_Init(void) {

	/* Delay system */
	PIOS_DELAY_Init();

	int32_t retval = PIOS_Flash_Posix_Init(&pios_posix_flash_id,
			&flash_config, false);

	if (retval != 0) {
		fprintf(stderr, "Unable to initialize posix flash driver: %d\n",
				retval);
		exit(1);
	}

	/* Register the partition table */
	PIOS_FLASH_register_partition_table(pios_flash_partition_table, NELEMENTS(pios_flash_partition_table));

	if (PIOS_FLASHFS_Logfs_Init(&pios_uavo_settings_fs_id, &flashfs_config_settings, FLASH_PARTITION_LABEL_SETTINGS) != 0)
		fprintf(stderr, "Unable to open the settings partition\n");

	/* Initialize UAVObject libraries */
	UAVObjectsInitializeAll();

	/* Initialize the alarms library */
	AlarmsInitialize();

	HwSimulationInitialize();

	if (!PIOS_COM_TELEM_USB) {
		uintptr_t pios_tcp_telem_id;

		if (PIOS_TCP_Init(&pios_tcp_telem_id, &pios_tcp_telem_cfg)) {
			PIOS_Assert(0);
		}

		if (PIOS_COM_Init(&PIOS_COM_TELEM_USB,
					&pios_tcp_com_driver, pios_tcp_telem_id,
					PIOS_COM_TELEM_TCP_RX_BUF_LEN,
					PIOS_COM_TELEM_TCP_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}
	}

	printf("Completed PIOS_Board_Init\n");
}

/**
 * @}
 * @}
 */
