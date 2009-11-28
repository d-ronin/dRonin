
/**
 * Project: OpenPilot
 *    
 * @author The OpenPilot Team, http://www.openpilot.org, Copyright (C) 2009.
 *    
 * @see The GNU Public License (GPL)
 */
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


#ifndef PIOS_H
#define PIOS_H


/* C Header Files */
#include <stdlib.h>
#include <stdio.h>

/* STM32 Std Perf Lib */
#include <stm32f10x.h>
#include <stm32f10x_conf.h>

/* FatFS Functions */
#include <ff.h>
#include <diskio.h>

/* Include Flyingfox Hardware Header Files */
#include <pios_board.h>
#include <pios_sys.h>
#include <pios_settings.h>
#include <pios_led.h>
#include <pios_uart.h>

//#include <pios_irq.h>
//#include <pios_spi.h>
//#include <pios_uart.h>

/* More added here as they get written */


#endif /* PIOS_H */
