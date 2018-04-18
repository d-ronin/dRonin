/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_INTERNAL_ADC Internal ADC Functions
 * @{
 *
 * @file       pios_internal_adc.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org Copyright (C) 2013.
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      ADC functions header.
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

#ifndef PIOS_INTERNAL_ADC_H
#define PIOS_INTERNAL_ADC_H

#if defined(STM32F30X)
/* TODO: transition other targets to work this way. */
void PIOS_INTERNAL_ADC_DMA_Handler(uintptr_t internal_adc_id);
#else
void PIOS_INTERNAL_ADC_DMA_Handler();
#endif

#endif /* PIOS_INTERNAL_ADC_H */

/**
  * @}
  * @}
  */
