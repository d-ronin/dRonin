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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
	PIOS_MODULE_STORM32BGC,
	PIOS_MODULE_UAVOFRSKYSENSORHUBBRIDGE,
	PIOS_MODULE_UAVOFRSKYSPORTBRIDGE,
	PIOS_MODULE_UAVOHOTTBRIDGE,
	PIOS_MODULE_UAVOLIGHTTELEMETRYBRIDGE,
	PIOS_MODULE_UAVOMAVLINKBRIDGE,
	PIOS_MODULE_UAVOMSPBRIDGE,
	PIOS_MODULE_NUM
};

void PIOS_Modules_Enable(enum pios_modules module);
bool PIOS_Modules_IsEnabled(enum pios_modules module);

#endif // PIOS_MODULES_H_

/**
 * @ }
 * @ }
 */