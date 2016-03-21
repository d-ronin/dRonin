/**
 ******************************************************************************
 * @file       pios_modules.c
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

#include "pios.h"

static bool modules_enabled[PIOS_MODULE_NUM] = {false, };

void PIOS_Modules_Enable(enum pios_modules module)
{
	if (module >= PIOS_MODULE_NUM)
		return;
	modules_enabled[module] = true;
}

bool PIOS_Modules_IsEnabled(enum pios_modules module)
{
	if (module >= PIOS_MODULE_NUM)
		return false;
	return modules_enabled[module];
}

/**
 * @ }
 * @ }
 */