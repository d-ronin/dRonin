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
