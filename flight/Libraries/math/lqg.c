/**
 ******************************************************************************
 * @addtogroup Libraries Libraries
 * @{
 * @addtogroup FlightMath math support libraries
 * @{
 *
 * @file       lqg.c
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

#include "lqg.h"

/*
	References
	=====================================================================================================

	General idea

		* Linear Gaussian Quadratic Control System (James Cotton (peabody124), Taulabs)
		  http://buildandcrash.blogspot.be/2016/10/linear-gaussian-quadratic-control-system.html

	Covariance calculation for LQR, instead of DARE

		* ECE5530: Multivariable Control Systems II (Dr. Gregory L. Plett)
		  http://mocha-java.uccs.edu/ECE5530/ECE5530-CH03.pdf
		  (Page 34)

*/

#define RTKF_SOLUTION_LIMIT			150
#define LQR_SOLUTION_LIMIT			1000

/* Bullshit to quickly make copypasta of MATLAB answers work. */
#define P00 P[0][0]
#define P10 P[1][0]
#define P20 P[2][0]
#define P01 P[0][1]
#define P11 P[1][1]
#define P21 P[2][1]
#define P02 P[0][2]
#define P12 P[1][2]
#define P22 P[2][2]

#define A00 A[0][0]
#define A10 A[1][0]
#define A20 A[2][0]
#define A01 A[0][1]
#define A11 A[1][1]
#define A21 A[2][1]
#define A02 A[0][2]
#define A12 A[1][2]
#define A22 A[2][2]

#define Q00 Q[0][0]
#define Q10 Q[1][0]
#define Q20 Q[2][0]
#define Q01 Q[0][1]
#define Q11 Q[1][1]
#define Q21 Q[2][1]
#define Q02 Q[0][2]
#define Q12 Q[1][2]
#define Q22 Q[2][2]

#define B0 B[0]
#define B1 B[1]
#define B2 B[2]

#define X0 X[0]
#define X1 X[1]
#define X2 X[2]

#define K0 K[0]
#define K1 K[1]
#define K2 K[2]

/*
	Kalman covariance cycle.

	P = APA' + Q
	K = PH' (R+HPH')^-1
	P = (I-KH) P
*/
void rtkf_calculate_covariance_3x3(float A[3][3], float K[3], float P[3][3], float Q[3][3], float R)
{
	float nP[3][3];

	nP[0][0] = Q00 + A00*(A00*P00 + A01*P10 + A02*P20) + A01*(A00*P01 + A01*P11 + A02*P21) + A02*(A00*P02 + A01*P12 + A02*P22);
	nP[0][1] = A11*(A00*P01 + A01*P11 + A02*P21) + A12*(A00*P02 + A01*P12 + A02*P22);
	nP[0][2] = A22*(A00*P02 + A01*P12 + A02*P22);

	nP[1][0] = A00*(A11*P10 + A12*P20) + A01*(A11*P11 + A12*P21) + A02*(A11*P12 + A12*P22);
	nP[1][1] = Q11 + A11*(A11*P11 + A12*P21) + A12*(A11*P12 + A12*P22);
	nP[1][2] = A22*(A11*P12 + A12*P22);

	nP[2][0] = A22*(A00*P20 + A01*P21 + A02*P22);
	nP[2][1] = A22*(A11*P21 + A12*P22);
	nP[2][2] = P22*A22*A22 + Q22;

	memcpy(P, nP, sizeof(float)*9);

	float S = P00 + R;

	K0 = P00/S;
	K1 = P10/S;
	K2 = P20/S;

	P10 = P10 - K1*P00;
	P11 = P11 - K1*P01;
	P12 = P12 - K1*P02;

	P20 = P20 - K2*P00;
	P21 = P21 - K2*P01;
	P22 = P22 - K2*P02;

	P00 = P00 - K0*P00;
	P01 = P01 - K0*P01;
	P02 = P02 - K0*P02;
 }

struct rtkf_state {

	int solver_iterations;

	float A[3][3];
	float B[3];
	float K[3];
	float P[3][3];
	float Q[3][3];
	float R;
	float X[3];

	float biaslim;

};

