/*
    ChibiOS/RT - Copyright (C) 2006-20s13 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup ChibiOS ChibiOS Interface
 * @{ 
 *
 * @file       PiOS/inc/board.h
 *****************************************************************************/

#ifndef _BOARD_H_
#define _BOARD_H_

#include "pios_chibios_transition_priv.h"

/* Some older STM32F4 revisions might get antsy when flash prefetching is enabled.
   This makes sure the affected revs don't get enabled by ChibiOS. */
#define STM32_USE_REVISION_A_FIX

/*
 * Board oscillators-related settings.
 * NOTE: LSE not fitted.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                0
#endif

#define STM32_HSECLK                HSE_VALUE

/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD                   330

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */

/**
 * @}
 * @}
 */
