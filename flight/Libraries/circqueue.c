/**
 ******************************************************************************
 * @file       circqueue.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @brief Implements a 1 reader, 1 writer nonblocking circular queue
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

#include <circqueue.h>

struct circ_queue {
	uint16_t elem_size;	/**< Element size in octets */
	uint16_t num_elem;	/**< Number of elements in circqueue (capacity+1) */
	volatile uint16_t write_head;	/**< Element position writer is at */
	volatile uint16_t read_tail;	/**< Element position reader is at */

	/* head == tail: empty.
	 * head == tail-1: full.
	 */

	/* This is declared as a uint32_t for alignment reasons. */
	uint32_t contents[];		/**< Contents of the circula queue */
};

/** Allocate a new circular queue.
 * @param[in] elem_size The size of each element, as obtained from sizeof().
 * @param[in] num_elem The number of elements in the queue.  The capacity is
 * one less than this (it may not be completely filled.
 * @returns The handle to the circular queue.
 */
circ_queue_t circ_queue_new(uint16_t elem_size, uint16_t num_elem) {
	PIOS_Assert(elem_size > 0);
	PIOS_Assert(num_elem > 2);

	uint32_t size = elem_size * num_elem;

	/* PIOS_malloc_no_dma may not be safe for some later uses.. hmmm */
	struct circ_queue *ret = PIOS_malloc(sizeof(*ret) + size);

	memset(ret, 0, sizeof(*ret) + size);

	ret->elem_size = elem_size;
	ret->num_elem = num_elem;

	return ret;
}

/** Get a pointer to the current queue write position.
 * This position is unavailable to any present readers and may be filled in
 * with the desired data without respect to any synchronization.
 *
 * @param[in] q Handle to circular queue.
 * @returns The position for new data to be written to (of size elem_size).
 */
void *circ_queue_cur_write_pos(circ_queue_t q) {
	void *contents = q->contents;

	return contents + q->write_head * q->elem_size;
}

static inline uint16_t next_pos(uint16_t num_pos, uint16_t current_pos) {
	PIOS_Assert(current_pos < num_pos);

	current_pos++;
	/* Also save on uint16_t wrap */

	if (current_pos >= num_pos) {
		current_pos = 0;
	}

	return current_pos;
}

/** Makes the current block of data available to readers and advances write pos.
 * This may fail if the queue contain num_elems -1 elements, in which case the
 * advance may be retried in the future.  In this case, data already written to
 * write_pos is preserved and the advance may be retried (or overwritten with
 * new data).
 *
 * @param[in] q Handle to circular queue.
 * @returns 0 if the write succeeded, nonzero on error.
 */
int circ_queue_advance_write(circ_queue_t q) {
	uint16_t new_write_head = next_pos(q->num_elem, q->write_head);

	/* the head is not allowed to advance to meet the tail */
	if (new_write_head == q->read_tail) {
		return -1;	/* Full */

		/* Caller can either let the data go away, or try again to
		 * advance later */
	}

	q->write_head = new_write_head;
	return 0;
}

/** Returns a block of data to the reader.
 * The block is "claimed" until released with circ_queue_read_completed.
 * No new data is available until that call is made (instead the same
 * block-in-progress will be returned).
 *
 * @param[in] q Handle to circular queue.
 * @returns pointer to the data, or NULL if the queue is empty.
 */
void *circ_queue_read_pos(circ_queue_t q) {
	uint16_t read_tail = q->read_tail;
	void *contents = q->contents;

	if (q->write_head == read_tail) {
		/* There is nothing new to read.  */
		return NULL;
	}

	return contents + q->read_tail * q->elem_size;
}

/** Releases a block of read data obtained by circ_queue_read_pos.
 * Behavior is undefined if circ_queue_read_pos did not previously return
 * a block of data.
 *
 * @param[in] q Handle to the circular queue.
 */
void circ_queue_read_completed(circ_queue_t q) {
	/* Avoid multiple accesses to a volatile */
	uint16_t read_tail = q->read_tail;

	/* If this is being called, the queue had better not be empty--
	 * we're supposed to finish consuming this element after a prior call
	 * to circ_queue_read_pos
	 */
	PIOS_Assert(read_tail != q->write_head);

	q->read_tail = next_pos(q->num_elem, read_tail);
}