/*
	Repeat the Kalman covariance cycle for specified amount of iterations.

	Tests show that 50 iterations should be plenty to become stable.
*/
 void rtkf_stabilize_covariance(rtkf_t rtkf, int iterations)
 {
 	PIOS_Assert(rtkf);
 	for (int i = 0; i < iterations; i++) {
 		rtkf_calculate_covariance_3x3(rtkf->A, rtkf->K, rtkf->P, rtkf->Q, rtkf->R);
 	}
 	rtkf->solver_iterations += iterations;
 }

/*
	Kalman prediction

	X_k+1 = AX_k + Bu_k + K(y - HX)
*/
void rtkf_prediction_step(float A[3][3], float B[3], float K[3], float X[3], float signal, float input)
{
	float nX0, nX1, nX2;

	nX0 = B0*input + A00*X0 + A01*X1 + A02*X2;
	nX1 = B1*input + A11*X1 + A12*X2;
	nX2 = B2*input + A22*X2;

	X0 = nX0 - K0*(nX0 - signal);
	X1 = nX1 - K1*(nX0 - signal);
	float in2 = input*input;
	X2 = (nX2 - K2*(nX0 - signal)) * bound_min_max(1.0f - in2*in2, 0, 1.0f);
}

void rtkf_predict_axis(rtkf_t rtkf, float signal, float input, float Xout[3])
{
	rtkf_prediction_step(rtkf->A, rtkf->B, rtkf->K, rtkf->X, signal, input);

	rtkf->X2 = bound_sym(rtkf->X2, rtkf->biaslim);

	Xout[0] = rtkf->X0;
	Xout[1] = rtkf->X1;
	Xout[2] = rtkf->X2;
}

/*
	Rate-torque Kalman filter system.

	A = [   1   Beta*(tau-tau*e^(-Ts/tau))   -Ts*Beta+Beta*(tau-tau*e^(-Ts/tau))   ]
	    [   0   e^(-Ts/tau)                  e^(-Ts/tau)-1                         ]
	    [   0   0                            1                                     ]

	B = [   Ts*Beta-Beta*(tau-tau*e^(-Ts/tau))   ]
	    [   1-e^(-Ts/tau)                        ]
	    [   0                                    ]
*/
void rtkf_initialize_matrices_int(float A[3][3], float B[3], float beta, float tau, float Ts)
{
	A00 = 1;
	A01 = beta*(tau - tau*expf(-Ts/tau));
	A02 = beta*(tau - tau*expf(-Ts/tau)) - Ts*beta;
	A11 = expf(-Ts/tau);
	A12 = expf(-Ts/tau) - 1;
	A22 = 1;

	B0 = Ts*beta - beta*(tau - tau*expf(-Ts/tau));
	B1 = 1 - expf(-Ts/tau);
	B2 = 0;
}

/*
	Q matrix set by experimentation.

	R = 1000 seems a workable value for raw gyro input.
*/
rtkf_t rtkf_create(float beta, float tau, float Ts, float R, float q1, float q2, float q3, float biaslim)
{
	struct rtkf_state *state = PIOS_malloc_no_dma(sizeof(*state));
	PIOS_Assert(state);
	memset(state, 0, sizeof(*state));

	rtkf_initialize_matrices_int(state->A, state->B, expf(beta), tau, Ts);
	state->Q00 = q1;
	state->Q11 = q2;
	state->Q22 = q3;
	state->R = R;
	state->biaslim = biaslim;

	return state;
}

bool rtkf_is_solved(rtkf_t rtkf)
{
	return rtkf->solver_iterations >= RTKF_SOLUTION_LIMIT;
}

