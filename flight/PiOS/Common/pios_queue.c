/**
 ******************************************************************************
 * @file       pios_queue.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
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

#include "pios.h"
#include "pios_queue.h"

#if !defined(PIOS_INCLUDE_CHIBIOS)
#error "pios_queue.c requires PIOS_INCLUDE_CHIBIOS"
#endif

#if defined(PIOS_INCLUDE_CHIBIOS)

#include "ch.h"

struct pios_queue
{
	mailbox_t mb;
	memory_pool_t mp;
	void *mpb;
};

#if !defined(PIOS_QUEUE_MAX_WAITERS)
#define PIOS_QUEUE_MAX_WAITERS 2
#endif /* !defined(PIOS_QUEUE_MAX_WAITERS) */

/**
 *
 * @brief   Creates a queue.
 *
 * @returns instance of @p struct pios_queue or NULL on failure
 *
 */
struct pios_queue *PIOS_Queue_Create(size_t queue_length, size_t item_size)
{
	struct pios_queue *queuep = PIOS_malloc_no_dma(sizeof(struct pios_queue));
	if (queuep == NULL)
		return NULL;

	/* Create the memory pool. */
	queuep->mpb = PIOS_malloc_no_dma(item_size * (queue_length + PIOS_QUEUE_MAX_WAITERS));
	if (queuep->mpb == NULL) {
		PIOS_free(queuep);
		return NULL;
	}
	chPoolObjectInit(&queuep->mp, item_size, NULL);
	chPoolLoadArray(&queuep->mp, queuep->mpb, queue_length + PIOS_QUEUE_MAX_WAITERS);

	/* Create the mailbox. */
	msg_t *mb_buf = PIOS_malloc_no_dma(sizeof(msg_t) * queue_length);
	chMBObjectInit(&queuep->mb, mb_buf, queue_length);

	return queuep;
}

/**
 *
 * @brief   Destroys an instance of @p struct pios_queue
 *
 * @param[in] queuep       pointer to instance of @p struct pios_queue
 *
 */
void PIOS_Queue_Delete(struct pios_queue *queuep)
{
	PIOS_free(queuep->mpb);
	PIOS_free(queuep);
}

/**
 *
 * @brief   Appends an item to a queue.
 *
 * @param[in] queuep       pointer to instance of @p struct pios_queue
 * @param[in] itemp        pointer to item which will be appended to the queue
 * @param[in] timeout_ms   timeout for appending item to queue in milliseconds
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Queue_Send(struct pios_queue *queuep, const void *itemp, uint32_t timeout_ms)
{
	void *buf = chPoolAlloc(&queuep->mp);
	if (buf == NULL)
		return false;

	memcpy(buf, itemp, queuep->mp.object_size);

	systime_t timeout;
	if (timeout_ms == PIOS_QUEUE_TIMEOUT_MAX)
		timeout = TIME_INFINITE;
	else if (timeout_ms == 0)
		timeout = TIME_IMMEDIATE;
	else
		timeout = MS2ST(timeout_ms);

	msg_t result = chMBPost(&queuep->mb, (msg_t)buf, timeout);

	if (result != MSG_OK)
	{
		chPoolFree(&queuep->mp, buf);
		return false;
	}

	return true;
}

/**
 *
 * @brief   Appends an item to a queue from ISR context.
 *
 * @param[in] queuep       pointer to instance of @p struct pios_queue
 * @param[in] itemp        pointer to item which will be appended to the queue
 * @param[in] timeout_ms   timeout for appending item to queue in milliseconds
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Queue_Send_FromISR(struct pios_queue *queuep, const void *itemp, bool *wokenp)
{
	chSysLockFromISR();
	void *buf = chPoolAllocI(&queuep->mp);
	if (buf == NULL)
	{
		chSysUnlockFromISR();
		return false;
	}

	memcpy(buf, itemp, queuep->mp.object_size);

	msg_t result = chMBPostI(&queuep->mb, (msg_t)buf);

	if (result != MSG_OK)
	{
		chPoolFreeI(&queuep->mp, buf);
		chSysUnlockFromISR();
		return false;
	}

	chSysUnlockFromISR();

	return true;
}

/**
 *
 * @brief   Retrieves an item from the front of a queue.
 *
 * @param[in] queuep       pointer to instance of @p struct pios_queue
 * @param[in] itemp        pointer to item which will be retrieved
 * @param[in] timeout_ms   timeout for retrieving item from queue in milliseconds
 *
 * @returns true on success or false on timeout or failure
 *
 */
bool PIOS_Queue_Receive(struct pios_queue *queuep, void *itemp, uint32_t timeout_ms)
{
	msg_t buf;

	systime_t timeout;
	if (timeout_ms == PIOS_QUEUE_TIMEOUT_MAX)
		timeout = TIME_INFINITE;
	else if (timeout_ms == 0)
		timeout = TIME_IMMEDIATE;
	else
		timeout = MS2ST(timeout_ms);

	msg_t result = chMBFetch(&queuep->mb, &buf, timeout);

	if (result != MSG_OK)
		return false;

	memcpy(itemp, (void*)buf, queuep->mp.object_size);

	chPoolFree(&queuep->mp, (void*)buf);

	return true;
}

/*
 * @brief Gets the item size of a queue.
 *
 * @param[in] queuep Pointer to instance of @p struct pios_queue
 *
 * @returns Size of item
 *
 */
size_t PIOS_Queue_GetItemSize(struct pios_queue *queuep)
{
	PIOS_Assert(queuep);
	return queuep->mp.object_size;
}

#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
