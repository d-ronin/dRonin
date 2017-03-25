/**
 ******************************************************************************
 * @file       pios_mutex.c
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

#include "pios.h"
#include "pios_mutex.h"

#if !defined(PIOS_INCLUDE_RTOS)
#error "pios_mutex.c requires an RTOS"
#endif

#if defined(PIOS_INCLUDE_CHIBIOS)
struct pios_mutex
{
	Mutex mtx;
};

struct pios_recursive_mutex
{
	Mutex mtx;
	uint32_t count;
};

/**
 *
 * @brief   Creates a non recursive mutex.
 *
 * @returns instance of @p struct pios_mutex or NULL on failure
 *
 */
struct pios_mutex *PIOS_Mutex_Create(void)
{
	struct pios_mutex *mtx = PIOS_malloc_no_dma(sizeof(struct pios_mutex));

	if (mtx == NULL)
		return NULL;

	chMtxInit(&mtx->mtx);

	return mtx;
}

/**
 *
 * @brief   Locks a non recursive mutex.
 *
 * @param[in] mtx          pointer to instance of @p struct pios_mutex
 * @param[in] timeout_ms   timeout for acquiring the lock in milliseconds
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Mutex_Lock(struct pios_mutex *mtx, uint32_t timeout_ms)
{
	PIOS_Assert(mtx != NULL);

	chMtxLock(&mtx->mtx);

	return true;
}

/**
 *
 * @brief   Unlocks a non recursive mutex.
 *
 * @param[in] mtx          pointer to instance of @p struct pios_mutex
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Mutex_Unlock(struct pios_mutex *mtx)
{
	PIOS_Assert(mtx != NULL);

	chMtxUnlock();

	return true;
}

/**
 *
 * @brief   Creates a recursive mutex.
 *
 * @returns instance of @p struct pios_recursive mutex or NULL on failure
 *
 */
struct pios_recursive_mutex *PIOS_Recursive_Mutex_Create(void)
{
	struct pios_recursive_mutex *mtx = PIOS_malloc_no_dma(sizeof(struct pios_recursive_mutex));

	if (mtx == NULL)
		return NULL;

	chMtxInit(&mtx->mtx);
	mtx->count = 0;

	return mtx;
}

/**
 *
 * @brief   Locks a recursive mutex.
 *
 * @param[in] mtx          pointer to instance of @p struct pios_recursive_mutex
 * @param[in] timeout_ms   timeout for acquiring the lock in milliseconds
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Recursive_Mutex_Lock(struct pios_recursive_mutex *mtx, uint32_t timeout_ms)
{
	PIOS_Assert(mtx != NULL);

	chSysLock();

	if (chThdSelf() != mtx->mtx.m_owner)
		chMtxLockS(&mtx->mtx);

	++mtx->count;

	chSysUnlock();

	return true;
}

/**
 *
 * @brief   Unlocks a non recursive mutex.
 *
 * @param[in] mtx          pointer to instance of @p struct pios_mutex
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Recursive_Mutex_Unlock(struct pios_recursive_mutex *mtx)
{
	PIOS_Assert(mtx != NULL);

	chSysLock();

	--mtx->count;

	if (mtx->count == 0)
		chMtxUnlockS();

	chSysUnlock();

	return true;
}

#endif

