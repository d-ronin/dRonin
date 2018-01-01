/*
    SPC5 HAL - Copyright (C) 2013 STMicroelectronics

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
 * @file    SPC58NExx/hal_lld.h
 * @brief   SPC58NExx HAL subsystem low level driver header.
 * @pre     This module requires the following macros to be defined in the
 *          @p board.h file:
 *          - SPC5_XOSC_CLK.
 *          - SPC5_OSC_BYPASS (optionally).
 *          .
 *
 * @addtogroup HAL
 * @{
 */

#ifndef HAL_LLD_H
#define HAL_LLD_H

#include "registers.h"
#include "spc5_registry.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Defines the support for realtime counters in the HAL.
 */
#define HAL_IMPLEMENTS_COUNTERS             TRUE

/**
 * @name    Platform identification
 * @{
 */
#define PLATFORM_NAME                       "SPC58NExx High End Powertrain"
/** @} */

/**
 * @name    Absolute Maximum Ratings
 * @{
 */
/**
 * @brief   Maximum XOSC clock frequency.
 */
#define SPC5_XOSC_CLK_MAX                   40000000

/**
 * @brief   Minimum XOSC clock frequency.
 */
#define SPC5_XOSC_CLK_MIN                   4000000

/**
 * @brief   Maximum PLL0 input clock frequency.
 */
#define SPC5_PLL0IN_MIN                     8000000

/**
 * @brief   Maximum PLL0 input clock frequency.
 */
#define SPC5_PLL0IN_MAX                     44000000

/**
 * @brief   Maximum PLL1 input clock frequency.
 */
#define SPC5_PLL1IN_MIN                     38000000

/**
 * @brief   Maximum PLL1 input clock frequency.
 */
#define SPC5_PLL1IN_MAX                     78000000

/**
 * @brief   Maximum PLL0 VCO clock frequency.
 */
#define SPC5_PLL0VCO_MAX                    1250000000

/**
 * @brief   Maximum PLL0 VCO clock frequency.
 */
#define SPC5_PLL0VCO_MIN                    600000000

/**
 * @brief   Maximum PLL1 VCO clock frequency.
 */
#define SPC5_PLL1VCO_MAX                    1250000000

/**
 * @brief   Maximum PLL1 VCO clock frequency.
 */
#define SPC5_PLL1VCO_MIN                    600000000

/**
 * @brief   Maximum PLL0 output clock frequency.
 */
#define SPC5_PLL0_CLK_MAX                   625000000

/**
 * @brief   Maximum PLL0 output clock frequency.
 */
#define SPC5_PLL0_CLK_MIN                   4762000

/**
 * @brief   Maximum PLL1 output clock frequency.
 */
#define SPC5_PLL1_CLK_MAX                   625000000

/**
 * @brief   Maximum PLL1 output clock frequency.
 */
#define SPC5_PLL1_CLK_MIN                   4762000

/**
 * @brief   Maximum AC0 DC0 clock frequency.
 */
#define SPC5_AC0_DC0_CLK_MAX                100000000

/**
 * @brief   Maximum AC0 DC1 clock frequency.
 */
#define SPC5_AC0_DC1_CLK_MAX                16000000

/**
 * @brief   Maximum AC0 DC2 clock frequency.
 */
#define SPC5_AC0_DC2_CLK_MAX                16000000

/**
 * @brief   Maximum AC0 DC3 clock frequency.
 */
#define SPC5_AC0_DC3_CLK_MAX                100000000

/**
 * @brief   Maximum AC0 DC4 clock frequency.
 */
#define SPC5_AC0_DC4_CLK_MAX                50000000

/**
 * @brief   Maximum AC6 DC0 clock frequency.
 */
#define SPC5_AC6_DC0_CLK_MAX                100000000

/**
 * @brief   Maximum AC7 DC0 clock frequency.
 */
#define SPC5_AC7_DC0_CLK_MAX                100000000

/**
 * @brief   Maximum AC8 DC0 clock frequency.
 */
#define SPC5_AC8_DC0_CLK_MAX                80000000

/**
 * @brief   Maximum AC9 DC0 clock frequency.
 */
#define SPC5_AC9_DC0_CLK_MAX                40000000

/**
 * @brief   Maximum AC11 DC0 clock frequency.
 */
#define SPC5_AC11_DC0_CLK_MAX               80000000

/**
 * @brief   Maximum AC12 DC0 clock frequency.
 */
#define SPC5_AC12_DC0_CLK_MAX               100000000

/**
 * @brief   Maximum AC12 DC1 clock frequency.
 */
#define SPC5_AC12_DC1_CLK_MAX               100000000

/**
 * @brief   Maximum AC12 DC2 clock frequency.
 */
#define SPC5_AC12_DC2_CLK_MAX               100000000

/**
 * @brief   Maximum AC12 DC3 clock frequency.
 */
#define SPC5_AC12_DC3_CLK_MAX               100000000

/**
 * @brief   Maximum AC12 DC4 clock frequency.
 */
#define SPC5_AC12_DC4_CLK_MAX               100000000
/** @} */

/**
 * @name    Internal clock sources
 * @{
 */
#define SPC5_IRC_CLK                        16000000
/** @} */

/**
 * @name    PLL0 registers bits definitions
 * @{
 */
#define SPC5_PLL0_CR_EXPDIE                 (1U << 7)
#define SPC5_PLL0_CR_LOLIE                  (1U << 3)
#define SPC5_PLL0_CR_LOLRE                  (1U << 2)

#define SPC5_PLL0_SR_EXTPDF                 (1U << 7)
#define SPC5_PLL0_SR_LOLIF                  (1U << 3)
#define SPC5_PLL0_SR_LOCK                   (1U << 2)

#define SPC5_PLL0_DV_RFDPHI1_MASK           (15U << 27)
#define SPC5_PLL0_DV_RFDPHI1(n)             ((n) << 27)

#define SPC5_PLL0_DV_RFDPHI_MASK            (63U << 16)
#define SPC5_PLL0_DV_RFDPHI(n)              ((n) << 16)

#define SPC5_PLL0_DV_PREDIV_MASK            (15U << 12)
#define SPC5_PLL0_DV_PREDIV(n)              ((n) << 12)

#define SPC5_PLL0_DV_MFD_MASK               (127U << 0)
#define SPC5_PLL0_DV_MFD(n)                 ((n) << 0)
/** @} */

/**
 * @name    PLL1 registers bits definitions
 * @{
 */
#define SPC5_PLL1_CR_EXPDIE                 (1U << 7)
#define SPC5_PLL1_CR_LOLIE                  (1U << 3)
#define SPC5_PLL1_CR_LOLRE                  (1U << 2)
#define SPC5_PLL1_SR_EXTPDF                 (1U << 7)
#define SPC5_PLL1_SR_LOLIF                  (1U << 3)
#define SPC5_PLL1_SR_LOCK                   (1U << 2)

#define SPC5_PLL1_DV_RFDPHI_MASK            (63U << 16)
#define SPC5_PLL1_DV_RFDPHI(n)              ((n) << 16)

#define SPC5_PLL1_DV_MFD_MASK               (127U << 0)
#define SPC5_PLL1_DV_MFD(n)                 ((n) << 0)

#define SPC5_PLL1_FM_MODEN                  (1U << 30)
#define SPC5_PLL1_FM_MODSEL                 (1U << 29)
#define SPC5_PLL1_FM_MODPRD_MASK            (0x1FFFU << 16)
#define SPC5_PLL1_FM_INCSTP_MASK            (0x7FFFU << 0)

#define SPC5_PLL1_FD_FDEN                   (1U << 30)
#define SPC5_PLL1_FD_DTHDIS_MASK            (3U << 16)
#define SPC5_PLL1_FD_DTHDIS(n)              ((n) << 16)
#define SPC5_PLL1_FD_FRCDIV_MASK            (0xFFF << 0)
#define SPC5_PLL1_FD_FRCDIV(n)              ((n) << 0)
/** @} */

/**
 * @name    Clock selectors used in the various GCM SC registers
 * @{
 */
#define SPC5_CGM_SC_MASK                    (15U << 24)
#define SPC5_CGM_SC_IRC                     (0U << 24)
#define SPC5_CGM_SC_XOSC                    (1U << 24)
#define SPC5_CGM_SC_PLL0PHI                 (2U << 24)
#define SPC5_CGM_SC_PLL0PHI1                (3U << 24)
#define SPC5_CGM_SC_PLL1PHI                 (4U << 24)
#define SPC5_CGM_SC_LFAST                   (5U << 24)
#define SPC5_CGM_SC_RTC_CLK                 (6U << 24)
/** @} */

