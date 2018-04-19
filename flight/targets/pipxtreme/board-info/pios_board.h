/**
 ******************************************************************************
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup PipX PipXtreme Radio
 * @{
 *
 * @file       pipxtreme/board-info/pios_board.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      Board header file for PipXtreme
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

#ifndef STM32103CB_PIPXTREME_H_
#define STM32103CB_PIPXTREME_H_

#define TAULINK_VERSION_STICK 0x01
#define TAULINK_VERSION_MODULE 0x02

//------------------------
// Timers and Channels Used
//------------------------
/*
Timer | Channel 1 | Channel 2 | Channel 3 | Channel 4
------+-----------+-----------+-----------+----------
TIM1  |  Servo 4  |           |           |
TIM2  |  RC In 5  |  RC In 6  |  Servo 6  |
TIM3  |  Servo 5  |  RC In 2  |  RC In 3  |  RC In 4
TIM4  |  RC In 1  |  Servo 3  |  Servo 2  |  Servo 1
------+-----------+-----------+-----------+----------
*/

//------------------------
// DMA Channels Used
//------------------------
/* Channel 1  - 				*/
/* Channel 2  - 				*/
/* Channel 3  - 				*/
/* Channel 4  - 				*/
/* Channel 5  - 				*/
/* Channel 6  - 				*/
/* Channel 7  - 				*/
/* Channel 8  - 				*/
/* Channel 9  - 				*/
/* Channel 10 - 				*/
/* Channel 11 - 				*/
/* Channel 12 - 				*/


//------------------------
// BOOTLOADER_SETTINGS
//------------------------
#define BOARD_READABLE	true
#define BOARD_WRITABLE	true
#define MAX_DEL_RETRYS	3


//------------------------
// WATCHDOG_SETTINGS
//------------------------
#define PIOS_WATCHDOG_TIMEOUT    500
#define PIOS_WDG_REGISTER        BKP_DR4
#define PIOS_WDG_TELEMETRY       0x0100
#define PIOS_WDG_RADIORX         0x0200
#define PIOS_WDG_RFM22B          0x0400
#define PIOS_WDG_SYSTEM          0x0800

//------------------------
// PIOS_LED
//------------------------
#define PIOS_LED_USB	0
#define PIOS_LED_LINK	1
#define PIOS_LED_RX	2
#define PIOS_LED_TX	3
#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
#define PIOS_LED_D1     4
#define PIOS_LED_D2     5
#define PIOS_LED_D3     6
#define PIOS_LED_D4     7
#endif

#define PIOS_LED_HEARTBEAT PIOS_LED_USB
#define PIOS_LED_ALARM PIOS_LED_TX

#define USB_LED_ON					PIOS_ANNUNC_On(PIOS_LED_USB)
#define USB_LED_OFF					PIOS_ANNUNC_Off(PIOS_LED_USB)
#define USB_LED_TOGGLE					PIOS_ANNUNC_Toggle(PIOS_LED_USB)

#define LINK_LED_ON					PIOS_ANNUNC_On(PIOS_LED_LINK)
#define LINK_LED_OFF					PIOS_ANNUNC_Off(PIOS_LED_LINK)
#define LINK_LED_TOGGLE					PIOS_ANNUNC_Toggle(PIOS_LED_LINK)

#define RX_LED_ON					PIOS_ANNUNC_On(PIOS_LED_RX)
#define RX_LED_OFF					PIOS_ANNUNC_Off(PIOS_LED_RX)
#define RX_LED_TOGGLE					PIOS_ANNUNC_Toggle(PIOS_LED_RX)

#define TX_LED_ON					PIOS_ANNUNC_On(PIOS_LED_TX)
#define TX_LED_OFF					PIOS_ANNUNC_Off(PIOS_LED_TX)
#define TX_LED_TOGGLE					PIOS_ANNUNC_Toggle(PIOS_LED_TX)

#ifdef PIOS_RFM22B_DEBUG_ON_TELEM
#define D1_LED_ON					PIOS_ANNUNC_On(PIOS_LED_D1)
#define D1_LED_OFF					PIOS_ANNUNC_Off(PIOS_LED_D1)
#define D1_LED_TOGGLE					PIOS_ANNUNC_Toggle(PIOS_LED_D1)

#define D2_LED_ON					PIOS_ANNUNC_On(PIOS_LED_D2)
#define D2_LED_OFF					PIOS_ANNUNC_Off(PIOS_LED_D2)
#define D2_LED_TOGGLE					PIOS_ANNUNC_Toggle(PIOS_LED_D2)

#define D3_LED_ON					PIOS_ANNUNC_On(PIOS_LED_D3)
#define D3_LED_OFF					PIOS_ANNUNC_Off(PIOS_LED_D3)
#define D3_LED_TOGGLE					PIOS_ANNUNC_Toggle(PIOS_LED_D3)

