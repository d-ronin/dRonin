/**
 ******************************************************************************
 *
 * @file       flyingpio/board-info/pios_board.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup FlyingPIO FlyingPi IO Hat
 * @{
 * @brief      Board header file for FlyingPIO IO Expander
 * @see        The GNU Public License (GPL) Version 3
 *
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


#ifndef _FLYINGPIO_PIOS_BOARD_H
#define _FLYINGPIO_PIOS_BOARD_H

#include <stdbool.h>

//------------------------
// PIOS_LED
//------------------------
#define PIOS_LED_HEARTBEAT				0

//------------------------
// PIOS_WDG
//------------------------
#define PIOS_WATCHDOG_TIMEOUT			125

//------------------------
// PIOS_I2C
// See also pios_board.c
//------------------------
#define PIOS_I2C_MAX_DEVS				1

//-------------------------
// PIOS_COM
//
// See also pios_board.c
//-------------------------
extern uintptr_t pios_com_telem_serial_id;
extern uintptr_t pios_com_gps_id;
extern uintptr_t pios_com_bridge_id;
extern uintptr_t pios_com_mavlink_id;

#define PIOS_COM_GPS                    (pios_com_gps_id)
#define PIOS_COM_TELEM_RF               (pios_com_telem_serial_id)
#define PIOS_COM_BRIDGE                 (pios_com_bridge_id)
#define PIOS_COM_MAVLINK                (pios_com_mavlink_id)

//------------------------
// TELEMETRY
//------------------------
#define TELEM_QUEUE_SIZE				20

//-------------------------
// System Settings
//-------------------------

#define PIOS_SYSCLK				40000000
#define PIOS_PERIPHERAL_APB1_CLOCK					(PIOS_SYSCLK)


//-------------------------
// Interrupt Priorities
//-------------------------
#define PIOS_IRQ_PRIO_LOW				12              // lower than RTOS
#define PIOS_IRQ_PRIO_MID				8               // higher than RTOS
#define PIOS_IRQ_PRIO_HIGH				5               // for SPI, ADC, I2C etc...
#define PIOS_IRQ_PRIO_HIGHEST			4               // for USART etc...
#define PIOS_IRQ_PRIO_EXTREME			0 		// for I2C

//------------------------
// PIOS_RCVR
// See also pios_board.c
//------------------------
#define PIOS_RCVR_MAX_CHANNELS			12
#define PIOS_GCSRCVR_TIMEOUT_MS			100

//-------------------------
// Receiver PPM input
//-------------------------
#define PIOS_PPM_NUM_INPUTS				8

//-------------------------
// Receiver PWM input
//-------------------------
#define PIOS_PWM_NUM_INPUTS				8

//-------------------------
// Receiver DSM input
//-------------------------
#define PIOS_DSM_NUM_INPUTS				12

//-------------------------
// Reciever HSUM input
//-------------------------
#define PIOS_HSUM_MAX_DEVS				2
#define PIOS_HSUM_NUM_INPUTS			32

//-------------------------
// Receiver S.Bus input
//-------------------------
#define PIOS_SBUS_NUM_INPUTS			(16+2)

//-------------------------
// Servo outputs
//-------------------------
#define PIOS_SERVO_UPDATE_HZ			50
#define PIOS_SERVOS_INITIAL_POSITION	0 /* dont want to start motors, have no pulse till settings loaded */

//--------------------------
// Timer controller settings
//--------------------------
#define PIOS_TIM_MAX_DEVS				3

//-------------------------
// ADC
// ADC0 : PA4 ADC_IN4
// ADC1 : PA5 ADC_IN5
// ADC2 : PA1 ADC_IN1
// ADC3 : PB1 ADC_IN9
//-------------------------

#if defined(PIOS_INCLUDE_ADC)
extern uintptr_t pios_internal_adc_id;
#define pios_internal_adc_id					(pios_internal_adc_id)
#endif
#define PIOS_ADC_SUB_DRIVER_MAX_INSTANCES		1

#define VREF_PLUS								3.3

#endif /* _FLYINGPIO_PIOS_BOARD_H */

/**
 * @}
 * @}
 */
