/**
 ******************************************************************************
 * @file       pios_chibios_transition_priv.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @brief ChibiOS transition header, mapping ISR names.
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

#ifndef PIOS_CHIBIOS_TRANSITION_PRIV_H_
#define PIOS_CHIBIOS_TRANSITION_PRIV_H_

#define WWDG_IRQHandler                 Vector40 // Window WatchDog
#define PVD_IRQHandler                  Vector44 // PVD through EXTI Line detection
#define TAMPER_IRQHandler               Vector48 // Tamper and TimeStamps through the EXTI line
#define RTC_IRQHandler                  Vector4C // RTC Wakeup through the EXTI line
#define FLASH_IRQHandler                Vector50 // FLASH
#define RCC_IRQHandler                  Vector54 // RCC
#define EXTI0_IRQHandler                Vector58 // EXTI Line0
#define EXTI1_IRQHandler                Vector5C // EXTI Line1
#define EXTI2_IRQHandler                Vector60 // EXTI Line2
#define EXTI3_IRQHandler                Vector64 // EXTI Line3
#define EXTI4_IRQHandler                Vector68 // EXTI Line4
#define DMA1_Channel1_IRQHandler        Vector6C // DMA1 Channel 1
#define DMA1_Channel2_IRQHandler        Vector70 // DMA1 Channel 2
#define DMA1_Channel3_IRQHandler        Vector74 // DMA1 Channel 3
#define DMA1_Channel4_IRQHandler        Vector78 // DMA1 Channel 4
#define DMA1_Channel5_IRQHandler        Vector7C // DMA1 Channel 5
#define DMA1_Channel6_IRQHandler        Vector80 // DMA1 Channel 6
#define DMA1_Channel7_IRQHandler        Vector84 // DMA1 Channel 7
#define ADC1_2_IRQHandler               Vector88 // ADC1 and ADC2
#define USB_HP_CAN1_TX_IRQHandler       Vector8C // CAN1 TX
#define USB_LP_CAN1_RX0_IRQHandler      Vector90 // CAN1 RX0
#define CAN1_RX1_IRQHandler             Vector94 // CAN1 RX1
#define CAN1_SCE_IRQHandler             Vector98 // CAN1 SCE
#define EXTI9_5_IRQHandler              Vector9C // External Line[9:5]s
#define TIM1_BRK_IRQHandler             VectorA0 // TIM1 Break
#define TIM1_UP_IRQHandler              VectorA4 // TIM1 Update
#define TIM1_TRG_COM_IRQHandler         VectorA8 // TIM1 Trigger and Commutation and TIM17
#define TIM1_CC_IRQHandler              VectorAC // TIM1 Capture Compare
#define TIM2_IRQHandler                 VectorB0 // TIM2
#define TIM3_IRQHandler                 VectorB4 // TIM3
#define TIM4_IRQHandler                 VectorB8 // TIM4
#define I2C1_EV_IRQHandler              VectorBC // I2C1 Event
#define I2C1_ER_IRQHandler              VectorC0 // I2C1 Error
#define I2C2_EV_IRQHandler              VectorC4 // I2C2 Event
#define I2C2_ER_IRQHandler              VectorC8 // I2C2 Error
#define SPI1_IRQHandler                 VectorCC // SPI1
#define SPI2_IRQHandler                 VectorD0 // SPI2
#define USART1_IRQHandler               VectorD4 // USART1
#define USART2_IRQHandler               VectorD8 // USART2
#define USART3_IRQHandler               VectorDC // USART3
#define EXTI15_10_IRQHandler            VectorE0 // External Line[15:10]s
#define RTCAlarm_IRQHandler             VectorE4 // RTC Alarm (A and B) through EXTI Line
#define OTG_FS_WKUP_IRQHandler          VectorE8 // USB FS Wakeup through EXTI line
#define TIM5_IRQHandler                 Vector108
#define SPI3_IRQHandler                 Vector10C
#define UART4_IRQHandler                Vector110
#define UART5_IRQHandler                Vector114
#define TIM6_IRQHandler                 Vector118
#define TIM7_IRQHandler                 Vector11C
#define DMA2_Channel1_IRQHandler        Vector120
#define DMA2_Channel2_IRQHandler        Vector124
#define DMA2_Channel3_IRQHandler        Vector128
#define DMA2_Channel4_IRQHandler        Vector12C
#define DMA2_Channel5_IRQHandler        Vector130
#define ETH_IRQHandler                  Vector134
#define ETH_WKUP_IRQHandler             Vector138
#define CAN2_TX_IRQHandler              Vector13C
#define CAN2_RX0_IRQHandler             Vector140
#define CAN2_RX1_IRQHandler             Vector144
#define CAN2_SCE_IRQHandler             Vector148
#define OTG_FS_IRQHandler               Vector14C

#endif /* PIOS_CHIBIOS_TRANSITION_PRIV_H_ */