/*
	LQR covariance cycle.

	P = A'PA - A'PB (R+B'PB)^-1 B'PA + Q
*/
void lqr_calculate_covariance_2x2(float A[2][2], float B[2], float P[2][2], float Q[2][2], float R)
{
	float nP[2][2];

	float B0B0 = B0*B0;
	float B1B1 = B1*B1;
	float B0B1 = B0*B1;
	float A00A00 = A00*A00;
	float A01A01 = A01*A01;
	float A11A11 = A11*A11;
	float P01P10 = P01*P10;
	float P00P11 = P00*P11;

	float div = (R + B0B0*P00 + B1B1*P11 + B0B1*(P01 + P10));

	nP[0][0] = (Q00*R + P00*(A00A00*R + B0B0*Q00) +
		B1B1*(P11*Q00 + A00A00*(P00P11 - P01P10)) +
		B0B1*Q00*(P01 + P10))
	    /
	    div;

	float common = A01*(P00*R + B1B1*(P00P11 - P01P10)) - A11*B0B1*(P00P11 + P01P10);

	nP[1][0] = (A00*(A11*P10*R + common))
	    /
	    div;

	nP[0][1] = (A00*(A11*P01*R + common))
	    /
	    div;

	nP[1][1] = (Q11*R + A01A01*P00*R + B0B0*P00*Q11 +
		A11A11*(P11*R + B0B0*(P00P11 - P01P10)) +
		B1B1*(P11*Q11 + A01A01*P00P11 - A01A01*P01P10) +
		A01*A11*P01*R + A01*A11*P10*R + B0B1*P01*Q11 +
		B0B1*(P10*Q11 - 2*A01*A11*P00P11 + 2*A01*A11*P01P10))
		/
		div;

	P00 = nP[0][0];
	P01 = nP[0][1];
	P10 = nP[1][0];
	P11 = nP[1][1];
}

struct lqr_state {

	int solver_iterations;

	float A[2][2];
	float B[2];
	float K[2];
	float P[2][2];
	float R;
	float Q[2][2];
	float u;

	float beta;
	float tau;

};

/*
	Calculate gains for LQR.

	K = (R + B'PB)^-1 B'PA
*/
void lqr_calculate_gains_int(float A[2][2], float B[2], float P[2][2], float K[2], float R)
{
	float div = (R + B1*B1*P11 + B0*(B0*P00 + B1*(P01 + P10)));

	K[0] = (A00*(B0*P00 + B1*P10)) / div;
	K[1] = (A01*(B0*P00 + B1*P10) + A11*(B0*P01 + B1*P11)) / div;
}

/*
	Repeat the LQR covariance cycle for as much iterations needed.

	Discrete algrebraic Riccati "solver" for dumb people.

	Experimentation shows that it needs at least 700-800 iterations to become stable. Stabilizing
	two rate-controller LQR systems (say for roll and pitch axis) with 1000 iterations works
	within a fraction of a second on an STM32F405.

	The end results pretty much match covariances calculated with dare() in MATLAB. Calculated
	gains from this also match what dlqr() spits out.

	Trying peabody124's old branch last year, his proper DARE solver using Eigen took the best of
	five seconds. It was a tri-state LQR though.

	Changing the Q state weight matrix in middle operation usually seems to restabilize within
	a 100 cycles, so TxPID for tuning might qualify.
*/
void lqr_stabilize_covariance(lqr_t lqr, int iterations)
{
	PIOS_Assert(lqr);

	for (int i = 0; i < iterations; i++) {
		lqr_calculate_covariance_2x2(lqr->A, lqr->B, lqr->P, lqr->Q, lqr->R);
	}

	lqr_calculate_gains_int(lqr->A, lqr->B, lqr->P, lqr->K, lqr->R);

	lqr->solver_iterations += iterations;
}

bool lqr_is_solved(lqr_t lqr)
{
	return lqr->solver_iterations >= LQR_SOLUTION_LIMIT;
}

/*
	LQR rate controller.

	A = [   1   tau*Beta*(1-e^(-Ts/tau))   ]
	    [   0   e^(-Ts/tau)                ]

	B = [   Ts*Beta-tau*Beta*(1-e^(-Ts/tau))   ]
	    [   1-e^(-Ts/tau)                      ]
*/
void lqr_initialize_matrices_int(float A[2][2], float B[2], float beta, float tau, float Ts)
{
	A00 = 1;
	A01 = -beta*tau*(expf(-Ts/tau) - 1);
	A10 = 0;
	A11 = expf(-Ts/tau);

	B0 = Ts*beta + beta*tau*(expf(-Ts/tau) - 1);
	B1 = 1 - expf(-Ts/tau);
}

