/**
 ******************************************************************************
 * @file       pios_thread.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
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

#include "pios.h"
#include "pios_thread.h"

#if !defined(PIOS_INCLUDE_CHIBIOS)
#error "pios_thread.c requires PIOS_INCLUDE_CHIBIOS"
#endif

#if !defined(CH_CFG_USE_MEMCORE) || CH_CFG_USE_MEMCORE != TRUE
#error "pios_thread: Need to use memcore with ChibiOS."
#endif

#if defined(PIOS_INCLUDE_CHIBIOS)
struct pios_thread
{
	thread_t *threadp;
	uint32_t *stack;
};

#define CVT_MS2ST(msec) ((systime_t)(((((uint32_t)(msec)) * ((uint64_t)CH_CFG_ST_FREQUENCY) - 1UL) / 1000UL) + 1UL))
#define CVT_ST2MS(n) (((((n) - 1ULL) * 1000ULL) / ((uint64_t)CH_CFG_ST_FREQUENCY)) + 1UL)

/**
 * Compute size that is at rounded up to the nearest
 * multiple of 8
 */ 
static uint32_t ceil_size(uint32_t size)
{
	const uint32_t a = sizeof(stkalign_t);
	size = size + (a - size % a);
	return size;
}

/**
 * @brief   Creates a handle for the current thread.
 *
 * @param[in] namep        pointer to thread name
 *
 * @returns instance of @p struct pios_thread or NULL on failure
 */
struct pios_thread *PIOS_Thread_WrapCurrentThread(const char *namep)
{
	struct pios_thread *thread = PIOS_malloc_no_dma(sizeof(struct pios_thread));

	if (thread) {
		thread->threadp = chThdGetSelfX();

		extern uint32_t __main_thread_stack_base__;
		thread->stack = &__main_thread_stack_base__;
#if CH_CFG_USE_REGISTRY
		thread->threadp->name = namep;
#endif /* CH_USE_REGISTRY */
	}

	return thread;
}

/**
 * @brief Changes the priority of this thread.
 *
 * @param[in] prio The new priority.
 */
void PIOS_Thread_ChangePriority(enum pios_thread_prio_e prio)
{
	chThdSetPriority(prio);
}

/**
 *
 * @brief   Creates a thread.
 *
 * @param[in] fp           pointer to thread function
 * @param[in] namep        pointer to thread name
 * @param[in] stack_bytes  stack size in bytes
 * @param[in] argp         pointer to argument which will be passed to thread function
 * @param[in] prio         thread priority
 *
 * @returns instance of @p struct pios_thread or NULL on failure
 *
 */
struct pios_thread *PIOS_Thread_Create(void (*fp)(void *), const char *namep, size_t stack_bytes, void *argp, enum pios_thread_prio_e prio)
{
	struct pios_thread *thread = PIOS_malloc_no_dma(sizeof(struct pios_thread));
	if (thread == NULL)
		return NULL;

	// Use special functions to ensure ChibiOS stack requirements
	stack_bytes = ceil_size(stack_bytes);
	uint8_t *wap = chCoreAllocAligned(stack_bytes, PORT_STACK_ALIGN);
	if (wap == NULL)
	{
		PIOS_free(thread);
		return NULL;
	}

	thread->threadp = chThdCreateStatic(wap, stack_bytes, prio, (tfunc_t)fp, argp);
	if (thread->threadp == NULL)
	{
		PIOS_free(thread);
		PIOS_free(wap);
		return NULL;
	}

#if CH_CFG_USE_REGISTRY
	thread->threadp->name = namep;
#endif /* CH_CFG_USE_REGISTRY */

	/* Newer ChibiOS versions store the thread struct at the bottom of the stack allocation
	   instead of the top. */
	thread->stack = (uint32_t*)wap;

	return thread;
}

#if (CH_CFG_USE_WAITEXIT == TRUE)
/**
 *
 * @brief   Destroys an instance of @p struct pios_thread.
 *
 * @param[in] threadp      pointer to instance of @p struct pios_thread
 *
 */
void PIOS_Thread_Delete(struct pios_thread *threadp)
{
	if (threadp == NULL)
	{
		chThdExit(0);
	}
	else
	{
		chThdTerminate(threadp->threadp);
		chThdWait(threadp->threadp);
	}
}
#else
#error "PIOS_Thread_Delete requires CH_USE_WAITEXIT to be defined TRUE"
#endif /* (CH_CFG_USE_WAITEXIT == TRUE) */