/**
 * @name    Clock dividers used in the CGM_AC12_DC0.DIV_FMT field
 * @{
 */
#define SPC5_CGM_DIV_FMT_DIV1               (0U << 0)
#define SPC5_CGM_DIV_FMT_DIV10              (1U << 0)
#define SPC5_CGM_DIV_FMT_DIV100             (2U << 0)
#define SPC5_CGM_DIV_FMT_DIV1000            (3U << 0)
/** @} */

/**
 * @name    ME_GS register bits definitions
 * @{
 */
#define SPC5_ME_GS_SYSCLK_MASK              (15U << 0)
#define SPC5_ME_GS_SYSCLK_IRC               (0U << 0)
#define SPC5_ME_GS_SYSCLK_XOSC              (1U << 0)
#define SPC5_ME_GS_SYSCLK_PLL0PHI           (2U << 0)
#define SPC5_ME_GS_SYSCLK_PLL1PHI           (4U << 0)
/** @} */

/**
 * @name    ME_ME register bits definitions
 * @{
 */
#define SPC5_ME_ME_RESET                    (1U << 0)
#define SPC5_ME_ME_TEST                     (1U << 1)
#define SPC5_ME_ME_SAFE                     (1U << 2)
#define SPC5_ME_ME_DRUN                     (1U << 3)
#define SPC5_ME_ME_RUN0                     (1U << 4)
#define SPC5_ME_ME_RUN1                     (1U << 5)
#define SPC5_ME_ME_RUN2                     (1U << 6)
#define SPC5_ME_ME_RUN3                     (1U << 7)
#define SPC5_ME_ME_HALT0                    (1U << 8)
#define SPC5_ME_ME_STOP0                    (1U << 10)
/** @} */

/**
 * @name    ME_CCTLx register bits definitions
 * @{
 */
#define SPC5_ME_CCTL_RESET                  (1U << 0)
#define SPC5_ME_CCTL_TEST                   (1U << 1)
#define SPC5_ME_CCTL_SAFE                   (1U << 2)
#define SPC5_ME_CCTL_DRUN                   (1U << 3)
#define SPC5_ME_CCTL_RUN0                   (1U << 4)
#define SPC5_ME_CCTL_RUN1                   (1U << 5)
#define SPC5_ME_CCTL_RUN2                   (1U << 6)
#define SPC5_ME_CCTL_RUN3                   (1U << 7)
#define SPC5_ME_CCTL_HALT0                  (1U << 8)
#define SPC5_ME_CCTL_STOP0                  (1U << 10)
/** @} */

/**
 * @name    ME_xxx_MC registers bits definitions
 * @{
 */
#define SPC5_ME_MC_SYSCLK_MASK              (15U << 0)
#define SPC5_ME_MC_SYSCLK(n)                ((n) << 0)
#define SPC5_ME_MC_SYSCLK_IRC               SPC5_ME_MC_SYSCLK(0)
#define SPC5_ME_MC_SYSCLK_XOSC              SPC5_ME_MC_SYSCLK(1)
#define SPC5_ME_MC_SYSCLK_PLL0PHI           SPC5_ME_MC_SYSCLK(2)
#define SPC5_ME_MC_SYSCLK_PLL1PHI           SPC5_ME_MC_SYSCLK(4)
#define SPC5_ME_MC_SYSCLK_DISABLED          SPC5_ME_MC_SYSCLK(15)
#define SPC5_ME_MC_IRCON                    (1U << 4)
#define SPC5_ME_MC_XOSC0ON                  (1U << 5)
#define SPC5_ME_MC_PLL0ON                   (1U << 6)
#define SPC5_ME_MC_PLL1ON                   (1U << 7)
#define SPC5_ME_MC_FLAON_MASK               (3U << 16)
#define SPC5_ME_MC_FLAON(n)                 ((n) << 16)
#define SPC5_ME_MC_FLAON_PD                 SPC5_ME_MC_FLAON(1)
#define SPC5_ME_MC_FLAON_LP                 SPC5_ME_MC_FLAON(2)
#define SPC5_ME_MC_FLAON_NORMAL             SPC5_ME_MC_FLAON(3)
#define SPC5_ME_MC_MVRON                    (1U << 20)
#define SPC5_ME_MC_PDO                      (1U << 23)
#define SPC5_ME_MC_PWRLVL_MASK              (7U << 28)
#define SPC5_ME_MC_PWRLVL(n)                ((n) << 28)
/** @} */

/**
 * @name    ME_MCTL register bits definitions
 * @{
 */
#define SPC5_ME_MCTL_KEY                    0x5AF0U
#define SPC5_ME_MCTL_KEY_INV                0xA50FU
#define SPC5_ME_MCTL_MODE_MASK              (15U << 28)
#define SPC5_ME_MCTL_MODE(n)                ((n) << 28)
/** @} */

/**
 * @name    ME_RUN_PCx registers bits definitions
 * @{
 */
#define SPC5_ME_RUN_PC_SAFE                 (1U << 2)
#define SPC5_ME_RUN_PC_DRUN                 (1U << 3)
#define SPC5_ME_RUN_PC_RUN0                 (1U << 4)
#define SPC5_ME_RUN_PC_RUN1                 (1U << 5)
#define SPC5_ME_RUN_PC_RUN2                 (1U << 6)
#define SPC5_ME_RUN_PC_RUN3                 (1U << 7)
/** @} */

/**
 * @name    ME_LP_PCx registers bits definitions
 * @{
 */
#define SPC5_ME_LP_PC_HALT0                 (1U << 8)
#define SPC5_ME_LP_PC_STOP0                 (1U << 10)
/** @} */

/**
 * @name    ME_PCTL registers bits definitions
 * @{
 */
#define SPC5_ME_PCTL_RUN_MASK               (7U << 0)
#define SPC5_ME_PCTL_RUN(n)                 ((n) << 0)
#define SPC5_ME_PCTL_LP_MASK                (7U << 3)
#define SPC5_ME_PCTL_LP(n)                  ((n) << 3)
#define SPC5_ME_PCTL_DBG                    (1U << 6)
/** @} */

/**
 * @name    SSCM_ERROR register bits definitions
 * @{
 */
#define SPC5_SSCM_ERROR_RAE                 (1U << 0)
#define SPC5_SSCM_ERROR_PAE                 (1U << 1)
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Disables the clocks initialization in the HAL.
 */
#if !defined(SPC5_NO_INIT) || defined(__DOXYGEN__)
#define SPC5_NO_INIT                        FALSE
#endif

/**
 * @brief   Disables the overclock checks.
 */
#if !defined(SPC5_ALLOW_OVERCLOCK) || defined(__DOXYGEN__)
#define SPC5_ALLOW_OVERCLOCK                FALSE
#endif

/**
 * @brief   Disables the watchdogs on start.
 */
#if !defined(SPC5_DISABLE_WATCHDOG) || defined(__DOXYGEN__)
#define SPC5_DISABLE_WATCHDOG               TRUE
#endif

/**
 * @brief   PLL0 PREDIV divider value.
 * @note    The default value is calculated for XOSC=40MHz and PLL0PHI=400MHz.
 */
#if !defined(SPC5_PLL0_PREDIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_PLL0_PREDIV_VALUE              2
#endif

/**
 * @brief   PLL0 MFD multiplier value.
 * @note    The default value is calculated for XOSC=40MHz and PLL0PHI=400MHz.
 */
#if !defined(SPC5_PLL0_MFD_VALUE) || defined(__DOXYGEN__)
#define SPC5_PLL0_MFD_VALUE                 40
#endif

/**
 * @brief   PLL0 RFDPHI divider value.
 * @note    The default value is calculated for XOSC=40MHz and PLL0PHI=400MHz.
 */
#if !defined(SPC5_PLL0_RFDPHI_VALUE) || defined(__DOXYGEN__)
#define SPC5_PLL0_RFDPHI_VALUE              2
#endif

/**
 * @brief   PLL0 RFDPHI1 divider value.
 * @note    The default value is calculated for XOSC=40MHz and PLL0PHI1=66.6MHz.
 */
#if !defined(SPC5_PLL0_RFDPHI1_VALUE) || defined(__DOXYGEN__)
#define SPC5_PLL0_RFDPHI1_VALUE             12
#endif

/**
 * @brief   PLL1 MFD multiplier value.
 * @note    The default value is calculated for XOSC=40MHz and PLL1PHI=200MHz.
 */
#if !defined(SPC5_PLL1_MFD_VALUE) || defined(__DOXYGEN__)
#define SPC5_PLL1_MFD_VALUE                 20
#endif

