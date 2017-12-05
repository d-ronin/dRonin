/**
 ******************************************************************************
 * @file       sensors.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup Modules Modules
 * @{
 * @addtogroup StabilizationModule Stabilization Module
 * @{
 * @brief Header for sensor functions
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

#ifndef _SENSORS_H
#define _SENSORS_H

int32_t sensors_init(void);
bool sensors_step();

#endif

/**
 * @}
 * @}
 */