/**
 *
 * @brief   Returns the current system time.
 *
 * @returns current system time
 *
 */
uint32_t PIOS_Thread_Systime(void)
{
	return (uint32_t)CVT_ST2MS(chVTGetSystemTime());
}

/**
 *
 * @brief   Suspends execution of current thread at least for specified time.
 *
 * @param[in] time_ms      time in milliseconds to suspend thread execution
 *
 */
void PIOS_Thread_Sleep(uint32_t time_ms)
{
	if (time_ms == PIOS_THREAD_TIMEOUT_MAX)
		chThdSleep(TIME_INFINITE);
	else
		chThdSleep(MS2ST(time_ms));
}

/**
 *
 * @brief   Suspends execution of current thread for a regular interval.
 *
 * @param[in] previous_ms  pointer to system time of last execution,
 *                         must have been initialized with PIOS_Thread_Systime() on first invocation
 * @param[in] increment_ms time of regular interval in milliseconds
 *
 */
void PIOS_Thread_Sleep_Until(uint32_t *previous_ms, uint32_t increment_ms)
{
	// Do the math in the millisecond domain.
	*previous_ms += increment_ms;

	systime_t future = CVT_MS2ST(*previous_ms);
	systime_t increment_st = CVT_MS2ST(increment_ms);

	chSysLock();

	systime_t now = chVTGetSystemTime();
	systime_t sleep_time = future - now;

	if (sleep_time > increment_st) {
		// OK, the calculated sleep time is much longer than
		// the desired interval.  There's two possibilities:
		// 1) We were already late!  If so just don't sleep.
		// 2) The timer wrapped. (at 49 days)  Just don't sleep.

		if ((now - future) > increment_st) {
			// However, in this particular case we need to fix up
			// the clock.  If we're very late, OR WRAPPED,
			// don't try and keep on the previous timebase.
			*previous_ms = CVT_ST2MS(now);
		}
	} else if (sleep_time > 0) {
		// Be sure not to request a sleep_time of 0, as that's
		// IMMEDIATE and makes chTdSleepS panic.
		chThdSleepS(sleep_time);
	}

	chSysUnlock();
}

bool PIOS_Thread_Period_Elapsed(const uint32_t prev_systime, const uint32_t increment_ms)
{
	/* TODO: make PIOS_Thread_Systime return opaque type to avoid ms conversion */
	return CVT_MS2ST(increment_ms) <= chVTTimeElapsedSinceX(CVT_MS2ST(prev_systime));
}

/**
 *
 * @brief   Returns stack usage of a thread.
 *
 * @param[in] threadp      pointer to instance of @p struct pios_thread
 *
 * @return stack usage in bytes
 */
uint32_t PIOS_Thread_Get_Stack_Usage(struct pios_thread *threadp)
{
#if CH_DBG_FILL_THREADS
	uint32_t *stack = threadp->stack;
	uint32_t *stklimit = stack;
	while (*stack ==
			((CH_DBG_STACK_FILL_VALUE << 24) |
			(CH_DBG_STACK_FILL_VALUE << 16) |
			(CH_DBG_STACK_FILL_VALUE << 8) |
			(CH_DBG_STACK_FILL_VALUE << 0)))
		++stack;
	return (stack - stklimit) * 4;
#else
	return 0;
#endif /* CH_DBG_FILL_THREADS */
}

/**
 *
 * @brief   Returns runtime of a thread.
 *
 * @param[in] threadp      pointer to instance of @p struct pios_thread
 *
 * @return runtime in milliseconds
 *
 */
 uint32_t PIOS_Thread_Get_Runtime(struct pios_thread *threadp)
{
	chSysLock();

	uint32_t result = threadp->threadp->ticks_total;
	threadp->threadp->ticks_total = 0;

	chSysUnlock();

	return result;
}

/**
 *
 * @brief   Suspends execution of all threads.
 *
 */
void PIOS_Thread_Scheduler_Suspend(void)
{
	chSysLock();
}

/**
 *
 * @brief   Resumes execution of all threads.
 *
 */
void PIOS_Thread_Scheduler_Resume(void)
{
	chSysUnlock();
}

#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
