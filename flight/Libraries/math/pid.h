/**
 ******************************************************************************
 * @addtogroup Libraries Libraries
 * @{
 * @addtogroup FlightMath math support libraries
 * @{
 *
 * @file       pid.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @brief      PID Control algorithms
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

#ifndef PID_H
#define PID_H

struct pid_deadband {
	float width;				// Deadband width in degrees per second.
	float slope;				// Deadband slope (0..1).
	float cubic_weight;			// Cubic weight.
	float integrated_response;	// Response at deadband edge.
};

//! 
struct pid {
	float p;
	float i;
	float d;
	float iLim;

	float dT;

	float iAccumulator;
	float lastErr;
	float lastDer;
};

//! Methods to use the pid structures
float pid_apply(struct pid *pid, const float err);
float pid_apply_antiwindup(struct pid *pid, const float err, float min_bound, float max_bound, float aw_bound);
float pid_apply_setpoint(struct pid *pid, struct pid_deadband *deadband, const float setpoint, const float measured);
float pid_apply_setpoint_antiwindup(struct pid *pid,
		struct pid_deadband *deadband, const float setpoint,
		const float measured, float min_bound, float max_bound, float aw_bound);
void pid_zero(struct pid *pid);
void pid_configure(struct pid *pid, float p, float i, float d, float iLim, float dT);
void pid_configure_derivative(float cutoff, float gamma);
void pid_configure_deadband(struct pid_deadband *deadband, float width, float slope);


#endif /* PID_H */

/**
 * @}
 * @}
 */
