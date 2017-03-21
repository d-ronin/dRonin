/**
 ******************************************************************************
 * @file       pios_mutex.h
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


#ifndef PIOS_MUTEX_H_
#define PIOS_MUTEX_H_

#define PIOS_MUTEX_TIMEOUT_MAX 0xffffffff

#include <stdint.h>
#include <stdbool.h>

struct pios_mutex;
struct pios_recursive_mutex;

/*
 * The following functions implement the concept of a non-recursive mutex usable
 * with PIOS_INCLUDE_CHIBIOS.
 *
 * Note that this is not the same as:
 * - binary semaphore
 * - semaphore
 * - recursive mutex
 *
 * see FreeRTOS documentation for details: http://www.freertos.org/a00113.html
 * see ChibiOS documentation for details: http://chibios.sourceforge.net/html/group__synchronization.html
 */

struct pios_mutex *PIOS_Mutex_Create(void);
bool PIOS_Mutex_Lock(struct pios_mutex *mtx, uint32_t timeout_ms);
bool PIOS_Mutex_Unlock(struct pios_mutex *mtx);

/*
 * The following functions implement the concept of a recursive mutex usable
 * with PIOS_INCLUDE_CHIBIOS.
 *
 * Note that this is not the same as:
 * - binary semaphore
 * - semaphore
 * - non-recursive mutex
 *
 * Note that this implementation doesn't prevent priority inversion.
 *
 * see FreeRTOS documentation for details: http://www.freertos.org/a00113.html
 * see ChibiOS documentation for details: http://chibios.sourceforge.net/html/group__synchronization.html
 */

struct pios_recursive_mutex *PIOS_Recursive_Mutex_Create(void);
bool PIOS_Recursive_Mutex_Lock(struct pios_recursive_mutex *mtx, uint32_t timeout_ms);
bool PIOS_Recursive_Mutex_Unlock(struct pios_recursive_mutex *mtx);

#endif /* PIOS_MUTEX_H_ */

/**
  * @}
  * @}
  */
