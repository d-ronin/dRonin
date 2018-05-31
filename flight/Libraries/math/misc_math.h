/**
 ******************************************************************************
 * @addtogroup Libraries Libraries
 * @{
 * @addtogroup FlightMath math support libraries
 * @{
 *
 * @file       math_misc.h
 * @author     dRonin, http://dRonin.org, Copyright (C) 2017
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      Miscellaneous math support
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

#ifndef MISC_MATH_H
#define MISC_MATH_H

#include <math.h>

#include "stdint.h"
#include "stdbool.h"

// Max/Min macros. Taken from http://stackoverflow.com/questions/3437404/min-and-max-in-c
#define MAX(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

//! This is but one definition of sign(.)
#define sign(x) ((x) < 0 ? -1 : 1)

//! Bound input value within range (plus or minus)
float bound_sym(float val, float range);

//! Bound input value between min and max
float bound_min_max(float val, float min, float max);

//! Circular modulus
float circular_modulus_deg(float err);
float circular_modulus_rad(float err);

//! Approximation an exponential scale curve
float expo3(float x, int32_t g);
float expoM(float x, int32_t g, float exponent);

float interpolate_value(const float fraction, const float beginVal,
			const float endVal);
float vectorn_magnitude(const float *v, int n);
float vector3_distances(const float *actual,
		        const float *desired, float *out, bool normalize);
void vector2_clip(float *vels, float limit);
void vector2_rotate(const float *original, float *out, float angle);
float cubic_deadband(float in, float w, float b, float m, float r);
void cubic_deadband_setup(float w, float b, float *m, float *r);
float linear_interpolate(float const input, float const * curve, uint8_t num_points, const float input_min, const float input_max);
void randomize_addseed(uint32_t seed);
uint32_t randomize_int(uint32_t interval);
void apply_channel_deadband(float *value, float deadband);

/* Note--- current compiler chain has been verified to produce proper call
 * to fpclassify even when compiling with -ffast-math / -ffinite-math.
 * Previous attempts were made here to limit scope of disabling those
 * optimizations to this function, but were infectious and increased
 * stack usage across unrelated code because of compiler limitations.
 * See TL issue #1879.
 */
static inline bool IS_NOT_FINITE(float x) {
	return (!isfinite(x));
}

/** @brief Fast approximation of sine; 3rd order taylor expansion.
 * Based on http://www.coranac.com/2009/07/sines/
 * @param[in] x angle, 2^15 units/circle
 * @returns approximately sin(x / 2^15 * 2 * PI) in Q12 form
 */
static inline int16_t sin_approx(int32_t x)
{
#define s_qN 13
#define s_qP 15
#define s_qA 12
	static const int 
		     qR= 2*s_qN - s_qP,
		     qS= s_qN + s_qP + 1 - s_qA;

	x= x<<(30-s_qN);          // shift to full s32 range (Q13->Q30)

	if( (x^(x<<1)) < 0)     // test for quadrant 1 or 2
		x= (1<<31) - x;

	x= x>>(30-s_qN);

	return x * ( (3<<s_qP) - (x*x>>qR) ) >> qS;
}

/** @brief Multiplies out = a b
 *
 * Matrices are stored in row order, that is a[i*cols + j]
 * a, b, and output must be distinct.
 *
 * @param[in] a multiplicand dimensioned arows by acolsbrows
 * @param[in] b multiplicand dimensioned acolsbrows by bcols
 * @param[out] output product dimensioned arows by bcols
 * @param[in] arows matrix dimension
 * @param[in] acolsbrows matrix dimension
 * @param[in] bcols matrix dimension
 */
