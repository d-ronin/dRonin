/**
 ******************************************************************************
 * @file       circqueue.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
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
	uint32_t contents[];		/**< Contents of the circular queue */
};

/** Allocate a new circular queue.
 * @param[in] elem_size The size of each element, as obtained from sizeof().
 * @param[in] num_elem The number of elements in the queue.  The capacity is
 * one less than this (it may not be completely filled).
 * @returns The handle to the circular queue.
 */
circ_queue_t circ_queue_new(uint16_t elem_size, uint16_t num_elem) {
	PIOS_Assert(elem_size > 0);
	PIOS_Assert(num_elem >= 2);

	uint32_t size = elem_size * num_elem;

	/* PIOS_malloc_no_dma may not be safe for some later uses.. hmmm */
	struct circ_queue *ret = PIOS_malloc(sizeof(*ret) + size);

	if (!ret)
		return NULL;

	memset(ret, 0, sizeof(*ret) + size);

	ret->elem_size = elem_size;
	ret->num_elem = num_elem;

	return ret;
}

/** Get a pointer to the current queue write position.
 * This position is unavailable to the reader and may be filled in
 * with the desired data without respect to any synchronization.
 * No promise is made that circ_queue_advance_write will succeed, though.
 *
 * Alternatively, in *contig we return the number of things that can be
 * filled in.  We promise that you can circ_queue_advance_write_multi that
 * many.  You can also always write one, but no promises you'll be able to
 * advance write.
 *
 * @param[in] q Handle to circular queue.
 * @param[out] contig The num elements available for contiguous write.
 * @param[out] avail The num elements available before a reader has
 * freed up more space.  (Includes wraparound/non-contig elems).
 * @returns The position for new data to be written to (of size elem_size).
 */
void *circ_queue_write_pos(circ_queue_t q, uint16_t *contig,
		uint16_t *avail) {
	void *contents = q->contents;
	uint16_t wr_head = q->write_head;
	uint16_t rd_tail = q->read_tail;

	if (contig) {
		if (rd_tail <= wr_head) {
			/* Avail is the num elems to the end of the buf */
			int16_t offset = 0;

			if (rd_tail == 0) {
				/* Can't advance to the beginning, because
				 * we'd catch our tail.  [only when tail
				 * perfectly wrapped] */
				offset = -1;
			}
			*contig = q->num_elem - wr_head + offset;
		} else {
			/* rd_tail > wr_head */
			/* wr_head is not allowed to advance to meet tail,
			 * so minus one */
			*contig = rd_tail - wr_head - 1;
		}
	}

	if (avail) {
		if (rd_tail <= wr_head) {
			/* To end of buf, to rd_tail, minus one. */
			*avail = q->num_elem - wr_head + rd_tail - 1;
		} else {
			/* Otherwise just to 1 before rd_tail. */
			*avail = rd_tail - wr_head - 1;
		}
	}

	return contents + wr_head * q->elem_size;
}

static inline uint16_t advance_by_n(uint16_t num_pos, uint16_t current_pos,
		uint16_t num_to_advance) {
	PIOS_Assert(current_pos < num_pos);
	PIOS_Assert(num_to_advance <= num_pos);

	uint32_t pos = current_pos + num_to_advance;

	if (pos >= num_pos) {
		pos -= num_pos;
	}

	return pos;

}

static inline uint16_t next_pos(uint16_t num_pos, uint16_t current_pos) {
	return advance_by_n(num_pos, current_pos, 1);
}

/** Makes multiple elements available to the reader.  Amt is expected to be
 * equal or less to an 'avail' returned by circ_queue_cur_write_pos.
 *
 * @param[in] q The circular q handle.
 * @param[in] amt The number of bytes we've filled in for the reader.
 * @returns 0 if the write succeeded
 */
int circ_queue_advance_write_multi(circ_queue_t q, uint16_t amt) {
	if (amt == 0) {
		return 0;
	}

	uint16_t orig_wr_head = q->write_head;

	uint16_t new_write_head = advance_by_n(q->num_elem, orig_wr_head,
			amt);

	/* Legal states at the end of this are---
	 * a "later place" in the buffer without wrapping.
	 * or the 0th position-- if we've consumed all to the end.
	 */

	PIOS_Assert((new_write_head > orig_wr_head) || (new_write_head == 0));

	/* the head is not allowed to advance to meet the tail */
	if (new_write_head == q->read_tail) {
		/* This is only sane if they're trying to return one, like
		 * advance_write does */
		PIOS_Assert(amt == 1);

		return -1;	/* Full */

		/* Caller can either let the data go away, or try again to
		 * advance later. */
	}

	q->write_head = new_write_head;

	return 0;
}

