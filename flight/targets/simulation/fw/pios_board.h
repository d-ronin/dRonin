/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Revolution OpenPilot revolution support files
 * @{
 *
 * @file       pios_board_sim.c 
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      Simulation configuration settings
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
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef PIOS_BOARD_H
#define PIOS_BOARD_H


// PIOS_ADC
#define PIOS_ADC_SUB_DRIVER_MAX_INSTANCES	3

//------------------------
// PIOS_LED
//------------------------
#define PIOS_LED_HEARTBEAT 0
#define PIOS_LED_ALARM 1
#define PIOS_ANNUNC_NUM 2

//-------------------------
// COM
//
// See also pios_board_posix.c
//-------------------------
//#define PIOS_USART_TX_BUFFER_SIZE		256
#define PIOS_COM_BUFFER_SIZE 1024
#define PIOS_COM_MAX_DEVS 255
#define PIOS_UDP_RX_BUFFER_SIZE		PIOS_COM_BUFFER_SIZE
#define PIOS_TCP_RX_BUFFER_SIZE		PIOS_COM_BUFFER_SIZE
#define PIOS_SERIAL_RX_BUFFER_SIZE		PIOS_COM_BUFFER_SIZE

extern uintptr_t pios_com_telem_rf_id;
extern uintptr_t pios_com_telem_usb_id;
extern uintptr_t pios_com_gps_id;
extern uintptr_t pios_com_debug_id;
extern uintptr_t pios_com_openlog_id;
extern uintptr_t pios_com_lighttelemetry_id;
extern uintptr_t pios_com_msp_id;

#define PIOS_COM_TELEM_RF                       (pios_com_telem_rf_id)
#define PIOS_COM_TELEM_USB                      (pios_com_telem_usb_id)
#define PIOS_COM_GPS                            (pios_com_gps_id)
#define PIOS_COM_MSP                            (pios_com_msp_id)
#define PIOS_COM_DEBUG                          (pios_com_debug_id)
#define PIOS_COM_OPENLOG                        (pios_com_openlog_id)
#define PIOS_COM_LIGHTTELEMETRY                 (pios_com_lighttelemetry_id)

#define PIOS_GCSRCVR_TIMEOUT_MS 200

#define DEBUG_PRINTF(level, ...) do { (void) (level); fprintf(stderr, __VA_ARGS__); } while (0)

#endif /* PIOS_BOARD_H */
