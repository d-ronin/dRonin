/**
 ******************************************************************************
 * @file       circqueue.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _CIRCQUEUE_H
#define _CIRCQUEUE_H

#include <pios.h>
#include <stdint.h>

typedef struct circ_queue *circ_queue_t;

circ_queue_t circ_queue_new(uint16_t elem_size, uint16_t num_elem);

void *circ_queue_cur_write_pos(circ_queue_t q);

int circ_queue_advance_write(circ_queue_t q);

void *circ_queue_read_pos(circ_queue_t q);

void circ_queue_read_completed(circ_queue_t q);

#endif
