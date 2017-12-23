/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    chmboxes.h
 * @brief   Mailboxes macros and structures.
 *
 * @addtogroup mailboxes
 * @{
 */

#ifndef CHMBOXES_H
#define CHMBOXES_H

#if (CH_CFG_USE_MAILBOXES == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Structure representing a mailbox object.
 */
typedef struct {
  msg_t                 *buffer;        /**< @brief Pointer to the mailbox
                                                    buffer.                 */
  msg_t                 *top;           /**< @brief Pointer to the location
                                                    after the buffer.       */
  msg_t                 *wrptr;         /**< @brief Write pointer.          */
  msg_t                 *rdptr;         /**< @brief Read pointer.           */
  cnt_t                 cnt;            /**< @brief Messages in queue.      */
  bool                  reset;          /**< @brief True in reset state.    */
  threads_queue_t       qw;             /**< @brief Queued writers.         */
  threads_queue_t       qr;             /**< @brief Queued readers.         */
} mailbox_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Data part of a static mailbox initializer.
 * @details This macro should be used when statically initializing a
 *          mailbox that is part of a bigger structure.
 *
 * @param[in] name      the name of the mailbox variable
 * @param[in] buffer    pointer to the mailbox buffer array of @p msg_t
 * @param[in] size      number of @p msg_t elements in the buffer array
 */
#define _MAILBOX_DATA(name, buffer, size) {                                 \
  (msg_t *)(buffer),                                                        \
  (msg_t *)(buffer) + size,                                                 \
  (msg_t *)(buffer),                                                        \
  (msg_t *)(buffer),                                                        \
  (cnt_t)0,                                                                 \
  false,                                                                    \
  _THREADS_QUEUE_DATA(name.qw),                                             \
  _THREADS_QUEUE_DATA(name.qr),                                             \
}

/**
 * @brief   Static mailbox initializer.
 * @details Statically initialized mailboxes require no explicit
 *          initialization using @p chMBObjectInit().
 *
 * @param[in] name      the name of the mailbox variable
 * @param[in] buffer    pointer to the mailbox buffer array of @p msg_t
 * @param[in] size      number of @p msg_t elements in the buffer array
 */
#define MAILBOX_DECL(name, buffer, size)                                    \
  mailbox_t name = _MAILBOX_DATA(name, buffer, size)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void chMBObjectInit(mailbox_t *mbp, msg_t *buf, cnt_t n);
  void chMBReset(mailbox_t *mbp);
  void chMBResetI(mailbox_t *mbp);
  msg_t chMBPost(mailbox_t *mbp, msg_t msg, systime_t timeout);
  msg_t chMBPostS(mailbox_t *mbp, msg_t msg, systime_t timeout);
  msg_t chMBPostI(mailbox_t *mbp, msg_t msg);
  msg_t chMBPostAhead(mailbox_t *mbp, msg_t msg, systime_t timeout);
  msg_t chMBPostAheadS(mailbox_t *mbp, msg_t msg, systime_t timeout);
  msg_t chMBPostAheadI(mailbox_t *mbp, msg_t msg);
  msg_t chMBFetch(mailbox_t *mbp, msg_t *msgp, systime_t timeout);
  msg_t chMBFetchS(mailbox_t *mbp, msg_t *msgp, systime_t timeout);
  msg_t chMBFetchI(mailbox_t *mbp, msg_t *msgp);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Returns the mailbox buffer size as number of messages.
 *
 * @param[in] mbp       the pointer to an initialized mailbox_t object
 * @return              The size of the mailbox.
 *
 * @iclass
 */
static inline cnt_t chMBGetSizeI(const mailbox_t *mbp) {

  /*lint -save -e9033 [10.8] Perfectly safe pointers
    arithmetic.*/
  return (cnt_t)(mbp->top - mbp->buffer);
  /*lint -restore*/
}

/**
 * @brief   Returns the number of used message slots into a mailbox.
 *
 * @param[in] mbp       the pointer to an initialized mailbox_t object
 * @return              The number of queued messages.
 * @retval QUEUE_RESET  if the queue is in reset state.
 *
 * @iclass
 */
static inline cnt_t chMBGetUsedCountI(const mailbox_t *mbp) {

  chDbgCheckClassI();

  return mbp->cnt;
}

/**
 * @brief   Returns the number of free message slots into a mailbox.
 *
 * @param[in] mbp       the pointer to an initialized mailbox_t object
 * @return              The number of empty message slots.
 *
 * @iclass
 */
static inline cnt_t chMBGetFreeCountI(const mailbox_t *mbp) {

  chDbgCheckClassI();

  return chMBGetSizeI(mbp) - chMBGetUsedCountI(mbp);
}

/**
 * @brief   Returns the next message in the queue without removing it.
 * @pre     A message must be waiting in the queue for this function to work
 *          or it would return garbage. The correct way to use this macro is
 *          to use @p chMBGetFullCountI() and then use this macro, all within
 *          a lock state.
 *
 * @param[in] mbp       the pointer to an initialized mailbox_t object
 * @return              The next message in queue.
 *
 * @iclass
 */
static inline msg_t chMBPeekI(const mailbox_t *mbp) {

  chDbgCheckClassI();

  return *mbp->rdptr;
}

/**
 * @brief   Terminates the reset state.
 *
 * @param[in] mbp       the pointer to an initialized mailbox_t object
 *
 * @xclass
 */
static inline void chMBResumeX(mailbox_t *mbp) {

  mbp->reset = false;
}

#endif /* CH_CFG_USE_MAILBOXES == TRUE */

#endif /* CHMBOXES_H */

/** @} */