/** Makes the current block of data available to the reader and advances write pos.
 * This may fail if the queue contain num_elems -1 elements, in which case the
 * advance may be retried in the future.  In this case, data already written to
 * write_pos is preserved and the advance may be retried (or overwritten with
 * new data).
 *
 * @param[in] q Handle to circular queue.
 * @returns 0 if the write succeeded, nonzero on error.
 */
int circ_queue_advance_write(circ_queue_t q) {
	return circ_queue_advance_write_multi(q, 1);
}

/** Returns a block of data to the reader.
 * The block is "claimed" until released with circ_queue_read_completed.
 * No new data is available until that call is made (instead the same
 * block-in-progress will be returned).
 *
 * @param[in] q Handle to circular queue.
 * @param[out] contig Returns number of contig elements that can be
 * read at once.
 * @param[out] avail Returns number of elements available to read
 * without any further writer activity.
 * @returns pointer to the data, or NULL if the queue is empty.
 */
void *circ_queue_read_pos(circ_queue_t q, uint16_t *contig, uint16_t *avail) {
	uint16_t read_tail = q->read_tail;
	uint16_t wr_head = q->write_head;

	void *contents = q->contents;

	if (contig) {
		if (wr_head >= read_tail) {
			/* read_tail is allowed to advance to meet head,
			 * so no minus one here. */
			*contig = wr_head - read_tail;
		} else {
			/* Number of contiguous elements to end of the buf,
			 * otherwise. */
			*contig = q->num_elem - read_tail;
		}
	}

	if (avail) {
		if (wr_head >= read_tail) {
			/* Same as immediately above; no wrap avail */
			*avail = wr_head - read_tail;
		} else {
			/* Distance to end, plus distance from beginning
			 * to wr_head */
			*avail = q->num_elem - read_tail + wr_head;
		}
	}

	if (wr_head == read_tail) {
		/* There is nothing new to read.  */
		return NULL;
	}

	return contents + q->read_tail * q->elem_size;
}

/** Empties all elements from the queue. */
void circ_queue_clear(circ_queue_t q) {
	q->read_tail = q->write_head;
}

/** Releases an element of read data obtained by circ_queue_read_pos.
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
	 * to circ_queue_read_pos.
	 */
	PIOS_Assert(read_tail != q->write_head);

	q->read_tail = next_pos(q->num_elem, read_tail);
}

/** Releases multiple elements of read data obtained by circ_queue_read_pos.
 * Behavior is undefined if returning more than circ_queue_read_pos
 * previously signaled in contig.
 *
 * @param[in] q Handle to the circula queue.
 * @param[in] num Number of elements to release.
 */
void circ_queue_read_completed_multi(circ_queue_t q, uint16_t num) {
	if (num == 0) {
		return;
	}

	/* Avoid multiple accesses to a volatile */
	uint16_t orig_read_tail = q->read_tail;

	/* If this is being called, the queue had better not be empty--
	 * we're supposed to finish consuming this element after a prior call
	 * to circ_queue_read_pos.
	 */
	PIOS_Assert(orig_read_tail != q->write_head);

	uint16_t read_tail = advance_by_n(q->num_elem, orig_read_tail, num);

	/* Legal states at the end of this are---
	 * a "later place" in the buffer without wrapping.
	 * or the 0th position-- if we've consumed all to the end.
	 */

	PIOS_Assert((read_tail > orig_read_tail) || (read_tail == 0));

	q->read_tail = read_tail;
}

uint16_t circ_queue_write_data(circ_queue_t q, const void *buf, uint16_t num) {
	uint16_t total_put = 0;
	uint16_t put_this_time = 0;

	const void *buf_pos = buf;

	do {
		void *wr_pos = circ_queue_write_pos(q, &put_this_time, NULL);

		if (!put_this_time) break;

		if (put_this_time > (num - total_put)) {
			put_this_time = num - total_put;
		}

		uint32_t sz = put_this_time * q->elem_size;

		memcpy(wr_pos, buf_pos, sz);

		circ_queue_advance_write_multi(q, put_this_time);

		buf_pos += sz;

		total_put += put_this_time;
	} while (total_put < num);

	return total_put;
}

uint16_t circ_queue_read_data(circ_queue_t q, void *buf, uint16_t num) {
	uint16_t total_read = 0;
	uint16_t read_this_time = 0;

	void *buf_pos = buf;

	do {
		void *rd_pos = circ_queue_read_pos(q, &read_this_time, NULL);

		if (!read_this_time) break;

		if (read_this_time > (num - total_read)) {
			read_this_time = num - total_read;
		}

		uint32_t sz = read_this_time * q->elem_size;

		memcpy(buf_pos, rd_pos, sz);

		circ_queue_read_completed_multi(q, read_this_time);

		buf_pos += sz;

		total_read += read_this_time;
	} while (total_read < num);

	return total_read;
}