/**
 * @brief   PLL1 RFDPHI divider value.
 * @note    The default value is calculated for XOSC=40MHz and PLL1PHI=200MHz.
 */
#if !defined(SPC5_PLL1_RFDPHI_VALUE) || defined(__DOXYGEN__)
#define SPC5_PLL1_RFDPHI_VALUE              2
#endif

/**
 * @brief   CGM_SC_DC0 clock divider value.
 * @note    Range 1..64, zero means disabled clock.
 * @note    The dividers on the SC must have values that are multiples of
 *          all the other SC dividers except the lowest one.
 */
#if !defined(SPC5_CGM_SC_DC0_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_SC_DC0_DIV_VALUE           1
#endif

/**
 * @brief   CGM_SC_DC1 clock divider value.
 * @note    Range 1..64, zero means disabled clock.
 * @note    The dividers on the SC must have values that are multiples of
 *          all the other SC dividers except the lowest one.
 */
#if !defined(SPC5_CGM_SC_DC1_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_SC_DC1_DIV_VALUE           2
#endif

/**
 * @brief   CGM_SC_DC2 clock divider value.
 * @note    Range 1..64, zero means disabled clock.
 * @note    The dividers on the SC must have values that are multiples of
 *          all the other SC dividers except the lowest one.
 */
#if !defined(SPC5_CGM_SC_DC2_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_SC_DC2_DIV_VALUE           4
#endif

/**
 * @brief   CGM_SC_DC3 clock divider value.
 * @note    Range 1..64, zero means disabled clock.
 * @note    The dividers on the SC must have values that are multiples of
 *          all the other SC dividers except the lowest one.
 */
#if !defined(SPC5_CGM_SC_DC3_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_SC_DC3_DIV_VALUE           1
#endif

#if !defined(_SPC58NE84C3_CUT1_) || defined(__DOXYGEN__)
/**
 * @brief   CGM_SC_DC4 clock divider value.
 * @note    Range 1..64, zero means disabled clock.
 * @note    The dividers on the SC must have values that are multiples of
 *          all the other SC dividers except the lowest one.
 */
#if !defined(SPC5_CGM_SC_DC4_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_SC_DC4_DIV_VALUE           1
#endif
#endif /* !defined(_SPC58NE84C3_CUT1_) */

/**
 * @brief   CGM_AC0_SC clock source.
 */
#if !defined(SPC5_CGM_AC0_SC_BITS) || defined(__DOXYGEN__)
#define SPC5_CGM_AC0_SC_BITS                SPC5_CGM_SC_PLL0PHI
#endif

/**
 * @brief   CGM_AC0_DC0 clock divider value.
 * @note    Range 1..16, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC0_DC0_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC0_DC0_DIV_VALUE          4
#endif

/**
 * @brief   CGM_AC0_DC1 clock divider value.
 * @note    Range 1..128, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC0_DC1_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC0_DC1_DIV_VALUE          25
#endif

/**
 * @brief   CGM_AC0_DC2 clock divider value.
 * @note    Range 1..128, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC0_DC2_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC0_DC2_DIV_VALUE          25
#endif

/**
 * @brief   CGM_AC0_DC3 clock divider value.
 * @note    Range 1..16, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC0_DC3_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC0_DC3_DIV_VALUE          4
#endif

#if defined(_SPC58NE84C3_CUT1_) || defined(__DOXYGEN__)
/**
 * @brief   CGM_AC0_DC4 clock divider value.
 * @note    Range 1..16, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC0_DC4_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC0_DC4_DIV_VALUE          8
#endif
#endif /* defined(_SPC58NE84C3_CUT1_) */

/**
 * @brief   CGM_AC3_SC clock source.
 */
#if !defined(SPC5_CGM_AC3_SC_BITS) || defined(__DOXYGEN__)
#define SPC5_CGM_AC3_SC_BITS                SPC5_CGM_SC_XOSC
#endif

/**
 * @brief   CGM_AC4_SC clock source.
 */
#if !defined(SPC5_CGM_AC4_SC_BITS) || defined(__DOXYGEN__)
#define SPC5_CGM_AC4_SC_BITS                SPC5_CGM_SC_XOSC
#endif

/**
 * @brief   CGM_AC6_SC clock source.
 */
#if !defined(SPC5_CGM_AC6_SC_BITS) || defined(__DOXYGEN__)
#define SPC5_CGM_AC6_SC_BITS                SPC5_CGM_SC_XOSC
#endif

/**
 * @brief   CGM_AC6_DC0 clock divider value.
 * @note    Range 1..512, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC6_DC0_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC6_DC0_DIV_VALUE          8
#endif

/**
 * @brief   CGM_AC7_SC clock source.
 */
#if !defined(SPC5_CGM_AC7_SC_BITS) || defined(__DOXYGEN__)
#define SPC5_CGM_AC7_SC_BITS                SPC5_CGM_SC_XOSC
#endif

/**
 * @brief   CGM_AC7_DC0 clock divider value.
 * @note    Range 1..512, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC7_DC0_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC7_DC0_DIV_VALUE          8
#endif

/**
 * @brief   CGM_AC8_SC clock source.
 */
#if !defined(SPC5_CGM_AC8_SC_BITS) || defined(__DOXYGEN__)
#define SPC5_CGM_AC8_SC_BITS                SPC5_CGM_SC_XOSC
#endif

/**
 * @brief   CGM_AC8_DC0 clock divider value.
 * @note    Range 1..64, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC8_DC0_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC8_DC0_DIV_VALUE          5
#endif

/**
 * @brief   CGM_AC9_SC clock source.
 */
#if !defined(SPC5_CGM_AC9_SC_BITS) || defined(__DOXYGEN__)
#define SPC5_CGM_AC9_SC_BITS                SPC5_CGM_SC_XOSC
#endif

/**
 * @brief   CGM_AC9_DC0 clock divider value.
 * @note    Range 1..64, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC9_DC0_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC9_DC0_DIV_VALUE          5
#endif

/**
 * @brief   CGM_AC11_SC clock source.
 */
#if !defined(SPC5_CGM_AC11_SC_BITS) || defined(__DOXYGEN__)
#define SPC5_CGM_AC11_SC_BITS               SPC5_CGM_SC_XOSC
#endif

/**
 * @brief   CGM_AC11_DC0 clock divider value.
 * @note    Range 1..64, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC11_DC0_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC11_DC0_DIV_VALUE         5
#endif

/**
 * @brief   CGM_AC12_SC clock source.
 */
#if !defined(SPC5_CGM_AC12_SC_BITS) || defined(__DOXYGEN__)
#define SPC5_CGM_AC12_SC_BITS               SPC5_CGM_SC_PLL0PHI
#endif

/**
 * @brief   CGM_AC12_DC0 clock divider value.
 * @note    Range 1..512, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC12_DC0_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC12_DC0_DIV_VALUE         4
#endif

/**
 * @brief   CGM_AC12_DC0 clock divider multiplier value.
 * @note    Possible values are 1, 10, 100 and 1000.
 */
#if !defined(SPC5_CGM_AC12_DC0_DIV_FMT_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC12_DC0_DIV_FMT_VALUE     1
#endif

/**
 * @brief   CGM_AC12_DC1 clock divider value.
 * @note    Range 1..16, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC12_DC1_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC12_DC1_DIV_VALUE         4
#endif

/**
 * @brief   CGM_AC12_DC2 clock divider value.
 * @note    Range 1..16, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC12_DC2_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC12_DC2_DIV_VALUE         4
#endif

/**
 * @brief   CGM_AC12_DC3 clock divider value.
 * @note    Range 1..16, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC12_DC3_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC12_DC3_DIV_VALUE         4
#endif

/**
 * @brief   CGM_AC12_DC4 clock divider value.
 * @note    Range 1..16, zero means disabled clock.
 */
#if !defined(SPC5_CGM_AC12_DC4_DIV_VALUE) || defined(__DOXYGEN__)
#define SPC5_CGM_AC12_DC4_DIV_VALUE         4
#endif

/**
 * @brief   Active run modes in ME_ME register.
 * @note    Modes RESET, SAFE, DRUN, and RUN0 modes are always enabled, there
 *          is no need to specify them.
 */
#if !defined(SPC5_ME_ME_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_ME_BITS                     (SPC5_ME_ME_RUN1 |              \
                                             SPC5_ME_ME_RUN2 |              \
                                             SPC5_ME_ME_RUN3 |              \
                                             SPC5_ME_ME_HALT0 |             \
                                             SPC5_ME_ME_STOP0)
#endif

/**
 * @brief   SAFE mode settings.
 */
