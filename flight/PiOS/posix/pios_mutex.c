/**
 ******************************************************************************
 * @file       pios_mutex.c
 * @author     dRonin, http://dRonin.org, Copyright (C) 2017
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_Mutex Mutex Abstraction
 * @{
 * @brief Abstracts the concept of a mutex to hide different implementations
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
#include <pios_mutex.h>

struct pios_mutex {
	pthread_mutex_t mutex;
};

struct pios_recursive_mutex {
	/* Must consist of only this */
	struct pios_mutex mutex;
};

struct pios_mutex *PIOS_Mutex_Create(void)
{
	pthread_mutexattr_t attr;

	if (pthread_mutexattr_init(&attr)) {
		abort();
	}

	pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

	struct pios_mutex *p = malloc(sizeof(*p));

	if (p) {
		if (pthread_mutex_init(&p->mutex, &attr)) {
			free(p);

			return NULL;
		}
	}

	return p;
}

bool PIOS_Mutex_Lock(struct pios_mutex *mtx, uint32_t timeout_ms)
{
	int ret;

	if (timeout_ms >= PIOS_MUTEX_TIMEOUT_MAX) {
		ret = pthread_mutex_lock(&mtx->mutex);
	} else {
		struct timespec abstime;

		clock_gettime(CLOCK_REALTIME, &abstime);

		abstime.tv_nsec += (timeout_ms % 1000) * 1000000;
		abstime.tv_sec += timeout_ms / 1000;

		if (abstime.tv_nsec > 1000000000) {
			abstime.tv_nsec -= 1000000000;
			abstime.tv_sec += 1;
		}

		ret = pthread_mutex_timedlock(&mtx->mutex, &abstime);
	}

	return (ret == 0);
}

bool PIOS_Mutex_Unlock(struct pios_mutex *mtx)
{
	int ret;

	ret = pthread_mutex_unlock(&mtx->mutex);

	PIOS_Assert(!ret);

	return true;
}

struct pios_recursive_mutex *PIOS_Recursive_Mutex_Create(void)
{
	pthread_mutexattr_t attr;

	if (pthread_mutexattr_init(&attr)) {
		abort();
	}

	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

	struct pios_mutex *p = malloc(sizeof(*p));

	if (p) {
		if (pthread_mutex_init(&p->mutex, &attr)) {
			free(p);

			return NULL;
		}
	}

	return (struct pios_recursive_mutex *) p;
}

bool PIOS_Recursive_Mutex_Lock(struct pios_recursive_mutex *mtx, uint32_t timeout_ms) 
{
	return PIOS_Mutex_Lock(&mtx->mutex, timeout_ms);
}

bool PIOS_Recursive_Mutex_Unlock(struct pios_recursive_mutex *mtx)
{
	return PIOS_Mutex_Unlock(&mtx->mutex);
}

/**
 * @}
 * @}
 */
