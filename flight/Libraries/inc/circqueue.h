/**
 ******************************************************************************
 * @file       circqueue.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @brief Public header for 1 reader, 1 writer circular queue
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

#ifndef _CIRCQUEUE_H
#define _CIRCQUEUE_H

#include <pios.h>
#include <stdint.h>

typedef struct circ_queue *circ_queue_t;

circ_queue_t circ_queue_new(uint16_t elem_size, uint16_t num_elem);

void *circ_queue_write_pos(circ_queue_t q, uint16_t *contig,
		uint16_t *avail);

int circ_queue_advance_write_multi(circ_queue_t q, uint16_t amt);

int circ_queue_advance_write(circ_queue_t q);

void *circ_queue_read_pos(circ_queue_t q, uint16_t *contig,
		uint16_t *avail);

void circ_queue_read_completed(circ_queue_t q);

void circ_queue_read_completed_multi(circ_queue_t q, uint16_t num);

uint16_t circ_queue_read_data(circ_queue_t q, void *buf, uint16_t num);

uint16_t circ_queue_write_data(circ_queue_t q, const void *buf, uint16_t num);

void circ_queue_clear(circ_queue_t q);
#endif