#if !defined(SPC5_ME_SAFE_MC_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_SAFE_MC_BITS                (SPC5_ME_MC_PDO)
#endif

/**
 * @brief   DRUN mode settings.
 */
#if !defined(SPC5_ME_DRUN_MC_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_DRUN_MC_BITS                (SPC5_ME_MC_SYSCLK_PLL1PHI |    \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_FLAON_NORMAL |      \
                                             SPC5_ME_MC_MVRON)
#endif

/**
 * @brief   RUN0 mode settings.
 */
#if !defined(SPC5_ME_RUN0_MC_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN0_MC_BITS                (SPC5_ME_MC_SYSCLK_PLL1PHI |    \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_FLAON_NORMAL |      \
                                             SPC5_ME_MC_MVRON)
#endif

/**
 * @brief   RUN1 mode settings.
 */
#if !defined(SPC5_ME_RUN1_MC_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN1_MC_BITS                (SPC5_ME_MC_SYSCLK_PLL1PHI |    \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_FLAON_NORMAL |      \
                                             SPC5_ME_MC_MVRON)
#endif

/**
 * @brief   RUN2 mode settings.
 */
#if !defined(SPC5_ME_RUN2_MC_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN2_MC_BITS                (SPC5_ME_MC_SYSCLK_PLL1PHI |    \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_FLAON_NORMAL |      \
                                             SPC5_ME_MC_MVRON)
#endif

/**
 * @brief   RUN3 mode settings.
 */
#if !defined(SPC5_ME_RUN3_MC_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN3_MC_BITS                (SPC5_ME_MC_SYSCLK_PLL1PHI |    \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_FLAON_NORMAL |      \
                                             SPC5_ME_MC_MVRON)
#endif

/**
 * @brief   HALT0 mode settings.
 */
#if !defined(SPC5_ME_HALT0_MC_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_HALT0_MC_BITS               (SPC5_ME_MC_SYSCLK_PLL1PHI |    \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_FLAON_NORMAL |      \
                                             SPC5_ME_MC_MVRON)
#endif

/**
 * @brief   STOP0 mode settings.
 */
#if !defined(SPC5_ME_STOP0_MC_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_STOP0_MC_BITS               (SPC5_ME_MC_SYSCLK_PLL1PHI |    \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_FLAON_NORMAL |      \
                                             SPC5_ME_MC_MVRON)
#endif

/**
 * @brief   Peripheral mode 0 (run mode).
 * @note    Do not change this setting, it is expected to be the "never run"
 *          mode.
 */
#if !defined(SPC5_ME_RUN_PC0_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN_PC0_BITS                0
#endif

/**
 * @brief   Peripheral mode 1 (run mode).
 * @note    Do not change this setting, it is expected to be the "always run"
 *          mode.
 */
#if !defined(SPC5_ME_RUN_PC1_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN_PC1_BITS                (SPC5_ME_RUN_PC_SAFE |          \
                                             SPC5_ME_RUN_PC_DRUN |          \
                                             SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#endif

/**
 * @brief   Peripheral mode 2 (run mode).
 * @note    Do not change this setting, it is expected to be the "only during
 *          normal run" mode.
 */
#if !defined(SPC5_ME_RUN_PC2_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN_PC2_BITS                (SPC5_ME_RUN_PC_DRUN |          \
                                             SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#endif

/**
 * @brief   Peripheral mode 3 (run mode).
 * @note    Not defined, available to application-specific modes.
 */
#if !defined(SPC5_ME_RUN_PC3_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN_PC3_BITS                (SPC5_ME_RUN_PC_DRUN |          \
                                             SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#endif

/**
 * @brief   Peripheral mode 4 (run mode).
 * @note    Not defined, available to application-specific modes.
 */
#if !defined(SPC5_ME_RUN_PC4_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN_PC4_BITS                (SPC5_ME_RUN_PC_DRUN |          \
                                             SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#endif

/**
 * @brief   Peripheral mode 5 (run mode).
 * @note    Not defined, available to application-specific modes.
 */
#if !defined(SPC5_ME_RUN_PC5_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN_PC5_BITS                (SPC5_ME_RUN_PC_DRUN |          \
                                             SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#endif

/**
 * @brief   Peripheral mode 6 (run mode).
 * @note    Not defined, available to application-specific modes.
 */
#if !defined(SPC5_ME_RUN_PC6_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN_PC6_BITS                (SPC5_ME_RUN_PC_DRUN |          \
                                             SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#endif

/**
 * @brief   Peripheral mode 7 (run mode).
 * @note    Not defined, available to application-specific modes.
 */
#if !defined(SPC5_ME_RUN_PC7_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_RUN_PC7_BITS                (SPC5_ME_RUN_PC_DRUN |          \
                                             SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#endif

/**
 * @brief   Peripheral mode 0 (low power mode).
 * @note    Do not change this setting, it is expected to be the "never run"
 *          mode.
 */
#if !defined(SPC5_ME_LP_PC0_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_LP_PC0_BITS                 0
#endif

/**
 * @brief   Peripheral mode 1 (low power mode).
 * @note    Do not change this setting, it is expected to be the "always run"
 *          mode.
 */
#if !defined(SPC5_ME_LP_PC1_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_LP_PC1_BITS                 (SPC5_ME_LP_PC_HALT0 |          \
                                             SPC5_ME_LP_PC_STOP0)
#endif

/**
 * @brief   Peripheral mode 2 (low power mode).
 * @note    Do not change this setting, it is expected to be the "halt only"
 *          mode.
 */
#if !defined(SPC5_ME_LP_PC2_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_LP_PC2_BITS                 (SPC5_ME_LP_PC_HALT0)
#endif

/**
 * @brief   Peripheral mode 3 (low power mode).
 * @note    Do not change this setting, it is expected to be the "stop only"
 *          mode.
 */
#if !defined(SPC5_ME_LP_PC3_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_LP_PC3_BITS                 (SPC5_ME_LP_PC_STOP0)
#endif

/**
 * @brief   Peripheral mode 4 (low power mode).
 * @note    Not defined, available to application-specific modes.
 */
#if !defined(SPC5_ME_LP_PC4_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_LP_PC4_BITS                 (SPC5_ME_LP_PC_HALT0 |          \
                                             SPC5_ME_LP_PC_STOP0)
#endif

/**
 * @brief   Peripheral mode 5 (low power mode).
 * @note    Not defined, available to application-specific modes.
 */
#if !defined(SPC5_ME_LP_PC5_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_LP_PC5_BITS                 (SPC5_ME_LP_PC_HALT0 |          \
                                             SPC5_ME_LP_PC_STOP0)
#endif

/**
 * @brief   Peripheral mode 6 (low power mode).
 * @note    Not defined, available to application-specific modes.
 */
#if !defined(SPC5_ME_LP_PC6_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_LP_PC6_BITS                 (SPC5_ME_LP_PC_HALT0 |          \
                                             SPC5_ME_LP_PC_STOP0)
#endif

/**
 * @brief   Peripheral mode 7 (low power mode).
 * @note    Not defined, available to application-specific modes.
 */
#if !defined(SPC5_ME_LP_PC7_BITS) || defined(__DOXYGEN__)
#define SPC5_ME_LP_PC7_BITS                 (SPC5_ME_LP_PC_HALT0 |          \
                                             SPC5_ME_LP_PC_STOP0)
#endif

/**
 * @brief   SSCM.ERROR register initialization.
 */
#if !defined(SPC5_SSCM_ERROR_INIT) || defined(__DOXYGEN__)
#define SPC5_SSCM_ERROR_INIT                (SPC5_SSCM_ERROR_PAE |          \
                                             SPC5_SSCM_ERROR_RAE)
#endif

/**
 * @brief   PIT channel 0 IRQ priority.
 * @note    This PIT channel is allocated permanently for system tick
 *          generation.
 */
#if !defined(SPC5_PIT0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define SPC5_PIT0_IRQ_PRIORITY              INTC_PSR_ENABLE(INTC_PSR_CORE2, 4)
#endif

/**
 * @brief   Clock initialization failure hook.
 * @note    The default is to stop the system and let the RTC restart it.
 * @note    The hook code must not return.
 */
#if !defined(SPC5_CLOCK_FAILURE_HOOK) || defined(__DOXYGEN__)
#define SPC5_CLOCK_FAILURE_HOOK()           osalSysHalt("clock failure")
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*-----------------------------------------*
 * Configuration-related checks.           *
 *-----------------------------------------*/

#if !defined(SPC58NExx_MCUCONF)
#error "Using a wrong mcuconf.h file, SPC58NExx_MCUCONF not defined"
#endif

/*-----------------------------------------*
 * Oscillators-related checks.             *
 *-----------------------------------------*/

