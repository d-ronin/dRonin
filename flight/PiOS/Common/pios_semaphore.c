/**
 ******************************************************************************
 * @file       pios_semaphore.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
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

#include "pios.h"
#include "pios_semaphore.h"

struct pios_semaphore
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	BinarySemaphore sema;
#else
	uint32_t sema_count;
#endif
};

#if defined(PIOS_INCLUDE_CHIBIOS)

/**
 *
 * @brief   Creates a binary semaphore.
 *
 * @returns instance of @p struct pios_semaphore or NULL on failure
 *
 */
struct pios_semaphore *PIOS_Semaphore_Create(void)
{
	struct pios_semaphore *sema = PIOS_malloc(sizeof(struct pios_semaphore));

	if (sema == NULL)
		return NULL;

	/*
	 * The initial state of a binary semaphore is "given".
	 */
	chBSemInit(&sema->sema, false);

	return sema;
}

/**
 *
 * @brief   Takes binary semaphore.
 *
 * @param[in] sema         pointer to instance of @p struct pios_semaphore
 * @param[in] timeout_ms   timeout for acquiring the lock in milliseconds
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Semaphore_Take(struct pios_semaphore *sema, uint32_t timeout_ms)
{
	PIOS_Assert(sema != NULL);

	if (timeout_ms == PIOS_SEMAPHORE_TIMEOUT_MAX)
		return chBSemWait(&sema->sema) == RDY_OK;
	else if (timeout_ms == 0)
		return chBSemWaitTimeout(&sema->sema, TIME_IMMEDIATE) == RDY_OK;
	else
		return chBSemWaitTimeout(&sema->sema, MS2ST(timeout_ms)) == RDY_OK;
}

/**
 *
 * @brief   Gives binary semaphore.
 *
 * @param[in] sema         pointer to instance of @p struct pios_semaphore
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Semaphore_Give(struct pios_semaphore *sema)
{
	PIOS_Assert(sema != NULL);

	chBSemSignal(&sema->sema);

	return true;
}

/**
 *
 * @brief   Takes binary semaphore from ISR context.
 *
 * @param[in] sema         pointer to instance of @p struct pios_semaphore
 * @param[out] woken       pointer to bool which will be set true if a context switch is required
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Semaphore_Take_FromISR(struct pios_semaphore *sema, bool *woken)
{
	/* Waiting on a semaphore within an interrupt is not supported by ChibiOS. */
	PIOS_Assert(false);
	return false;
}

/**
 *
 * @brief   Gives binary semaphore from ISR context.
 *
 * @param[in] sema         pointer to instance of @p struct pios_semaphore
 * @param[out] woken       pointer to bool which will be set true if a context switch is required
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Semaphore_Give_FromISR(struct pios_semaphore *sema, bool *woken)
{
	PIOS_Assert(sema != NULL);
	PIOS_Assert(woken != NULL);

	chSysLockFromIsr();
	chBSemSignalI(&sema->sema);
	chSysUnlockFromIsr();

	return true;
}

#else /* No RTOS */

/**
 *
 * @brief   Creates a binary semaphore.
 *
 * @returns instance of @p struct pios_semaphore or NULL on failure
 *
 */
struct pios_semaphore *PIOS_Semaphore_Create(void)
{
	struct pios_semaphore *sema = PIOS_malloc_no_dma(sizeof(struct pios_semaphore));

	if (sema == NULL)
		return NULL;

	/*
	 * The initial state of a binary semaphore is "given".
	 */
	sema->sema_count = 1;

	return sema;
}

/**
 *
 * @brief   Takes binary semaphore.
 *
 * @param[in] sema         pointer to instance of @p struct pios_semaphore
 * @param[in] timeout_ms   timeout for acquiring the lock in milliseconds
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Semaphore_Take(struct pios_semaphore *sema, uint32_t timeout_ms)
{
	PIOS_Assert(sema != NULL);

	uint32_t start = PIOS_DELAY_GetRaw();

	uint32_t temp_sema_count;
	do {
		PIOS_IRQ_Disable();
		if ((temp_sema_count = sema->sema_count) != 0)
			--sema->sema_count;
		PIOS_IRQ_Enable();
	} while (temp_sema_count == 0 &&
		PIOS_DELAY_DiffuS(start) < timeout_ms * 1000);

	return temp_sema_count != 0;
}

/**
 *
 * @brief   Gives binary semaphore.
 *
 * @param[in] sema         pointer to instance of @p struct pios_semaphore
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Semaphore_Give(struct pios_semaphore *sema)
{
	PIOS_Assert(sema != NULL);

	bool result = true;

	PIOS_IRQ_Disable();

	if (sema->sema_count == 0)
		++sema->sema_count;
	else
		result = false;

	PIOS_IRQ_Enable();

	return result;
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

#endif /* no-rtos */
