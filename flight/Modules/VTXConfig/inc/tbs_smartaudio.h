/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{ 
 * @addtogroup VTXConfig Module
 * @{ 
 *
 * @file       tbs_smartaudio.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
 * @brief      This module configures the video transmitter
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */


#ifndef TBS_SMARTAUDIO_H
#define TBS_SMARTAUDIO_H

#include "vtxinfo.h"

int32_t tbsvtx_get_state(uintptr_t usart_id, VTXInfoData *info);
int32_t tbsvtx_set_freq(uintptr_t usart_id, uint16_t frequency);
int32_t tbsvtx_set_power(uintptr_t usart_id, uint16_t power);

#endif /* defined(TBS_SMARTAUDIO_H) */


