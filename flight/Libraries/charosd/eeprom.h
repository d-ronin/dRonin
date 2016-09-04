/*
 * This file is part of MultiOSD <https://github.com/UncleRus/MultiOSD>
 *
 * MultiOSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef EEPROM_H_
#define EEPROM_H_

#define _eeprom_byte(ADDR)  ((uint8_t *)  (ADDR))
#define _eeprom_word(ADDR)  ((uint16_t *) (ADDR))
#define _eeprom_dword(ADDR) ((uint32_t *) (ADDR))
#define _eeprom_float(ADDR) ((float *)    (ADDR))
#define _eeprom_str(ADDR)   ((char *)     (ADDR))

// EEPROM sections offsets
#define ADC_EEPROM_OFFSET            0x05
#define MAX7456_EEPROM_OFFSET        0x0a
#define ADC_BATTERY_EEPROM_OFFSET    0x0c
#define ADC_RSSI_EEPROM_OFFSET       0x20
#define UAVTALK_EEPROM_OFFSET        0x28
#define MAVLINK_EEPROM_OFFSET        0x28
#define UBX_EEPROM_OFFSET            0x28
#define MSP_EEPROM_OFFSET            0x28
#define TELEMETRY_EEPROM_OFFSET      0x40
#define OSD_EEPROM_OFFSET            0x70
#define OSD_SCREENS_EEPROM_OFFSET    (OSD_EEPROM_OFFSET + 0x10)

#endif /* EEPROM_H_ */
