/**
 ******************************************************************************
 * @file       unittest.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
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
#include <circqueue.h>
}

// To use a test fixture, derive a class from testing::Test.
class CircQueueTest : public testing::Test {
protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(CircQueueTest, CircQueueFillUp) {
  circ_queue_t q = circ_queue_new(sizeof(int), 10);

  ASSERT_TRUE(q);

  int *pos;
  int ret;

  pos = (int *)circ_queue_read_pos(q);
  /* Should be empty and fail... */
  
  EXPECT_FALSE(pos);

  for (int i=0; i<9; i++) {
    pos = (int *)circ_queue_cur_write_pos(q);
    ASSERT_TRUE(pos);

    int *samePos = (int *)circ_queue_cur_write_pos(q);
    ASSERT_TRUE(pos == samePos);

    ret = circ_queue_advance_write(q);
    EXPECT_FALSE(ret);

    *pos = i + 99;

    pos = (int *)circ_queue_read_pos(q);

    EXPECT_TRUE(pos && (*pos == 99));
  }

  pos = (int *)circ_queue_cur_write_pos(q);
  ASSERT_TRUE(pos);

  ret = circ_queue_advance_write(q);

  /* Should be failure, because queue is full */

  ASSERT_TRUE(ret);

  /* Take the first 5 of 9 things off ... */
  for (int i=99; i<104; i++) {
    pos = (int *) circ_queue_read_pos(q);

    EXPECT_TRUE(*pos == i);

    int *samepos = (int *) circ_queue_read_pos(q);

    EXPECT_TRUE(samepos == pos);

    circ_queue_read_completed(q);

    int *notsamepos = (int *) circ_queue_read_pos(q);

    EXPECT_FALSE(notsamepos == pos);
  }

  int add_counter=1000;

  /* Interleave removing and adding. */
  /* Adds 4 + 2 elements for the odd ones */
  for (int i=104; i<108; i++) {
    pos = (int *) circ_queue_read_pos(q);

    EXPECT_TRUE(*pos == i);

    if (i % 2) {
      int *writepos = (int *)circ_queue_cur_write_pos(q);
      *writepos = add_counter++;
      ret = circ_queue_advance_write(q);
    }

    EXPECT_TRUE(*pos == i);

    circ_queue_read_completed(q);

    int *writepos = (int *)circ_queue_cur_write_pos(q);
    *writepos = add_counter++;

    ret = circ_queue_advance_write(q);

    EXPECT_FALSE(ret);
  }

  /* Neutral in number of elements */
  for (int i=1000; i<1050; i++) {
    pos = (int *) circ_queue_read_pos(q);

    EXPECT_TRUE(pos && (*pos == i));

    int *writepos = (int *)circ_queue_cur_write_pos(q);
    *writepos = add_counter++;

    EXPECT_TRUE(pos && (*pos == i));

    if (pos) {
      circ_queue_read_completed(q);
    }

    ret = circ_queue_advance_write(q);

    EXPECT_FALSE(ret);
  }

  /* Take last 6 elements off */
  for (int i=1050; i<1056; i++) {
    pos = (int *) circ_queue_read_pos(q);

    EXPECT_TRUE(pos && (*pos == i));

    if (pos) {
      circ_queue_read_completed(q);
    }
  }

  pos = (int *) circ_queue_read_pos(q);

  EXPECT_FALSE(pos);
}

TEST_F(CircQueueTest, CircQueueSpin) {
  circ_queue_t q = circ_queue_new(sizeof(int), 99);

  int write_val=10, read_val=10;

  for (int stride=80; stride<99; stride++) {
    for (int i=0; i<120; i++) {
      for (int j=0; j<stride; j++) {
	int *writepos = (int *)circ_queue_cur_write_pos(q);
	*writepos = write_val++;
	int ret = circ_queue_advance_write(q);
	EXPECT_FALSE(ret);
      }

      for (int j=0; j<stride; j++) {
	int *readpos = (int *) circ_queue_read_pos(q);
	ASSERT_TRUE(readpos);
	EXPECT_EQ(*readpos, read_val);
	read_val++;

	circ_queue_read_completed(q);
      }
    }
  }
}
