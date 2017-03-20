/**
 ******************************************************************************
 * @file       pios_semaphore.h
 * @author     dRonin, http://dRonin.org, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_Semaphore Semaphore Abstraction
 * @{
 * @brief Abstracts the concept of a binary semaphore to hide different implementations
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

#include <pios.h>
#include <pios_semaphore.h>

struct pios_semaphore {
#define SEMAPHORE_MAGIC 0x616d6553	/* 'Sema' */
	uint32_t magic;

	pthread_mutex_t mutex;
	pthread_cond_t cond;

	bool given;
};

struct pios_semaphore *PIOS_Semaphore_Create(void)
{
	struct pios_semaphore *s = PIOS_malloc(sizeof(*s));

	if (!s) {
		return NULL;
	}

	pthread_mutexattr_t attr;

	if (pthread_mutexattr_init(&attr)) {
		abort();
	}

	pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

	if (pthread_mutex_init(&s->mutex, &attr)) {
		abort();
	}

	if (pthread_cond_init(&s->cond, NULL)) {
		abort();
	}

	s->given = false;

	s->magic = SEMAPHORE_MAGIC;

	return s;
}

bool PIOS_Semaphore_Take(struct pios_semaphore *sema, uint32_t timeout_ms)
{
	PIOS_Assert(sema->magic == SEMAPHORE_MAGIC);

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

        pthread_mutex_lock(&sema->mutex);

        while (!sema->given) {
                if (timeout_ms != PIOS_QUEUE_TIMEOUT_MAX) {
                        if (pthread_cond_timedwait(&sema->cond,
                                        &sema->mutex, &abstime)) {
                                pthread_mutex_unlock(&sema->mutex);
                                return false;
                        }
                } else {
                        pthread_cond_wait(&sema->cond, &sema->mutex);
                }
        }

	sema->given = false;

	pthread_mutex_unlock(&sema->mutex);

	return true;
}

bool PIOS_Semaphore_Give(struct pios_semaphore *sema)
{
	bool old;

	PIOS_Assert(sema->magic == SEMAPHORE_MAGIC);

	pthread_mutex_lock(&sema->mutex);

	old = sema->given;

	sema->given = true;

	pthread_cond_signal(&sema->cond);
	
	pthread_mutex_unlock(&sema->mutex);

	return !old;
}

bool PIOS_Semaphore_Take_FromISR(struct pios_semaphore *sema, bool *woken)
{
	bool ret = PIOS_Semaphore_Take(sema, 0);

	if (ret && woken) {
		*woken = true;
	}

	return ret;
}

bool PIOS_Semaphore_Give_FromISR(struct pios_semaphore *sema, bool *woken)
{
	bool ret = PIOS_Semaphore_Give(sema);

	if (ret && woken) {
		*woken = true;
	}

	return ret;
}

/**
  * @}
  * @}
  */