#define D4_LED_ON					PIOS_ANNUNC_On(PIOS_LED_D4)
#define D4_LED_OFF					PIOS_ANNUNC_Off(PIOS_LED_D4)
#define D4_LED_TOGGLE					PIOS_ANNUNC_Toggle(PIOS_LED_D4)
#endif

//-------------------------
// System Settings
//-------------------------
#define PIOS_SYSCLK			72000000
#define PIOS_PERIPHERAL_APB1_CLOCK					(PIOS_SYSCLK / 2)
#define PIOS_PERIPHERAL_APB2_CLOCK					(PIOS_SYSCLK / 1)

//-------------------------
// Interrupt Priorities
//-------------------------
#define PIOS_IRQ_PRIO_LOW			12		// lower than RTOS
#define PIOS_IRQ_PRIO_MID			8		// higher than RTOS
#define PIOS_IRQ_PRIO_HIGH			5		// for SPI, ADC, I2C etc...
#define PIOS_IRQ_PRIO_HIGHEST			4 		// for USART etc...

//-------------------------
// PIOS_COM
//
// See also pios_board.c
//-------------------------
extern uintptr_t pios_com_telem_usb_id;
extern uintptr_t pios_com_vcp_id;
extern uintptr_t pios_com_radio_id;
extern uintptr_t pios_com_telem_serial_id;
extern uintptr_t pios_com_bridge_id;
extern uintptr_t pios_com_rf_id;
extern uintptr_t pios_com_telem_uart_telem_id;
extern uintptr_t pios_ppm_rcvr_id;
extern uintptr_t pios_com_debug_id;
extern uintptr_t pios_com_frsky_sport_id;
#define PIOS_COM_TELEM_USB         (pios_com_telem_usb_id)
#define PIOS_COM_VCP               (pios_com_vcp_id)
#define PIOS_COM_TELEMETRY         (pios_com_telem_serial_id)
#define PIOS_COM_BRIDGE            (pios_com_bridge_id)
#define PIOS_COM_RADIOBRIDGE       (pios_com_rf_id)
#define PIOS_PPM_RECEIVER          (pios_ppm_rcvr_id)
#define PIOS_COM_DEBUG             (pios_com_debug_id)
#define PIOS_COM_FRSKY_SPORT       (pios_com_frsky_sport_id)

#define DEBUG_LEVEL 0
#define DEBUG_PRINTF(level, ...) {if(level <= DEBUG_LEVEL && PIOS_COM_DEBUG > 0) { PIOS_COM_SendFormattedStringNonBlocking(PIOS_COM_DEBUG, __VA_ARGS__); }}

#define RFM22_DEBUG 1

//-------------------------
// ADC
// None
//-------------------------
//#define PIOS_ADC_OVERSAMPLING_RATE		1
#define PIOS_ADC_MAX_OVERSAMPLING               36

#define VREF_PLUS				3.3

//------------------------
// PIOS_RCVR
// See also pios_board.c
//------------------------
#define PIOS_RCVR_MAX_CHANNELS      12

//-------------------------
// Receiver PPM input
//-------------------------
#define PIOS_PPM_NUM_INPUTS   8
#define PIOS_SBUS_NUM_INPUTS         (16+2)
//-------------------------
// Servo outputs
//-------------------------
#define PIOS_SERVO_UPDATE_HZ                    50
#define PIOS_SERVOS_INITIAL_POSITION            0 /* dont want to start motors, have no pulse till settings loaded */

//--------------------------
// Timer controller settings
//--------------------------
#define PIOS_TIM_MAX_DEVS			3

//-------------------------
// USB
//-------------------------
#define PIOS_USB_ENABLED                        1
#define PIOS_USB_DETECT_GPIO_PORT               GPIOC
#define PIOS_USB_DETECT_GPIO_PIN                GPIO_Pin_15
#define PIOS_USB_DETECT_EXTI_LINE               EXTI_Line15

//-------------------------
// Flash EEPROM Emulation
//-------------------------

#define PIOS_FLASH_SIZE 0x20000
#define PIOS_FLASH_EEPROM_START_ADDR 0x08000000
#define PIOS_FLASH_PAGE_SIZE 1024
#define PIOS_FLASH_EEPROM_ADDR (PIOS_FLASH_EEPROM_START_ADDR + PIOS_FLASH_SIZE - PIOS_FLASH_PAGE_SIZE)
#define PIOS_FLASH_EEPROM_LEN PIOS_FLASH_PAGE_SIZE

#endif /* STM32103CB_PIPXTREME_H_ */

/**
 * @}
 * @}
 */
