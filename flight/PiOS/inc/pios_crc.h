/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_CRC CRC Functions
 * @{
 *
 * @file       pios_crc.h  
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      CRC functions header.
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

#include <stdint.h>

uint8_t PIOS_CRC_updateByte(uint8_t crc, const uint8_t data);
uint8_t PIOS_CRC_updateCRC(uint8_t crc, const uint8_t* data, int32_t length);
uint8_t PIOS_CRC_updateCRC_TBS(uint8_t crc, const uint8_t *data, int32_t length);

uint16_t PIOS_CRC16_updateByte(uint16_t crc, const uint8_t data);
uint16_t PIOS_CRC16_updateCRC(uint16_t crc, const uint8_t* data, int32_t length);
uint16_t PIOS_CRC16_CCITT_updateCRC(uint16_t crc, const uint8_t *data, uint32_t data_len);

uint32_t PIOS_CRC32_updateByte(uint32_t crc, const uint8_t data);
uint32_t PIOS_CRC32_updateCRC(uint32_t crc, const uint8_t* data, int32_t length);
