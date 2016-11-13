/**
 ******************************************************************************
 * @file       playuavosd/board-info/pios_board.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup PlayUavOsd
 * @{
 * @brief Board support for PlayUAVOSD board
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include <stdbool.h>

//------------------------
// Timers and Channels Used
//------------------------
/*
Timer | Channel 1 | Channel 2 | Channel 3 | Channel 4
------+-----------+-----------+-----------+----------
TIM1  |           |           |           |
TIM2  | --------------- PIOS_DELAY -----------------
TIM3  |           |           |           |
TIM4  |           |           |           |
TIM5  |           |           |           |
TIM6  |           |           |           |
TIM7  |           |           |           |
TIM8  |           |           |           |
------+-----------+-----------+-----------+----------
*/

//------------------------
// DMA Channels Used
//------------------------
/* Channel 1  -                                 */
/* Channel 2  -                                 */
/* Channel 3  -                                 */
/* Channel 4  -                                 */
/* Channel 5  -                                 */
/* Channel 6  -                                 */
/* Channel 7  -                                 */
/* Channel 8  -                                 */
/* Channel 9  -                                 */
/* Channel 10 -                                 */
/* Channel 11 -                                 */
/* Channel 12 -                                 */

//------------------------
// BOOTLOADER_SETTINGS
//------------------------
#define BOARD_READABLE	true
#define BOARD_WRITABLE	true
#define MAX_DEL_RETRYS	3


//------------------------
// PIOS_LED
//------------------------
#define PIOS_LED_HEARTBEAT	0
#define PIOS_LED_ALARM		1

//------------------------
// PIOS_WDG
//------------------------
#define PIOS_WATCHDOG_TIMEOUT    250
#define PIOS_WDG_REGISTER        RTC_BKP_DR4

//-------------------------
// PIOS_COM
//
// See also pios_board.c
//-------------------------
extern uintptr_t pios_com_telem_serial_id;
extern uintptr_t pios_com_gps_id;
extern uintptr_t pios_com_telem_usb_id;
extern uintptr_t pios_com_bridge_id;
extern uintptr_t pios_com_vcp_id;
extern uintptr_t pios_com_mavlink_id;
extern uintptr_t pios_com_hott_id;
extern uintptr_t pios_com_frsky_sensor_hub_id;
extern uintptr_t pios_com_lighttelemetry_id;
extern uintptr_t pios_com_debug_id;
extern uintptr_t pios_com_frsky_sport_id;
extern uintptr_t pios_com_openlog_logging_id;

#define PIOS_COM_GPS                    (pios_com_gps_id)
#define PIOS_COM_TELEM_USB              (pios_com_telem_usb_id)
#define PIOS_COM_TELEM_RF               (pios_com_telem_serial_id)
#define PIOS_COM_BRIDGE                 (pios_com_bridge_id)
#define PIOS_COM_VCP                    (pios_com_vcp_id)
#define PIOS_COM_MAVLINK                (pios_com_mavlink_id)
#define PIOS_COM_HOTT                   (pios_com_hott_id)
#define PIOS_COM_FRSKY_SENSOR_HUB       (pios_com_frsky_sensor_hub_id)
#define PIOS_COM_LIGHTTELEMETRY         (pios_com_lighttelemetry_id)
#define PIOS_COM_DEBUG                  (pios_com_debug_id)

#define DEBUG_LEVEL 0
#define DEBUG_PRINTF(level, ...) {if(level <= DEBUG_LEVEL && pios_com_debug_id > 0) { PIOS_COM_SendFormattedStringNonBlocking(pios_com_debug_id, __VA_ARGS__); }}

//------------------------
// TELEMETRY 
//------------------------
#define TELEM_QUEUE_SIZE         80
#define PIOS_TELEM_STACK_SIZE    624

#define PIOS_SYSCLK										168000000
//	Peripherals that belongs to APB1 are:
//	DAC			|PWR				|CAN1,2
//	I2C1,2,3		|UART4,5			|USART3,2
//	I2S3Ext		|SPI3/I2S3		|SPI2/I2S2
//	I2S2Ext		|IWDG				|WWDG
//	RTC/BKP reg	
// TIM2,3,4,5,6,7,12,13,14

// Calculated as SYSCLK / APBPresc * (APBPre == 1 ? 1 : 2)   
// Default APB1 Prescaler = 4 
#define PIOS_PERIPHERAL_APB1_CLOCK					(PIOS_SYSCLK / 4)

//	Peripherals belonging to APB2
//	SDIO			|EXTI				|SYSCFG			|SPI1
//	ADC1,2,3				
//	USART1,6
//	TIM1,8,9,10,11
//
// Default APB2 Prescaler = 2
//
#define PIOS_PERIPHERAL_APB2_CLOCK					(PIOS_SYSCLK / 2)


//-------------------------
// Interrupt Priorities
//-------------------------
#define PIOS_IRQ_PRIO_LOW                       12              // lower than RTOS
#define PIOS_IRQ_PRIO_MID                       8               // higher than RTOS
#define PIOS_IRQ_PRIO_HIGH                      5               // for SPI, ADC, I2C etc...
#define PIOS_IRQ_PRIO_HIGHEST                   4               // for USART etc...

//--------------------------
// Timer controller settings
//--------------------------
#define PIOS_TIM_MAX_DEVS			6

//-------------------------
// USB
//-------------------------
#define PIOS_USB_ENABLED                        1 /* Should remove all references to this */

/**
 * @}
 * @}
 */