static inline void matrix_mul(const float *a, const float *b,
		float *out, int arows, int acolsbrows, int bcols)
{
	const float *restrict apos = a;
	float *restrict opos = out;

	for (int ar = 0; ar < arows; ar++) {
		for (int bc = 0 ; bc < bcols; bc++) {
			float sum = 0;

			const float *restrict bpos = b + bc;

			int acbr;

			/* This is manually unrolled.  -Os doesn't really want to
			 * unroll things, but this is a case where we want to
			 * encourage it to.
			 *
			 * This structure was chosen / experimented with across
			 * many compilers to always do the right things-- namely
			 *
			 * A) vectorize when appropriate on the architecture
			 * B) eliminate the loop when there's a static, small number
			 * of cols (10 or less)
			 * C) generate pretty good code when the dimensions aren't
			 * known.
			 */
			for (acbr = 0; acbr < (acolsbrows & (~3)); acbr += 4) {
				/*
				sum += a[ar * acolsbrows + acbr] *
					b[acbr * bcols + bc];
				*/

				sum += (apos[acbr]) * (*bpos);
				bpos += bcols;
				sum += (apos[acbr+1]) * (*bpos);
				bpos += bcols;
				sum += (apos[acbr+2]) * (*bpos);
				bpos += bcols;
				sum += (apos[acbr+3]) * (*bpos);
				bpos += bcols;
			}

			for (; acbr < acolsbrows; acbr++) {
				/*
				sum += a[ar * acolsbrows + acbr] *
					b[acbr * bcols + bc];
				*/

				sum += (apos[acbr]) * (*bpos);
				bpos += bcols;
			}
			/* out[ar * bcols + bc] = sum; */

			*opos = sum;

			opos++;
		}

		apos += acolsbrows;
	}
}

/** @brief Multiplies a matrix by a scalar
 * Can safely be done in-place. (e.g. is slow and not vectorized/unrolled)
 *
 * @param[in] a Matrix of dimension rows x cols
 * @param[in] scalar the amount to multiply by
 * @param[out] out Product matrix of dimensions rows x cols
 * @param[in] rows Matrix dimension
 * @param[in] cols Matrix dimension
 */
static inline void matrix_mul_scalar(const float *a, float scalar, float *out,
		int rows, int cols)
{
	int size = rows * cols;

	const float * apos = a;

	for (int i = 0; i < size; i++) {
		*out = (*apos) * scalar;
		apos++; out++;
	}
}

/** @brief Adds two matrices
 * Can safely be done in-place. (e.g. is slow and not vectorized/unrolled)
 *
 * @param[in] a Matrix of dimension rows x cols
 * @param[in] b Matrix of dimension rows x cols
 * @param[in] scalar the amount to multiply by
 * @param[out] out Matrix of dimensions rows x cols
 * @param[in] rows Matrix dimension
 * @param[in] cols Matrix dimension
 */
static inline void matrix_add(const float *a, const float *b,
		float *out, int rows, int cols)
{
	int size = rows * cols;

	const float * apos = a;
	const float * bpos = b;
	float * opos = out;

	for (int i = 0; i < size; i++) {
		*opos = (*apos) + (*bpos);
		apos++; bpos++; opos++;
	}
}

/** @brief Subtracts two matrices
 * Can safely be done in-place. (e.g. is slow and not vectorized/unrolled)
 *
 * @param[in] a Matrix of dimension rows x cols
 * @param[in] b Matrix of dimension rows x cols
 * @param[in] scalar the amount to multiply by
 * @param[out] out Matrix of dimensions rows x cols
 * @param[in] rows Matrix dimension
 * @param[in] cols Matrix dimension
 */
static inline void matrix_sub(const float *a, const float *b,
		float *out, int rows, int cols)
{
	int size = rows * cols;

	const float * apos = a;
	const float * bpos = b;
	float * opos = out;

	for (int i = 0; i < size; i++) {
		*opos = (*apos) - (*bpos);
		apos++; bpos++; opos++;
	}
}

/** @brief Transposes a matrix
 *
 * Not safe in place.
 *
 * @param[in] a Matrix of dimension arows x acols
 * @param[out] out Matrix of dimension acols x arows
 * @param[in] arows Matrix dimension
 * @param[in] acols Matrix dimension
 */
static inline void matrix_transpose(const float *a, float *out, int arows,
		int acols)
{
	for (int i = 0; i < arows; i++) {
		for (int j = 0; j < acols; j++) {
			out[j * arows + i] = a[i * acols + j];
		}
	}
}

#define TOL_EPS 0.0001f

/* Not intended for outside use (at least just yet).  Sees whether
 * a and b look like good inverses.
 */
