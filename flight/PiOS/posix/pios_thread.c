/**
 ******************************************************************************
 * @file       pios_thread.c
 * @author     dRonin, http://dRonin.org, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_Thread Thread Abstraction
 * @{
 * @brief Abstracts the concept of a thread to hide different implementations
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
#include <unistd.h>

#include <pios.h>
#include <pios_thread.h>

struct pios_thread
{
	pthread_t thread;

	char *name;
};

struct pios_thread *PIOS_Thread_Create(void (*fp)(void *), const char *namep, size_t stack_bytes, void *argp, enum pios_thread_prio_e prio)
{
	struct pios_thread *thread = malloc(sizeof(thread));

	pthread_attr_t attr;

	if (pthread_attr_init(&attr)) {
		perror("pthread_attr_init");
		abort();
	}

	extern bool are_realtime;

	if (are_realtime) {
		struct sched_param param = {
			.sched_priority = 30 + prio * 5
		};

		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		pthread_attr_setschedparam(&attr, &param);
	}

	thread->name = strdup(namep);

	void *(*thr_func)(void *) = (void *) fp;

	int ret = pthread_create(&thread->thread, &attr, thr_func, argp);

	if (ret) {
		printf("Couldn't start thr (%s) ret=%d\n", namep, ret);

		free(thread->name);
		free(thread);
		return NULL;
	}

#ifdef __linux__
	pthread_setname_np(thread->thread, thread->name);
#endif

	printf("Started thread (%s) p=%p\n", namep, &thread->thread);

	return thread;
}

void PIOS_Thread_Delete(struct pios_thread *threadp)
{
	if (threadp != NULL) {
		abort();	// Only support this on "self"
	}

#if 0
	/* Need to figure out our own thread structure to clean it up */
	free(threadp->name);
	free(threadp);
#endif

	pthread_exit(0);
}

uint32_t PIOS_Thread_Systime(void)
{
	struct timespec monotime;

	clock_gettime(CLOCK_MONOTONIC, &monotime);

	uint32_t t = monotime.tv_sec * 1000 + monotime.tv_nsec / 1000000;

	static uint32_t base = 0;

	if (!base) {
		base = t;
	}

	return t - base;
}

void PIOS_Thread_Sleep(uint32_t time_ms)
{
	usleep(1000 * time_ms);
}

void PIOS_Thread_Sleep_Until(uint32_t *previous_ms, uint32_t increment_ms)
{
	*previous_ms += increment_ms;

	uint32_t now = PIOS_Thread_Systime();

	uint32_t ms = *previous_ms - now;

	if (ms > increment_ms) {
		// Very late or wrapped.
		*previous_ms = now;
	} else if (ms > 0) {
		PIOS_Thread_Sleep(ms);
	}
}

uint32_t PIOS_Thread_Get_Stack_Usage(struct pios_thread *threadp)
{
	return 0;	/* XXX */
}

uint32_t PIOS_Thread_Get_Runtime(struct pios_thread *threadp)
{
	return 0;	/* XXX */
}

/**
  * @}
  * @}
  */
