/**
 ******************************************************************************
 * @addtogroup Libraries Libraries
 * @{
 * @addtogroup FlightMath math support libraries
 * @{
 *
 * @file       lqg.h
 * @author     dRonin, http://dronin.org, Copyright (C) 2018
 * @brief      LQG Control algorithm
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

#ifndef LQG_H
#define LQG_H

#include <pios.h>
#include <misc_math.h>

#define LQG_SOLVER_FAILED -1
#define LQG_SOLVER_RUNNING 0
#define LQG_SOLVER_DONE 1

typedef struct rtkf_state* rtkf_t;
typedef struct lqr_state* lqr_t;

typedef struct lqg_state* lqg_t;

extern rtkf_t rtkf_create(float beta, float tau, float Ts, float R, float Q1, float Q2, float Q3, float biaslim);
extern void rtkf_stabilize_covariance(rtkf_t rtkf, int iterations);
extern int rtkf_solver_status(rtkf_t rtkf);

extern lqr_t lqr_create(float beta, float tau, float Ts, float q1, float q2, float r);
extern void lqr_stabilize_covariance(lqr_t lqr, int iterations);
extern int lqr_solver_status(lqr_t lqr);

extern void lqr_update(lqr_t lqr, float q1, float q2, float r);
extern void lqr_get_gains(lqr_t lqg, float K[2]);

extern lqg_t lqg_create(rtkf_t rtkf, lqr_t lqr);
extern int lqg_solver_status(lqg_t lqg);
extern lqr_t lqg_get_lqr(lqg_t lqg);
extern rtkf_t lqg_get_rtkf(lqg_t lqg);

extern void lqg_run_covariance(lqg_t lqg, int iter);

extern float lqg_controller(lqg_t lqg, float signal, float setpoint);

extern void lqg_set_x0(lqg_t lqq, float x0);
void lqg_get_rtkf_state(lqg_t lqg, float *rate, float *torque, float *bias);

#endif // LQG_H

/**
 * @}
 * @}
 */
