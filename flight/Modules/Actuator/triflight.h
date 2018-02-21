/**
 ******************************************************************************
 * @addtogroup TriflightModule Triflight Module
 * @{
 *
 * @file       triflight.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @brief      Trifight module. Specialty mixing for tricopters.
 *
 * This module computes corrections for the tricopter's yaw servo and rear
 * motor, based on the yaw servo performance and commanded/actual angle.
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
#include "triflightsettings.h"
#include "triflightstatus.h"

void triflightInit(ActuatorSettingsData  *actuatorSettings,
                   TriflightSettingsData *triflightSettings,
                   TriflightStatusData   *triflightStatus);

float getCorrectedServoValue(ActuatorSettingsData  *actuatorSettings,
                             TriflightSettingsData *triflightSettings,
                             TriflightStatusData   *triflightStatus,
                             float uncorrected_servo_value);

void feedbackServoStep(TriflightSettingsData *triflightSettings,
                       TriflightStatusData   *triflightStatus,
                       float dT);

void virtualServoStep(ActuatorSettingsData  *actuatorSettings,
                      TriflightSettingsData *triflightSettings,
                      TriflightStatusData   *triflightStatus,
                      float dT,
                      uint16_t servo_value);

float triGetMotorCorrection(ActuatorSettingsData  *actuatorSettings,
                            TriflightSettingsData *triflightSettings,
                            TriflightStatusData   *triflightStatus,
                            uint16_t servo_value);

void triTailTuneStep(ActuatorSettingsData  *actuatorSettings,
                     FlightStatusData      *flightStatus,
                     TriflightSettingsData *triflightSettings,
                     TriflightStatusData   *triflightStatus,
                     float                 *pServoVal,
                     bool armed,
                     float dT);

void virtualTailMotorStep(ActuatorSettingsData  *actuatorSettings,
                          TriflightSettingsData *triflightSettings,
                          TriflightStatusData   *triflightStatus,
                          float setpoint,
                          float dT);

void dynamicYaw(TriflightSettingsData *triflightSettings,
                TriflightStatusData   *triflightStatus);
/**
 * @}
 * @}
 */