static inline bool matrix_pseudoinv_convergecheck(const float *a,
		const float *b, const float *prod, int rows, int cols)
{
	const float *pos = prod;
	bool ret = true;

	/* Product (desired pseudo identity matrix) is cols*cols */
	for (int i = 0; i < cols; i++) {
		float sum = 0.0f;

		for (int j = 0; j < cols; j++) {
			sum += *pos;

			pos++;
		}

		/* It needs to be near 1 or 0 for us to be converged.
		 * First, return false if outside 0..1
		 */
		if (sum > (1 + TOL_EPS)) {
			ret = false;
			break;
		}

		if (sum < (0 - TOL_EPS)) {
			ret = false;
			break;
		}

		/* Next, continue if very close to 1 or 0 */
		if (sum > (1 - TOL_EPS)) {
			continue;
		}

		if (sum < (0 + TOL_EPS)) {
			continue;
		}

		/* We're between 0 and 1, outside of the tolerance */
		ret = false;
		break;
	}

	if (ret) {
		return true;
	}

	/* Sometimes the pseudoidentity matrix doesn't meet this constraint; fall
	 * back to verifying elements.
	 * (Which in itself is numerically problematic)
	 */

	int elems = rows * cols;

	for (int i = 0; i < elems; i++) {
		float diff = (a[i] - b[i]);

		/* This strange structure chosen because NaNs will return false
		 * to both of these...
		 */
		if (diff < TOL_EPS) {
			if (diff > -TOL_EPS) {
				continue;
			}
		}

		return false;
	}

	return true;
}

/* Not intended for outside use.  Performs one step of the pseudoinverse
 * approximation by calculating 2 * ainv - ainv * a * ainv */
static inline bool matrix_pseudoinv_step(const float *a, float *ainv,
		int arows, int acols)
{
	float prod[acols * acols];
	float invcheck[acols * arows];

	/* Calculate 2 * ainv - ainv * a * ainv */

	/* prod = ainv * a */
	matrix_mul(ainv, a, prod, acols, arows, acols);

	/* invcheck = prod * ainv */
	matrix_mul(prod, ainv, invcheck, acols, acols, arows);

	if (matrix_pseudoinv_convergecheck(ainv, invcheck, prod, arows, acols)) {
		return true;
	}

	/* ainv_ = 2 * ainv */
	matrix_mul_scalar(ainv, 2, ainv, acols, arows);

	/* ainv__ = ainv_ - invcheck */
	/* AKA expanded ainv__ = 2 * ainv - ainv * a * ainv */
	matrix_sub(ainv, invcheck, ainv, acols, arows);

	return false;
}

/** @brief Finds the largest value in a matrix.
 *
 * @param[in] a Matrix of dimension arows x acols
 * @param[in] arows Matrix dimension
 * @param[in] acols Matrix dimension
 */
static inline float matrix_getmaxabs(const float *a, int arows, int acols)
{
	float mx = 0.0f;

	const int size = arows * acols;

	for (int i = 0; i < size; i ++) {
		float val = fabsf(a[i]);

		if (val > mx) {
			mx = val;
		}
	}

	return mx;
}

#ifndef PSEUDOINV_CONVERGE_LIMIT
#define PSEUDOINV_CONVERGE_LIMIT 75
#endif

/** @brief Finds a pseudo-inverse of a matrix.
 *
 * This is an inverse when an inverse exists, and is equivalent to least
 * squares when the system is overdetermined or underdetermined.  AKA happiness.
 *
 * It works by iterative approximation.  Note it may run away or orbit the
 * solution in a few degenerate cases and return failure.
 *
 * @param[in] a Matrix of dimension arows x acols
 * @param[out] out Pseudoinverse matrix of dimensions arows x acols
 * @param[in] arows Matrix dimension
 * @param[in] acols Matrix dimension
 * @returns true if a good pseudoinverse was found, false otherwise.
 */
static inline bool matrix_pseudoinv(const float *a, float *out,
		int arows, int acols)
{
	matrix_transpose(a, out, arows, acols);

	float scale = matrix_getmaxabs(a, arows, acols);

	matrix_mul_scalar(out, 0.01f / scale, out, acols, arows);

	for (int i = 0; i < PSEUDOINV_CONVERGE_LIMIT; i++) {
		if (matrix_pseudoinv_step(a, out, arows, acols)) {
			/* Do one more step when we look pretty good! */
			matrix_pseudoinv_step(a, out, arows, acols);

			return true;
		}
	}

	return false;
}

/* The following are wrappers which attempt to do compile-time checking of
 * array dimensions.
 */
