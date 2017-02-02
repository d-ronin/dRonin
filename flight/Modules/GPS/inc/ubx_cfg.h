/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup GSPModule GPS Module
 * @brief Process GPS information
 * @{
 *
 * @file       ubx_cfg.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @brief      Include file for UBX configuration
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

#include "stdint.h"
#include "modulesettings.h"

void ubx_cfg_send_configuration(uintptr_t gps_port, char *buffer,
		ModuleSettingsGPSConstellationOptions constellation,
		ModuleSettingsGPSSBASConstellationOptions sbas_const,
		ModuleSettingsGPSDynamicsModeOptions dyn_mode);

void ubx_cfg_set_baudrate(uintptr_t gps_port, ModuleSettingsGPSSpeedOptions baud_rate);

/**
 * @}
 * @}
 */
