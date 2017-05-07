/**
 ******************************************************************************
 * @file       pios_queue.h
 * @author     dRonin, http://dronin.org Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_Queue Queue Abstraction
 * @{
 * @brief Abstracts the concept of a queue to hide different implementations
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

#include <pthread.h>
#include <stdlib.h>

#include <circqueue.h>

#include <pios_queue.h>
#include <pios_thread.h>

struct pios_queue {
#define QUEUE_MAGIC 75657551	/* 'Queu' */
	uint32_t magic;

	pthread_mutex_t mutex;
	pthread_cond_t cond;

	uint16_t item_size;
	uint16_t q_len;

	circ_queue_t queue;
};

struct pios_queue *PIOS_Queue_Create(size_t queue_length, size_t item_size)
{
	struct pios_queue *q = PIOS_malloc(sizeof(*q));

	if (!q) {
		return NULL;
	}

        pthread_mutexattr_t attr;

        if (pthread_mutexattr_init(&attr)) {
                abort();
        }

        pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

	if (pthread_mutex_init(&q->mutex, &attr)) {
		abort();
	}

	if (pthread_cond_init(&q->cond, NULL)) {
		abort();
	}

	q->item_size = item_size;
	q->q_len = queue_length;

	q->queue = circ_queue_new(item_size, queue_length+1);

	if (!q->queue) {
		pthread_mutex_destroy(&q->mutex);
		pthread_cond_destroy(&q->cond);

		PIOS_free(q);
		return NULL;
	}

	q->magic = QUEUE_MAGIC;

	return q;
}

void PIOS_Queue_Delete(struct pios_queue *queuep)
{
	PIOS_Assert(queuep->magic == QUEUE_MAGIC);

	pthread_mutex_destroy(&queuep->mutex);
	pthread_cond_destroy(&queuep->cond);

	/* XXX free up the queue buf */

	free(queuep);
}

bool PIOS_Queue_Send(struct pios_queue *queuep,
		const void *itemp, uint32_t timeout_ms)
{
	PIOS_Assert(queuep->magic == QUEUE_MAGIC);

	struct timespec abstime;

	if (timeout_ms != PIOS_QUEUE_TIMEOUT_MAX) {
		clock_gettime(CLOCK_REALTIME, &abstime);

		abstime.tv_nsec += (timeout_ms % 1000) * 1000000;
		abstime.tv_sec += timeout_ms / 1000;

		if (abstime.tv_nsec > 1000000000) {
			abstime.tv_nsec -= 1000000000;
			abstime.tv_sec += 1;
		}
	}

	pthread_mutex_lock(&queuep->mutex);

	while (!circ_queue_write_data(queuep->queue, itemp, 1)) {
		if (timeout_ms != PIOS_QUEUE_TIMEOUT_MAX) {
			if (pthread_cond_timedwait(&queuep->cond,
					&queuep->mutex, &abstime)) {
				pthread_mutex_unlock(&queuep->mutex);
				return false;
			}
		} else {
			pthread_cond_wait(&queuep->cond, &queuep->mutex);
		}
	}

	pthread_cond_broadcast(&queuep->cond);

	pthread_mutex_unlock(&queuep->mutex);

	return true;
}

bool PIOS_Queue_Send_FromISR(struct pios_queue *queuep,
		const void *itemp, bool *wokenp)
{
	bool ret = PIOS_Queue_Send(queuep, itemp, 0);

	if (ret && (wokenp)) {
		*wokenp = true;
	}

	return ret;
}

bool PIOS_Queue_Receive(struct pios_queue *queuep,
		void *itemp, uint32_t timeout_ms)
{
	PIOS_Assert(queuep->magic == QUEUE_MAGIC);

	struct timespec abstime;

	if (timeout_ms != PIOS_QUEUE_TIMEOUT_MAX) {
		clock_gettime(CLOCK_REALTIME, &abstime);

		abstime.tv_nsec += (timeout_ms % 1000) * 1000000;
		abstime.tv_sec += timeout_ms / 1000;

		if (abstime.tv_nsec > 1000000000) {
			abstime.tv_nsec -= 1000000000;
			abstime.tv_sec += 1;
		}
	}

	pthread_mutex_lock(&queuep->mutex);

	while (!circ_queue_read_data(queuep->queue, itemp, 1)) {
		if (timeout_ms != PIOS_QUEUE_TIMEOUT_MAX) {
			if (pthread_cond_timedwait(&queuep->cond,
					&queuep->mutex, &abstime)) {
				pthread_mutex_unlock(&queuep->mutex);
				return false;
			}
		} else {
			pthread_cond_wait(&queuep->cond, &queuep->mutex);
		}
	}

	pthread_cond_broadcast(&queuep->cond);

	pthread_mutex_unlock(&queuep->mutex);

	return true;
}

size_t PIOS_Queue_GetItemSize(struct pios_queue *queuep)
{
	PIOS_Assert(queuep);
	return queuep->item_size;
}

/**
  * @}
  * @}
  */