/* Check on the XOSC frequency.*/
#if (SPC5_XOSC_CLK < SPC5_XOSC_CLK_MIN) ||                                  \
    (SPC5_XOSC_CLK > SPC5_XOSC_CLK_MAX)
#error "invalid SPC5_XOSC_CLK value specified"
#endif

/*-----------------------------------------*
 * PLL-related checks.                     *
 *-----------------------------------------*/

/* Check on SPC5_PLL0_PREDIV_VALUE.*/
#if (SPC5_PLL0_PREDIV_VALUE < 0) || (SPC5_PLL0_PREDIV_VALUE > 7)
#error "invalid SPC5_PLL0_PREDIV_VALUE value specified"
#endif

/* Check on SPC5_PLL0_MFD_VALUE.*/
#if (SPC5_PLL0_MFD_VALUE < 8) || (SPC5_PLL0_MFD_VALUE > 127)
#error "invalid SPC5_PLL0_MFD_VALUE value specified"
#endif

/* Check on SPC5_PLL0_RFDPHI_VALUE.*/
#if (SPC5_PLL0_RFDPHI_VALUE < 1) || (SPC5_PLL0_RFDPHI_VALUE > 63)
#error "invalid SPC5_PLL0_RFDPHI_VALUE value specified"
#endif

/* Check on SPC5_PLL0_RFDPHI1_VALUE.*/
#if (SPC5_PLL0_RFDPHI1_VALUE < 4) || (SPC5_PLL0_RFDPHI1_VALUE > 15)
#error "invalid SPC5_PLL0_RFDPHI1_VALUE value specified"
#endif

/* Check on SPC5_PLL1_MFD_VALUE.*/
#if (SPC5_PLL1_MFD_VALUE < 16) || (SPC5_PLL1_MFD_VALUE > 34)
#error "invalid SPC5_PLL1_MFD_VALUE value specified"
#endif

/* Check on SPC5_PLL1_RFDPHI_VALUE.*/
#if (SPC5_PLL1_RFDPHI_VALUE < 2) || (SPC5_PLL1_RFDPHI_VALUE > 126) ||       \
    ((SPC5_PLL1_RFDPHI_VALUE & 1) != 0)
#error "invalid SPC5_PLL1_RFDPHI_VALUE value specified"
#endif

/*-----------------------------------------*
 * Mux-related checks and assignments.     *
 *-----------------------------------------*/

/* Check on SPC5_CGM_AC0_SC_BITS.*/
#if SPC5_CGM_AC0_SC_BITS == SPC5_CGM_SC_IRC
#define SPC5_AUX0_CLK                       SPC5_IRC_CLK
#elif SPC5_CGM_AC0_SC_BITS == SPC5_CGM_SC_XOSC
#define SPC5_AUX0_CLK                       SPC5_XOSC_CLK
#elif SPC5_CGM_AC0_SC_BITS == SPC5_CGM_SC_PLL0PHI
#define SPC5_AUX0_CLK                       SPC5_PLL0_PHI_CLK
#else
#error "invalid SPC5_CGM_AC0_SC_BITS value specified"
#endif

/* Check on SPC5_CGM_AC3_SC_BITS.*/
#if SPC5_CGM_AC3_SC_BITS == SPC5_CGM_SC_IRC
#define SPC5_PLL0_INPUT_CLK                 SPC5_IRC_CLK
#elif SPC5_CGM_AC3_SC_BITS == SPC5_CGM_SC_XOSC
#define SPC5_PLL0_INPUT_CLK                 SPC5_XOSC_CLK
#else
#error "invalid SPC5_CGM_AC3_SC_BITS value specified"
#endif

/* Check on SPC5_CGM_AC4_SC_BITS.*/
#if SPC5_CGM_AC4_SC_BITS == SPC5_CGM_SC_PLL0PHI1
#define SPC5_PLL1_INPUT_CLK                 SPC5_PLL0_PHI1_CLK
#elif SPC5_CGM_AC4_SC_BITS == SPC5_CGM_SC_XOSC
#define SPC5_PLL1_INPUT_CLK                 SPC5_XOSC_CLK
#else
#error "invalid SPC5_CGM_AC4_SC_BITS value specified"
#endif

/* Check on SPC5_CGM_AC6_SC_BITS.*/
#if SPC5_CGM_AC6_SC_BITS == SPC5_CGM_SC_IRC
#define SPC5_AUX6_CLK                      SPC5_IRC_CLK
#elif SPC5_CGM_AC6_SC_BITS == SPC5_CGM_SC_XOSC
#define SPC5_AUX6_CLK                      SPC5_XOSC_CLK
#elif SPC5_CGM_AC6_SC_BITS == SPC5_CGM_SC_PLL0PHI
#define SPC5_AUX6_CLK                      SPC5_PLL0_PHI_CLK
#elif SPC5_CGM_AC6_SC_BITS == SPC5_CGM_SC_PLL1PHI
#define SPC5_AUX6_CLK                      SPC5_PLL1_PHI_CLK
#elif SPC5_CGM_AC6_SC_BITS == SPC5_CGM_SC_RTC_CLK
#define SPC5_AUX6_CLK                      0 /* TODO: Define RTC clock.*/
#else
#error "invalid SPC5_CGM_AC6_SC_BITS value specified"
#endif

/* Check on SPC5_CGM_AC7_SC_BITS.*/
#if SPC5_CGM_AC7_SC_BITS == SPC5_CGM_SC_IRC
#define SPC5_AUX7_CLK                      SPC5_IRC_CLK
#elif SPC5_CGM_AC7_SC_BITS == SPC5_CGM_SC_XOSC
#define SPC5_AUX7_CLK                      SPC5_XOSC_CLK
#elif SPC5_CGM_AC7_SC_BITS == SPC5_CGM_SC_PLL0PHI
#define SPC5_AUX7_CLK                      SPC5_PLL0_PHI_CLK
#elif SPC5_CGM_AC7_SC_BITS == SPC5_CGM_SC_PLL1PHI
#define SPC5_AUX7_CLK                      SPC5_PLL1_PHI_CLK
#else
#error "invalid SPC5_CGM_AC7_SC_BITS value specified"
#endif

/* Check on SPC5_CGM_AC8_SC_BITS.*/
#if SPC5_CGM_AC8_SC_BITS == SPC5_CGM_SC_XOSC
#define SPC5_AUX8_CLK                      SPC5_XOSC_CLK
#elif SPC5_CGM_AC8_SC_BITS == SPC5_CGM_SC_PLL0PHI
#define SPC5_AUX8_CLK                      SPC5_PLL0_PHI_CLK
#else
#error "invalid SPC5_CGM_AC8_SC_BITS value specified"
#endif

/* Check on SPC5_CGM_AC9_SC_BITS.*/
#if SPC5_CGM_AC9_SC_BITS == SPC5_CGM_SC_XOSC
#define SPC5_AUX9_CLK                      SPC5_XOSC_CLK
#elif SPC5_CGM_AC9_SC_BITS == SPC5_CGM_SC_IRC
#define SPC5_AUX9_CLK                      SPC5_IRC_CLK
#else
#error "invalid SPC5_CGM_AC9_SC_BITS value specified"
#endif

/* Check on SPC5_CGM_AC11_SC_BITS.*/
#if SPC5_CGM_AC11_SC_BITS == SPC5_CGM_SC_XOSC
#define SPC5_AUX11_CLK                     SPC5_XOSC_CLK
#elif SPC5_CGM_AC11_SC_BITS == SPC5_CGM_SC_PLL0PHI
#define SPC5_AUX11_CLK                     SPC5_PLL0_PHI_CLK
#else
#error "invalid SPC5_CGM_AC11_SC_BITS value specified"
#endif

/* Check on SPC5_CGM_AC12_SC_BITS.*/
#if SPC5_CGM_AC12_SC_BITS == SPC5_CGM_SC_IRC
#define SPC5_AUX12_CLK                      SPC5_IRC_CLK
#elif SPC5_CGM_AC12_SC_BITS == SPC5_CGM_SC_XOSC
#define SPC5_AUX12_CLK                      SPC5_XOSC_CLK
#elif SPC5_CGM_AC12_SC_BITS == SPC5_CGM_SC_PLL0PHI
#define SPC5_AUX12_CLK                      SPC5_PLL0_PHI_CLK
#else
#error "invalid SPC5_CGM_AC12_SC_BITS value specified"
#endif

/*-----------------------------------------*
 * Dividers-related checks.                *
 *-----------------------------------------*/

