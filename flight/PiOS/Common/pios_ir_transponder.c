/**
 ******************************************************************************
 * @file       pios_ir_transponder.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @brief      Generate packets for various infrared lap timin protocols
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

#include <pios.h>

#if defined(PIOS_INCLUDE_IR_TRANSPONDER)
#include <misc_math.h>

/**
 * @brief Invert bits, needed for some protocols
 */
static void ir_invert_bits(uint8_t * data, uint8_t data_len)
{
	for (int i=0; i<data_len; i++) {
		data[i] = ~data[i];
	}
}

/**
 * @brief Generate an I-Lap packet
 * @note  Credit goes to Gareth Owenson for reverse engineering the protocol
 *        see http://blog.owenson.me/reversing-ilap-race-transponders
 */
void pios_ir_generate_ilap_packet(uint32_t ilap_id, uint8_t * data, uint8_t data_len)
{
	if (data_len < 6)
		return;

	uint8_t crc_data[4] = {0, 0, 0, 0};

	uint8_t digit;

	// BCD encoded in reverse
	uint32_t decimal = 10000000;

	for (int pos=3; pos>=0; pos--) {
		digit = MIN(9, (ilap_id / decimal));
		ilap_id -= digit * decimal;
		crc_data[pos] |= (digit << 4) & 0xF0;
		decimal /= 10;
		digit = MIN(9, (ilap_id / decimal));
		ilap_id -= digit * decimal;
		crc_data[pos] |= digit & 0x0F;
		decimal /= 10;
	}
	crc_data[3] |= 0xf0;

	uint16_t crc = PIOS_CRC16_CCITT_updateCRC(0x00, crc_data, 4);

	data[0] = crc_data[3];
	data[1] = (crc & 0xFF00) >> 8;
	data[2] = crc_data[2];
	data[3] = crc_data[1];
	data[4] = crc_data[0];
	data[5] = (crc & 0x00FF);

	ir_invert_bits(data, 6);
}

/**
 * @brief Generate a trackmate packet
 * @note Credit goes to Mitch Martin for reverse engineering the protocol
 *       see http://getglitched.com/?p=1288
 */
void pios_ir_generate_trackmate_packet(uint16_t trackmate_id, uint8_t * data, uint8_t data_len)
{
	if (data_len < 4)
		return;

	uint8_t crc_data[2];
	crc_data[1] = trackmate_id & 0x00FF;
	crc_data[0] = (trackmate_id & 0xFF00) >> 8;

	uint16_t crc = PIOS_CRC16_CCITT_updateCRC(0xFB1A, crc_data, 2);

	data[0] = (trackmate_id >> 8) & 0xFF;
	data[1] = trackmate_id & 0xFF;
	data[2] = (crc >> 8) & 0xFF;
	data[3] = crc & 0xFF;
}

#endif /* defined(PIOS_INCLUDE_IR_TRANSPONDER) */
