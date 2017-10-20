/**
 ******************************************************************************
 * @file       unittest.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup UnitTests
 * @{
 * @addtogroup UnitTests
 * @{
 * @brief Unit test
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

/*
 * NOTE: This program uses the Google Test infrastructure to drive the unit test
 *
 * Main site for Google Test: http://code.google.com/p/googletest/
 * Documentation and examples: http://code.google.com/p/googletest/wiki/Documentation
 */

#include "gtest/gtest.h"

#include <stdio.h>		/* printf */
#include <stdlib.h>		/* abort */
#include <string.h>		/* memset */
#include <stdint.h>		/* uint*_t */

extern "C" {

#include "coordinate_conversions.h" /* API for coordinate_conversions functions */

}

#include <math.h>		/* fabs() */

// To use a test fixture, derive a class from testing::Test.
class CoordConversion : public testing::Test {
protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

// Test fixture for Rne From LLA, minimal
class RneFromLLATest : public CoordConversion {
protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(RneFromLLATest, Equator) {
  float LLA[] = { 0, 0, 0 };
  float Rne[3][3];

  RneFromLLA(LLA, Rne);

  float eps = 0.000001f;
  ASSERT_NEAR(0, Rne[0][0], eps);
  ASSERT_NEAR(0, Rne[0][1], eps);
  ASSERT_NEAR(1, Rne[0][2], eps);

  ASSERT_NEAR(0, Rne[1][0], eps);
  ASSERT_NEAR(1, Rne[1][1], eps);
  ASSERT_NEAR(0, Rne[1][2], eps);

  ASSERT_NEAR(-1, Rne[2][0], eps);
  ASSERT_NEAR(0, Rne[2][1], eps);
  ASSERT_NEAR(0, Rne[2][2], eps);
};

class QuatRPYTest : public CoordConversion {
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(QuatRPYTest, BackAndForth) {
	float eps = 0.8f;

	float quat[4];
	float rpy[3];
	float rpy_out[3];

	rpy[0] = 1; rpy[1] = 1; rpy[2] = 1;

	RPY2Quaternion(rpy, quat);
	Quaternion2RPY(quat, rpy_out);

	for (int i = 0; i < 3; i++) {
		ASSERT_NEAR(rpy[i], rpy_out[i], eps);
	}

	rpy[0] = 179; rpy[1] = 1; rpy[2] = 1;

	RPY2Quaternion(rpy, quat);
	Quaternion2RPY(quat, rpy_out);

	for (int i = 0; i < 3; i++) {
		ASSERT_NEAR(rpy[i], rpy_out[i], eps);
	}

	rpy[0] = 40; rpy[1] = 89; rpy[2] = 0;

	RPY2Quaternion(rpy, quat);
	Quaternion2RPY(quat, rpy_out);

	for (int i = 0; i < 3; i++) {
		ASSERT_NEAR(rpy[i], rpy_out[i], eps);
	}

	rpy[0] = 45; rpy[1] = 89; rpy[2] = 0;

	RPY2Quaternion(rpy, quat);
	Quaternion2RPY(quat, rpy_out);

	for (int i = 0; i < 3; i++) {
		ASSERT_NEAR(rpy[i], rpy_out[i], eps);
	}

	rpy[0] = -179; rpy[1] = -89; rpy[2] = 0;

	RPY2Quaternion(rpy, quat);
	Quaternion2RPY(quat, rpy_out);

	for (int i = 0; i < 3; i++) {
		ASSERT_NEAR(rpy[i], rpy_out[i], eps);
	}

	rpy[0] = 0; rpy[1] = -90; rpy[2] = 0;

	RPY2Quaternion(rpy, quat);
	Quaternion2RPY(quat, rpy_out);

	for (int i = 0; i < 3; i++) {
		ASSERT_NEAR(rpy[i], rpy_out[i], eps);
	}

	rpy[0] = 90; rpy[1] = -90; rpy[2] = 0;

	RPY2Quaternion(rpy, quat);
	Quaternion2RPY(quat, rpy_out);

	for (int i = 0; i < 3; i++) {
		ASSERT_NEAR(rpy[i], rpy_out[i], eps);
	}

	rpy[0] = -130; rpy[1] = -90; rpy[2] = 90;

	RPY2Quaternion(rpy, quat);
	Quaternion2RPY(quat, rpy_out);

	ASSERT_NEAR(rpy[1], rpy_out[1], eps);

	for (float f = -179.5; f < 179.5f; f += 0.1f) {
		rpy[0] = f; rpy[1] = -90; rpy[2] = 0;

		RPY2Quaternion(rpy, quat);
		Quaternion2RPY(quat, rpy_out);

		for (int i = 0; i < 3; i++) {
			ASSERT_NEAR(rpy[i], rpy_out[i], eps);
		}
	}
}
