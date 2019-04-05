/**
 ******************************************************************************
 * @file       pios_modules.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_Modules Module Functions
 * @{
 * @brief Allows control of module enable/disable from PiOS
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

#ifndef PIOS_MODULES_H_
#define PIOS_MODULES_H_

enum pios_modules {
	PIOS_MODULE_COMUSBBRIDGE,
	PIOS_MODULE_GPS,
	PIOS_MODULE_UAVOFRSKYSENSORHUBBRIDGE,
	PIOS_MODULE_UAVOFRSKYSPORTBRIDGE,
	PIOS_MODULE_UAVOHOTTBRIDGE,
	PIOS_MODULE_UAVOLIGHTTELEMETRYBRIDGE,
	PIOS_MODULE_UAVOMAVLINKBRIDGE,
	PIOS_MODULE_UAVOMSPBRIDGE,
	PIOS_MODULE_UAVOCROSSFIRETELEMETRY,
	PIOS_MODULE_VTXCONFIG,
	PIOS_MODULE_AUTOTUNE,
	PIOS_MODULE_NUM
};

#ifndef PIOS_NO_MODULES
void PIOS_Modules_Enable(enum pios_modules module);
bool PIOS_Modules_IsEnabled(enum pios_modules module);
#else
#define PIOS_Modules_Enable(x) do { (void) (x); } while (0)
#endif

/* Couldn't really find a better place to put this declaration... */
enum actuator_interlock {
	ACTUATOR_INTERLOCK_OK,
	ACTUATOR_INTERLOCK_STOPREQUEST,
	ACTUATOR_INTERLOCK_STOPPED,
};

extern volatile enum actuator_interlock actuator_interlock;

/* Ideally would have a better home for this constant */
#define DSHOT_CMD_BEACON3 (3)

extern int actuator_send_dshot_command(uint8_t cmd_id, uint8_t num_to_send,
                uint16_t channel_mask);
extern void system_annunc_custom_string(const char *string);
extern void telemetry_set_inhibit(bool inhibit);

#endif // PIOS_MODULES_H_

/**
 * @ }
 * @ }
 */