/* Check on the SC divider 0 settings.*/
#if SPC5_CGM_SC_DC0_DIV_VALUE == 0
#define SPC5_CGM_SC_DC0_BITS                0
#elif (SPC5_CGM_SC_DC0_DIV_VALUE >= 1) && (SPC5_CGM_SC_DC0_DIV_VALUE <= 64)
#define SPC5_CGM_SC_DC0_BITS                (0x80000000U |                  \
                                             ((SPC5_CGM_SC_DC0_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_SC_DC0_DIV_VALUE value specified"
#endif

/* Check on the SC divider 1 settings.*/
#if SPC5_CGM_SC_DC1_DIV_VALUE == 0
#define SPC5_CGM_SC_DC1_BITS                0
#elif (SPC5_CGM_SC_DC1_DIV_VALUE >= 1) && (SPC5_CGM_SC_DC1_DIV_VALUE <= 64)
#define SPC5_CGM_SC_DC1_BITS                (0x80000000U |                  \
                                             ((SPC5_CGM_SC_DC1_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_SC_DC1_DIV_VALUE value specified"
#endif

/* Check on the SC divider 2 settings.*/
#if SPC5_CGM_SC_DC2_DIV_VALUE == 0
#define SPC5_CGM_SC_DC2_BITS                0
#elif (SPC5_CGM_SC_DC2_DIV_VALUE >= 1) && (SPC5_CGM_SC_DC2_DIV_VALUE <= 64)
#define SPC5_CGM_SC_DC2_BITS                (0x80000000U |                  \
                                             ((SPC5_CGM_SC_DC2_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_SC_DC2_DIV_VALUE value specified"
#endif

/* Check on the SC divider 3 settings.*/
#if SPC5_CGM_SC_DC3_DIV_VALUE == 0
#define SPC5_CGM_SC_DC3_BITS                0
#elif (SPC5_CGM_SC_DC3_DIV_VALUE >= 1) && (SPC5_CGM_SC_DC3_DIV_VALUE <= 64)
#define SPC5_CGM_SC_DC3_BITS                (0x80000000U |                  \
                                             ((SPC5_CGM_SC_DC3_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_SC_DC3_DIV_VALUE value specified"
#endif

#if !defined(_SPC58NE84C3_CUT1_)
/* Check on the SC divider 4 settings.*/
#if SPC5_CGM_SC_DC4_DIV_VALUE == 0
#define SPC5_CGM_SC_DC4_BITS                0
#elif (SPC5_CGM_SC_DC4_DIV_VALUE >= 1) && (SPC5_CGM_SC_DC4_DIV_VALUE <= 64)
#define SPC5_CGM_SC_DC4_BITS                (0x80000000U |                  \
                                             ((SPC5_CGM_SC_DC4_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_SC_DC4_DIV_VALUE value specified"
#endif
#endif /* !defined(_SPC58NE84C3_CUT1_) */

/* Check on the AUX0 divider 0 settings.*/
#if SPC5_CGM_AC0_DC0_DIV_VALUE == 0
#define SPC5_CGM_AC0_DC0_BITS               0
#elif (SPC5_CGM_AC0_DC0_DIV_VALUE >= 1) && (SPC5_CGM_AC0_DC0_DIV_VALUE <= 16)
#define SPC5_CGM_AC0_DC0_BITS               (0x80000000U |                  \
                                             ((SPC5_CGM_AC0_DC0_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC0_DC0_DIV_VALUE value specified"
#endif

/* Check on the AUX0 divider 1 settings.*/
#if SPC5_CGM_AC0_DC1_DIV_VALUE == 0
#define SPC5_CGM_AC0_DC1_BITS               0
#elif (SPC5_CGM_AC0_DC1_DIV_VALUE >= 1) && (SPC5_CGM_AC0_DC1_DIV_VALUE <= 128)
#define SPC5_CGM_AC0_DC1_BITS               (0x80000000U |                  \
                                             ((SPC5_CGM_AC0_DC1_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC0_DC1_DIV_VALUE value specified"
#endif

/* Check on the AUX0 divider 2 settings.*/
#if SPC5_CGM_AC0_DC2_DIV_VALUE == 0
#define SPC5_CGM_AC0_DC2_BITS               0
#elif (SPC5_CGM_AC0_DC2_DIV_VALUE >= 1) && (SPC5_CGM_AC0_DC2_DIV_VALUE <= 128)
#define SPC5_CGM_AC0_DC2_BITS               (0x80000000U |                  \
                                             ((SPC5_CGM_AC0_DC2_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC0_DC2_DIV_VALUE value specified"
#endif

/* Check on the AUX0 divider 3 settings.*/
#if SPC5_CGM_AC0_DC3_DIV_VALUE == 0
#define SPC5_CGM_AC0_DC3_BITS               0
#elif (SPC5_CGM_AC0_DC3_DIV_VALUE >= 1) && (SPC5_CGM_AC0_DC3_DIV_VALUE <= 16)
#define SPC5_CGM_AC0_DC3_BITS               (0x80000000U |                 \
                                             ((SPC5_CGM_AC0_DC3_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC0_DC3_DIV_VALUE value specified"
#endif

#if defined(_SPC58NE84C3_CUT1_)
/* Check on the AUX0 divider 4 settings.*/
#if SPC5_CGM_AC0_DC4_DIV_VALUE == 0
#define SPC5_CGM_AC0_DC4_BITS               0
#elif (SPC5_CGM_AC0_DC4_DIV_VALUE >= 1) && (SPC5_CGM_AC0_DC4_DIV_VALUE <= 16)
#define SPC5_CGM_AC0_DC4_BITS               (0x80000000U |                 \
                                             ((SPC5_CGM_AC0_DC4_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC0_DC4_DIV_VALUE value specified"
#endif
#endif /* defined(_SPC58NE84C3_CUT1_) */

/* Check on the AUX6 divider 0 settings.*/
#if SPC5_CGM_AC6_DC0_DIV_VALUE == 0
#define SPC5_CGM_AC6_DC0_BITS               0
#elif (SPC5_CGM_AC6_DC0_DIV_VALUE >= 1) && (SPC5_CGM_AC6_DC0_DIV_VALUE <= 512)
#define SPC5_CGM_AC6_DC0_BITS               (0x80000000U |                 \
                                             ((SPC5_CGM_AC6_DC0_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC6_DC0_DIV_VALUE value specified"
#endif

/* Check on the AUX7 divider 0 settings.*/
#if SPC5_CGM_AC7_DC0_DIV_VALUE == 0
#define SPC5_CGM_AC7_DC0_BITS               0
#elif (SPC5_CGM_AC7_DC0_DIV_VALUE >= 1) && (SPC5_CGM_AC7_DC0_DIV_VALUE <= 512)
#define SPC5_CGM_AC7_DC0_BITS               (0x80000000U |                 \
                                             ((SPC5_CGM_AC7_DC0_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC7_DC0_DIV_VALUE value specified"
#endif

/* Check on the AUX8 divider 0 settings.*/
#if SPC5_CGM_AC8_DC0_DIV_VALUE == 0
#define SPC5_CGM_AC8_DC0_BITS               0
#elif (SPC5_CGM_AC8_DC0_DIV_VALUE >= 1) && (SPC5_CGM_AC8_DC0_DIV_VALUE <= 64)
#define SPC5_CGM_AC8_DC0_BITS               (0x80000000U |                 \
                                             ((SPC5_CGM_AC8_DC0_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC8_DC0_DIV_VALUE value specified"
#endif

/* Check on the AUX9 divider 0 settings.*/
#if SPC5_CGM_AC9_DC0_DIV_VALUE == 0
#define SPC5_CGM_AC9_DC0_BITS               0
#elif (SPC5_CGM_AC9_DC0_DIV_VALUE >= 1) && (SPC5_CGM_AC9_DC0_DIV_VALUE <= 64)
#define SPC5_CGM_AC9_DC0_BITS               (0x80000000U |                 \
                                             ((SPC5_CGM_AC9_DC0_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC9_DC0_DIV_VALUE value specified"
#endif

/* Check on the AUX11 divider 0 settings.*/
#if SPC5_CGM_AC11_DC0_DIV_VALUE == 0
#define SPC5_CGM_AC11_DC0_BITS               0
#elif (SPC5_CGM_AC11_DC0_DIV_VALUE >= 1) && (SPC5_CGM_AC11_DC0_DIV_VALUE <= 64)
#define SPC5_CGM_AC11_DC0_BITS              (0x80000000U |                 \
                                             ((SPC5_CGM_AC11_DC0_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC11_DC0_DIV_VALUE value specified"
#endif

/* Check on the AUX12 divider 0 multiplier settings.*/
#if SPC5_CGM_AC12_DC0_DIV_FMT_VALUE == 1
#define SPC5_CGM_AC12_DC0_DIV_FMT           SPC5_CGM_DIV_FMT_DIV1
#elif SPC5_CGM_AC12_DC0_DIV_FMT_VALUE == 10
#define SPC5_CGM_AC12_DC0_DIV_FMT           SPC5_CGM_DIV_FMT_DIV10
#elif SPC5_CGM_AC12_DC0_DIV_FMT_VALUE == 100
#define SPC5_CGM_AC12_DC0_DIV_FMT           SPC5_CGM_DIV_FMT_DIV100
#elif SPC5_CGM_AC12_DC0_DIV_FMT_VALUE == 1000
#define SPC5_CGM_AC12_DC0_DIV_FMT           SPC5_CGM_DIV_FMT_DIV1000
#else
#error "invalid SPC5_CGM_AC12_DC0_DIV_FMT_VALUE value specified"
#endif

/* Check on the AUX12 divider 0 settings.*/
#if SPC5_CGM_AC12_DC0_DIV_VALUE == 0
#define SPC5_CGM_AC12_DC0_BITS               0
#elif (SPC5_CGM_AC12_DC0_DIV_VALUE >= 1) && (SPC5_CGM_AC12_DC0_DIV_VALUE <= 512)
#define SPC5_CGM_AC12_DC0_BITS               (0x80000000U |                  \
                                             SPC5_CGM_AC12_DC0_DIV_FMT |     \
                                              ((SPC5_CGM_AC12_DC0_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC12_DC0_DIV_VALUE value specified"
#endif

/* Check on the AUX12 divider 1 settings.*/
#if SPC5_CGM_AC12_DC1_DIV_VALUE == 0
#define SPC5_CGM_AC12_DC1_BITS               0
#elif (SPC5_CGM_AC12_DC1_DIV_VALUE >= 1) && (SPC5_CGM_AC12_DC1_DIV_VALUE <= 16)
#define SPC5_CGM_AC12_DC1_BITS               (0x80000000U |                 \
                                              ((SPC5_CGM_AC12_DC1_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC12_DC1_DIV_VALUE value specified"
#endif

/* Check on the AUX12 divider 2 settings.*/
#if SPC5_CGM_AC12_DC2_DIV_VALUE == 0
#define SPC5_CGM_AC12_DC2_BITS               0
#elif (SPC5_CGM_AC12_DC2_DIV_VALUE >= 1) && (SPC5_CGM_AC12_DC2_DIV_VALUE <= 16)
#define SPC5_CGM_AC12_DC2_BITS               (0x80000000U |                 \
                                              ((SPC5_CGM_AC12_DC2_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC12_DC2_DIV_VALUE value specified"
#endif

/* Check on the AUX12 divider 3 settings.*/
#if SPC5_CGM_AC12_DC3_DIV_VALUE == 0
#define SPC5_CGM_AC12_DC3_BITS               0
#elif (SPC5_CGM_AC12_DC3_DIV_VALUE >= 1) && (SPC5_CGM_AC12_DC3_DIV_VALUE <= 16)
#define SPC5_CGM_AC12_DC3_BITS               (0x80000000U |                 \
                                              ((SPC5_CGM_AC12_DC3_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC12_DC1_DIV_VALUE value specified"
#endif

/* Check on the AUX12 divider 4 settings.*/
#if SPC5_CGM_AC12_DC4_DIV_VALUE == 0
#define SPC5_CGM_AC12_DC4_BITS               0
#elif (SPC5_CGM_AC12_DC4_DIV_VALUE >= 1) && (SPC5_CGM_AC12_DC4_DIV_VALUE <= 16)
#define SPC5_CGM_AC12_DC4_BITS               (0x80000000U |                 \
                                              ((SPC5_CGM_AC12_DC4_DIV_VALUE - 1) << 16))
#else
#error "invalid SPC5_CGM_AC12_DC4_DIV_VALUE value specified"
#endif

/*-----------------------------------------*
 * Clock points calculation and check.     *
 *-----------------------------------------*/

/**
 * @brief   SPC5_PLL0_VCO_CLK clock point.
 */
#define SPC5_PLL0_VCO_CLK                                                   \
  ((SPC5_PLL0_INPUT_CLK / SPC5_PLL0_PREDIV_VALUE) * SPC5_PLL0_MFD_VALUE)

/* Check on PLL0 VCO output.*/
#if (SPC5_PLL0_VCO_CLK < SPC5_PLL0VCO_MIN) ||                               \
    (SPC5_PLL0_VCO_CLK > SPC5_PLL0VCO_MAX)
#error "SPC5_PLL0_VCO_CLK outside acceptable range (SPC5_PLL0VCO_MIN...SPC5_PLL0VCO_MAX)"
#endif

/**
 * @brief   SPC5_PLL0_PHI_CLK clock point.
 */
#define SPC5_PLL0_PHI_CLK                                                   \
  (SPC5_PLL0_VCO_CLK / SPC5_PLL0_RFDPHI_VALUE)

/* Check on SPC5_PLL0_PHI_CLK.*/
#if ((SPC5_PLL0_PHI_CLK > SPC5_PLL0_CLK_MAX) ||                             \
     (SPC5_PLL0_PHI_CLK < SPC5_PLL0_CLK_MIN)) && !SPC5_ALLOW_OVERCLOCK
#error "SPC5_PLL0_PHI_CLK outside acceptable range (SPC5_PLL0_CLK_MIN...SPC5_PLL0_CLK_MAX)"
#endif

/**
 * @brief   SPC5_PLL0_PHI1_CLK clock point.
 */
#define SPC5_PLL0_PHI1_CLK                                                  \
  (SPC5_PLL0_VCO_CLK / SPC5_PLL0_RFDPHI1_VALUE)

/* Check on SPC5_PLL0_PH1I_CLK.*/
#if ((SPC5_PLL0_PHI1_CLK > SPC5_PLL0_CLK_MAX) ||                            \
     (SPC5_PLL0_PHI1_CLK < SPC5_PLL0_CLK_MIN)) && !SPC5_ALLOW_OVERCLOCK
#error "SPC5_PLL0_PHI1_CLK outside acceptable range (SPC5_PLL0_CLK_MIN...SPC5_PLL0_CLK_MAX)"
#endif

/**
 * @brief   SPC5_PLL1_VCO_CLK clock point.
 */
#define SPC5_PLL1_VCO_CLK                                                   \
  (SPC5_PLL1_INPUT_CLK* SPC5_PLL1_MFD_VALUE)

/* Check on PLL1 VCO output.*/
#if (SPC5_PLL1_VCO_CLK < SPC5_PLL1VCO_MIN) ||                               \
    (SPC5_PLL1_VCO_CLK > SPC5_PLL1VCO_MAX)
#error "SPC5_PLL10_VCO_CLK outside acceptable range (SPC5_PLL1VCO_MIN...SPC5_PLL1VCO_MAX)"
#endif

/**
 * @brief   SPC5_PLL1_PHI_CLK clock point.
 * @note    The calculation is still wrong in the rev.5 RM.
 */
#define SPC5_PLL1_PHI_CLK                                                   \
  ((SPC5_PLL1_VCO_CLK / SPC5_PLL1_RFDPHI_VALUE) / 2)

/* Check on SPC5_PLL1_PHI_CLK.*/
#if ((SPC5_PLL1_PHI_CLK > SPC5_PLL1_CLK_MAX) ||                             \
     (SPC5_PLL1_PHI_CLK < SPC5_PLL1_CLK_MIN)) && !SPC5_ALLOW_OVERCLOCK
#error "SPC5_PLL1_PHI_CLK outside acceptable range (SPC5_PLL1_CLK_MIN...SPC5_PLL1_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC0_DC0_CLK clock point.
 */
#define SPC5_AC0_DC0_CLK                                                    \
  (SPC5_AUX0_CLK / SPC5_CGM_AC0_DC0_DIV_VALUE)

/* Check on SPC5_AC0_DC0_CLK.*/
#if SPC5_AC0_DC0_CLK > SPC5_AC0_DC0_CLK_MAX
#error "SPC5_AC0_DC0_CLK outside acceptable range (0...SPC5_AC0_DC0_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC0_DC1_CLK clock point.
 */
#define SPC5_AC0_DC1_CLK                                                    \
  (SPC5_AUX0_CLK / SPC5_CGM_AC0_DC1_DIV_VALUE)

/* Check on SPC5_AC0_DC1_CLK.*/
#if SPC5_AC0_DC1_CLK > SPC5_AC0_DC1_CLK_MAX
#error "SPC5_AC0_DC1_CLK outside acceptable range (0...SPC5_AC0_DC1_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC0_DC2_CLK clock point.
 */
#define SPC5_AC0_DC2_CLK                                                    \
  (SPC5_AUX0_CLK / SPC5_CGM_AC0_DC2_DIV_VALUE)

/* Check on SPC5_AC0_DC2_CLK.*/
#if SPC5_AC0_DC2_CLK > SPC5_AC0_DC2_CLK_MAX
#error "SPC5_AC0_DC2_CLK outside acceptable range (0...SPC5_AC0_DC2_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC0_DC3_CLK clock point.
 */
#define SPC5_AC0_DC3_CLK                                                    \
  (SPC5_AUX0_CLK / SPC5_CGM_AC0_DC3_DIV_VALUE)

/* Check on SPC5_AC0_DC3_CLK.*/
#if SPC5_AC0_DC3_CLK > SPC5_AC0_DC3_CLK_MAX
#error "SPC5_AC0_DC3_CLK outside acceptable range (0...SPC5_AC0_DC3_CLK_MAX)"
#endif

#if defined(_SPC58NE84C3_CUT1_) || defined(__DOXYGEN__)
/**
 * @brief   SPC5_AC0_DC4_CLK clock point.
 */
#define SPC5_AC0_DC4_CLK                                                    \
  (SPC5_AUX0_CLK / SPC5_CGM_AC0_DC4_DIV_VALUE)

/* Check on SPC5_AC0_DC4_CLK.*/
#if SPC5_AC0_DC4_CLK > SPC5_AC0_DC4_CLK_MAX
#error "SPC5_AC0_DC4_CLK outside acceptable range (0...SPC5_AC0_DC4_CLK_MAX)"
#endif
#endif /* defined(_SPC58NE84C3_CUT1_) */

/**
 * @brief   SPC5_AC6_DC0_CLK clock point.
 */
#define SPC5_AC6_DC0_CLK                                                    \
  (SPC5_AUX6_CLK / SPC5_CGM_AC6_DC0_DIV_VALUE)

/* Check on SPC5_AC6_DC0_CLK.*/
#if SPC5_AC6_DC0_CLK > SPC5_AC6_DC0_CLK_MAX
#error "SPC5_AC6_DC0_CLK outside acceptable range (0...SPC5_AC6_DC0_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC7_DC0_CLK clock point.
 */
#define SPC5_AC7_DC0_CLK                                                    \
  (SPC5_AUX7_CLK / SPC5_CGM_AC7_DC0_DIV_VALUE)

/* Check on SPC5_AC7_DC0_CLK.*/
#if SPC5_AC7_DC0_CLK > SPC5_AC7_DC0_CLK_MAX
#error "SPC5_AC7_DC0_CLK outside acceptable range (0...SPC5_AC7_DC0_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC8_DC0_CLK clock point.
 */
#define SPC5_AC8_DC0_CLK                                                    \
  (SPC5_AUX8_CLK / SPC5_CGM_AC8_DC0_DIV_VALUE)

/* Check on SPC5_AC8_DC0_CLK.*/
#if SPC5_AC8_DC0_CLK > SPC5_AC8_DC0_CLK_MAX
#error "SPC5_AC8_DC0_CLK outside acceptable range (0...SPC5_AC8_DC0_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC9_DC0_CLK clock point.
 */
#define SPC5_AC9_DC0_CLK                                                    \
  (SPC5_AUX9_CLK / SPC5_CGM_AC9_DC0_DIV_VALUE)

/* Check on SPC5_AC9_DC0_CLK.*/
#if SPC5_AC9_DC0_CLK > SPC5_AC9_DC0_CLK_MAX
#error "SPC5_AC9_DC0_CLK outside acceptable range (0...SPC5_AC9_DC0_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC11_DC0_CLK clock point.
 */
#define SPC5_AC11_DC0_CLK                                                   \
  (SPC5_AUX11_CLK / SPC5_CGM_AC11_DC0_DIV_VALUE)

/* Check on SPC5_AC11_DC0_CLK.*/
#if SPC5_AC11_DC0_CLK > SPC5_AC11_DC0_CLK_MAX
#error "SPC5_AC11_DC0_CLK outside acceptable range (0...SPC5_AC11_DC0_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC12_DC0_CLK clock point.
 */
#define SPC5_AC12_DC0_CLK                                                   \
  (SPC5_AUX12_CLK / (SPC5_CGM_AC12_DC0_DIV_VALUE *                          \
                     SPC5_CGM_AC12_DC0_DIV_FMT_VALUE))

/* Check on SPC5_AC12_DC0_CLK.*/
#if SPC5_AC12_DC0_CLK > SPC5_AC12_DC0_CLK_MAX
#error "SPC5_DSPI0_CLK outside acceptable range (0...SPC5_DSPI0_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC12_DC1_CLK clock point.
 */
#define SPC5_AC12_DC1_CLK                                                   \
  (SPC5_AUX12_CLK / SPC5_CGM_AC12_DC1_DIV_VALUE)

/* Check on SPC5_AC12_DC1_CLK.*/
#if SPC5_AC12_DC1_CLK > SPC5_AC12_DC1_CLK_MAX
#error "SPC5_AC12_DC1_CLK outside acceptable range (0...SPC5_AC12_DC1_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC12_DC2_CLK clock point.
 */
#define SPC5_AC12_DC2_CLK                                                   \
  (SPC5_AUX12_CLK / SPC5_CGM_AC12_DC2_DIV_VALUE)

/* Check on SPC5_AC12_DC2_CLK.*/
#if SPC5_AC12_DC2_CLK > SPC5_AC12_DC2_CLK_MAX
#error "SPC5_AC12_DC2_CLK outside acceptable range (0...SPC5_AC12_DC2_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC12_DC3_CLK clock point.
 */
#define SPC5_AC12_DC3_CLK                                                   \
  (SPC5_AUX12_CLK / SPC5_CGM_AC12_DC3_DIV_VALUE)

/* Check on SPC5_AC12_DC3_CLK.*/
#if SPC5_AC12_DC3_CLK > SPC5_AC12_DC3_CLK_MAX
#error "SPC5_AC12_DC3_CLK outside acceptable range (0...SPC5_AC12_DC3_CLK_MAX)"
#endif

/**
 * @brief   SPC5_AC12_DC4_CLK clock point.
 */
#define SPC5_AC12_DC4_CLK                                                   \
  (SPC5_AUX12_CLK / SPC5_CGM_AC12_DC4_DIV_VALUE)

/* Check on SPC5_AC12_DC4_CLK.*/
#if SPC5_AC12_DC4_CLK > SPC5_AC12_DC4_CLK_MAX
#error "SPC5_AC12_DC4_CLK outside acceptable range (0...SPC5_AC12_DC4_CLK_MAX)"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type representing a system clock frequency.
 */
typedef uint32_t halclock_t;

/**
 * @brief   Type of the realtime free counter value.
 */
typedef uint32_t halrtcnt_t;

/**
 * @brief   Run modes.
 */
typedef enum {
  SPC5_RUNMODE_SAFE  = 2,
  SPC5_RUNMODE_DRUN  = 3,
  SPC5_RUNMODE_RUN0  = 4,
  SPC5_RUNMODE_RUN1  = 5,
  SPC5_RUNMODE_RUN2  = 6,
  SPC5_RUNMODE_RUN3  = 7,
  SPC5_RUNMODE_HALT0 = 8,
  SPC5_RUNMODE_STOP0 = 10
} spc5_runmode_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Returns the current value of the system free running counter.
 * @note    This service is implemented by returning the content of the
 *          TBL register.
 *
 * @return              The value of the system free running counter of
 *                      type halrtcnt_t.
 *
 * @notapi
 */
static inline
halrtcnt_t hal_lld_get_counter_value(void) {

  return  (halrtcnt_t)0xFFFFFFFFU - (halrtcnt_t)PIT_0.CH[1].CVAL.R;
}

/**
 * @brief   Realtime counter frequency.
 *
 * @return              The realtime counter frequency of type halclock_t.
 *
 * @notapi
 */
#define hal_lld_get_counter_frequency() SPC5_AC12_DC4_CLK

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/*#include "spc5_edma.h"*/

#ifdef __cplusplus
extern "C" {
#endif
  void hal_lld_init(void);
  void spc_clock_init(void);
  bool halSPCSetRunMode(spc5_runmode_t mode);
  void halSPCSetPeripheralClockMode(uint32_t n, uint32_t pctl);
#if !SPC5_NO_INIT
  uint32_t halSPCGetSystemClock(void);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_LLD_H */

/** @} */
