/**
 ******************************************************************************
 * @addtogroup TauLabsLibraries Tau Labs Libraries
 * @{
 * @addtogroup TauLabsMath Tau Labs math support libraries
 * @{
 *
 * @file       math_misc.h
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
uint16_t randomize_int(uint16_t interval);

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

// The well known 3rd order expansion of sine; 2^15 units/circle
// Based on http://www.coranac.com/2009/07/sines/
// Output is Q12
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

/* Multiplies out = a * b
 *
 * Matrices are stored in row order, that is a[i*cols + j]
 *
 * output is arows by bcols
 * a is arows by acolsbrows
 * b is acolsbrows by bcols
 */
static inline void matrix_mul(const float *a, const float *b,
		float *out, int arows, int acolsbrows, int bcols)
{
	const float * restrict apos = a;
	float * restrict opos = out;

	for (int ar = 0; ar < arows; ar++) {
		for (int bc = 0 ; bc < bcols; bc++) {
			float sum = 0;

			const float * restrict bpos = b + bc;

			int acbr;

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

#define matrix_mul_check(a, b, out, arows, acolsbrows, bcols) \
	do { \
		/* Note the dimensions each get used multiple times */	\
		const float (*my_a)[arows*acolsbrows] = &(a); \
		const float (*my_b)[acolsbrows*bcols] = &(b); \
		float (*my_out)[arows*bcols] = &(out); \
		matrix_mul(*my_a, *my_b, *my_out, arows, acolsbrows, bcols); \
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