/*
	Q1 and Q2 in the state weighting matrix penalizes rate and torque.

	Need to talk to peabody124 about the difference in magnitude. Q2 uses frame invariance idea.

	Current workable values for 5" miniquads seems to be Q1 = 0.00001, Q2 = 0.00013333.
*/
lqr_t lqr_create(float beta, float tau, float Ts, float q1, float q2, float r)
{
	struct lqr_state *state = PIOS_malloc_no_dma(sizeof(*state));
	PIOS_Assert(state);
	memset(state, 0, sizeof(*state));

	lqr_initialize_matrices_int(state->A, state->B, expf(beta), tau, Ts);
	state->Q00 = q1;
	state->Q11 = q2*expf(beta);
	state->R = r;

	state->beta = beta;
	state->tau = tau;

	return state;
}

void lqr_update(lqr_t lqr, float q1, float q2, float r)
{
	PIOS_Assert(lqr);

	lqr->Q00 = q1;
	lqr->Q11 = q2*expf(lqr->beta);
	lqr->R = r;
	lqr->solver_iterations = 0;
}

void lqr_get_gains(lqr_t lqr, float K[2])
{
	PIOS_Assert(lqr);
	K[0] = lqr->K[0];
	K[1] = lqr->K[1];
}


struct lqg_state {

	int axis;

	struct lqr_state *lqr;
	struct rtkf_state *rtkf;

};

/*
	Estimate the system state, i.e. presumed true rate and torque, then
	feed it into the LQR.

	xr = x - setpoint

	u = -K*xr + bias

	u gets clamped to -1..1 because that's the actuator range. Might also prevent the
	RTKF to go overboard with predicting, when the LQR is trying to demand a lot from the
	actuators.
*/
float lqg_controller(lqg_t lqg, float signal, float setpoint)
{
	lqr_t lqr = lqg->lqr;
	rtkf_t rtkf = lqg->rtkf;

	float x_est[3]; /* Rate, Torque, Bias */
	rtkf_predict_axis(rtkf, signal, lqr->u, x_est);

	float xr0 = x_est[0] - setpoint;

	float u = x_est[2] - lqr->K0 * xr0 - lqr->K1 * x_est[1];
	if (u < -1) u = -1;
	else if (u > 1) u = 1;

	lqr->u = u;

	return u;
}

lqg_t lqg_create(rtkf_t rtkf, lqr_t lqr)
{
	PIOS_Assert(rtkf);
	PIOS_Assert(lqr);

	struct lqg_state *state = PIOS_malloc_no_dma(sizeof(*state));
	PIOS_Assert(state);
	memset(state, 0, sizeof(*state));

	state->rtkf = rtkf;
	state->lqr = lqr;

	return state;
}

void lqg_get_rtkf_state(lqg_t lqg, float *rate, float *torque, float *bias)
{
	*rate = lqg->rtkf->X[0];
	*torque = lqg->rtkf->X[1];
	*bias = lqg->rtkf->X[2];
}

void lqg_set_x0(lqg_t lqg, float x0)
{
	lqg->rtkf->X[0] = x0;
	lqg->rtkf->X[1] = 0;
	lqg->rtkf->X[2] = 0;
}

bool lqg_is_solved(lqg_t lqg)
{
	return lqg != NULL && lqr_is_solved(lqg->lqr) && rtkf_is_solved(lqg->rtkf);
}

rtkf_t lqg_get_rtkf(lqg_t lqg)
{
	return lqg ? lqg->rtkf : NULL;
}

lqr_t lqg_get_lqr(lqg_t lqg)
{
	return lqg ? lqg->lqr : NULL;
}

void lqg_run_covariance(lqg_t lqg, int iter)
{
	rtkf_t rtkf = lqg_get_rtkf(lqg);
	lqr_t lqr = lqg_get_lqr(lqg);
	PIOS_Assert(rtkf);
	PIOS_Assert(lqr);

	if (!rtkf_is_solved(rtkf))
		rtkf_stabilize_covariance(rtkf, iter);
	if (!lqr_is_solved(lqr))
		lqr_stabilize_covariance(lqr, iter);
}

/**
 * @}
 * @}
 */