#define matrix_mul_check(a, b, out, arows, acolsbrows, bcols) \
	do { \
		/* Note the dimensions each get used multiple times */	\
		const float (*my_a)[arows*acolsbrows] = &(a); \
		const float (*my_b)[acolsbrows*bcols] = &(b); \
		float (*my_out)[arows*bcols] = &(out); \
		matrix_mul(*my_a, *my_b, *my_out, arows, acolsbrows, bcols); \
	} while (0);

#define matrix_mul_scalar_check(a, scalar, out, rows, cols) \
	do { \
		/* Note the dimensions each get used multiple times */	\
		const float (*my_a)[rows*cols] = &(a); \
		float (*my_out)[rows*cols] = &(out); \
		matrix_mul_scalar(*my_a, scalar, my_out, rows, cols); \
	} while (0);

#define matrix_add_check(a, b, out, rows, cols) \
	do { \
		/* Note the dimensions each get used multiple times */	\
		const float (*my_a)[rows*cols] = &(a); \
		const float (*my_b)[rows*cols] = &(b); \
		float (*my_out)[rows*cols] = &(out); \
		matrix_add(*my_a, *my_b, *my_out, rows, cols) \
	} while (0);

#define matrix_sub_check(a, b, out, rows, cols) \
	do { \
		/* Note the dimensions each get used multiple times */	\
		const float (*my_a)[rows*cols] = &(a); \
		const float (*my_b)[rows*cols] = &(b); \
		float (*my_out)[rows*cols] = &(out); \
		matrix_sub(*my_a, *my_b, *my_out, rows, cols) \
	} while (0);

#define matrix_transpose_check(a, b, out, rows, cols) \
	do { \
		/* Note the dimensions each get used multiple times */	\
		const float (*my_a)[rows*cols] = &(a); \
		const float (*my_b)[rows*cols] = &(b); \
		float (*my_out)[rows*cols] = &(out); \
		matrix_transpose(*my_a, *my_b, *my_out, rows, cols) \
	} while (0);

/* Following functions from fastapprox https://code.google.com/p/fastapprox/
 * are governed by this license agreement: */

/*=====================================================================*
 *                   Copyright (C) 2011 Paul Mineiro                   *
 * All rights reserved.                                                *
 *                                                                     *
 * Redistribution and use in source and binary forms, with             *
 * or without modification, are permitted provided that the            *
 * following conditions are met:                                       *
 *                                                                     *
 *     * Redistributions of source code must retain the                *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer.                                       *
 *                                                                     *
 *     * Redistributions in binary form must reproduce the             *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer in the documentation and/or            *
 *     other materials provided with the distribution.                 *
 *                                                                     *
 *     * Neither the name of Paul Mineiro nor the names                *
 *     of other contributors may be used to endorse or promote         *
 *     products derived from this software without specific            *
 *     prior written permission.                                       *
 *                                                                     *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND              *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,         *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES               *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE             *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER               *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,                 *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES            *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE           *
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR                *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF          *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY              *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE             *
 * POSSIBILITY OF SUCH DAMAGE.                                         *
 *                                                                     *
 * Contact: Paul Mineiro <paul@mineiro.com>                            *
 *=====================================================================*/

#ifdef __cplusplus
#define cast_uint32_t static_cast<uint32_t>
#else
#define cast_uint32_t (uint32_t)
#endif

static inline float
fastlog2 (float x)
{
	union { float f; uint32_t i; } vx = { x };
	union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | (0x7e << 23) };
	float y = vx.i;
	y *= 1.0f / (1 << 23);

	return y - 124.22551499f
		- 1.498030302f * mx.f
		- 1.72587999f / (0.3520887068f + mx.f);
}

static inline float
fastpow2 (float p)
{
	float offset = (p < 0) ? 1.0f : 0.0f;
	int w = p;
	float z = p - w + offset;
	union { uint32_t i; float f; } v = { cast_uint32_t ((1 << 23) * (p + 121.2740838f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z)) };

	return v.f;
}


static inline float
fastpow (float x, float p)
{
	return fastpow2 (p * fastlog2 (x));
}

static inline float
fastexp (float p)
{
	  return fastpow2 (1.442695040f * p);
}

/* Use full versions for now on all targets */
#define powapprox powf
#define expapprox expf

#endif /* MISC_MATH_H */

/**
 * @}
 * @}
 */
