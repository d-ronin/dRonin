/**
 ******************************************************************************
 * @addtogroup TauLabsLibraries Tau Labs Libraries
 * @{
 * @addtogroup TauLabsMath Tau Labs math support libraries
 * @{
 *
 * @file       math_misc.c
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdbool.h>
#include <math.h>
#include "misc_math.h" 		/* API declarations */
#include "physical_constants.h"

/**
 * Calculate the "cubic deadband" system parameters.
 * @param[in] The width of the deadband
 * @param[in] Slope of deadband at in=0, sane values between 0 and 1.
 * @param[out] m cubic weighting of function
 * @param[out] integrated response at in=w
 */
void cubic_deadband_setup(float w, float b, float *m, float *r)
{
	/* So basically.. we want the function to be tangent to the
	 ** linear sections-- have a slope of 1-- at -w and w.  In the
	 ** middle we want a slope of b.   So the cube here does all the
	 ** work b isn't doing. */
	*m = cbrtf((1-b)/(3*powf(w,2)));

	*r = *m * powf(w, 3) + b * w;
}

/**
 * Perform a linear interpolation over N points
 *
 * output range is defined by the curve vector
 * curve is defined in N intervals connecting N+1 points
 *
 * @param[in] input the input value, in [input_min,input_max]
 * @param[in] curve the array of points in the curve
 * @param[in] num_points the number of points in the curve
 * @param[in] input_min the lower range of the input values
 * @param[in] input_max the upper range of the input values
 * @return the output value, in [0,1]
 */
float linear_interpolate(float const input, float const * curve, uint8_t num_points, const float input_min, const float input_max)
{
	// shift our input [min,max] into the typical range [0,1]
	float scale = fmaxf( (input - input_min) / (input_max - input_min), 0.0f) * (float) (num_points - 1);
	// select a starting bin via truncation
	int idx1 = scale;
	// save the offset from the starting bin for linear interpolation
	scale -= (float)idx1;
	// select an ending bin (linear interpolation occurs between starting and ending bins)
	int idx2 = idx1 + 1;
	// if the ending bin bin is above the last bin
	if (idx2 >= num_points) {
		//clamp to highest entry in table
		idx2 = num_points -1;
		// if the starting bin is above the last bin
		if (idx1 >= num_points) {
			// we no longer do interpolation; instead, we just select the max point on the curve
			return curve[num_points-1];
		}
	}
	return curve[idx1] * (1.0f - scale) + curve[idx2] * scale;
}



/**
 * @}
 * @}
 */
