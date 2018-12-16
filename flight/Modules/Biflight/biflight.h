/**
 ******************************************************************************
 * @addtogroup BiflightModule Biflight Module
 * @{
 *
 * @file       biflight.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @brief      Bifight module. Specialty mixing for bicopters.
 *
 * Based on algorithms and code developed here:
 *     http://rcexplorer.se/forums/topic/debugging-the-tricopter-mini-racer/
 *
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

#include "pios_config.h"

#include "actuatorsettings.h"
#include "flightstatus.h"
#include "biflightsettings.h"
#include "biflightstatus.h"

#define LEFT 0
#define RIGHT 1

void biflightInit(void);

void biServoStep(BiflightSettingsData *biflightSettings,
                 BiflightStatusData   *biflightStatus,
                 float dT);

void biServoCalibrateStep(ActuatorSettingsData *actuatorSettings,
                          FlightStatusData     *flightStatus,
                          BiflightSettingsData *biflightSettings,
                          BiflightStatusData   *biflightStatus,
                          float                *pLeftServoVal,
						  float                *pRightServoVal,
                          bool armed,
                          float dT);
/**
 * @}
 * @}
 */
