/* Imported from MultiOSD (https://github.com/UncleRus/MultiOSD/)
 * Altered for use on STM32 Flight controllers by dRonin
 * Copyright (C) dRonin 2016
 */

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
#ifndef CONFIG_H_
#define CONFIG_H_

/*
 * OSD config
 */
#define OSD_SCREEN_PANELS 24                              // (OSD_SCREEN_PANELS * 3) bytes in SRAM
#define OSD_MAX_SCREENS 8

#define OSD_DEFAULT_SCREENS 8                             // each screen will consume (OSD_SCREEN_PANELS * 3) bytes in EEPROM
#define OSD_DEFAULT_SWITCH_MODE OSD_SWITCH_MODE_TOGGLE    // OSD_SWITCH_MODE_SELECTOR or OSD_SWITCH_MODE_TOGGLE
#define OSD_DEFAULT_SWITCH_CHANNEL 6                      // switch input channel
#define OSD_DEFAULT_SELECTOR_MIN 1000                     // us, minimal input channel value for selector mode
#define OSD_DEFAULT_SELECTOR_MAX 2000                     // us, maximal input channel value for selector mode
#define OSD_DEFAULT_TOGGLE_TRESHOLD	1200                  // us, input channel value threshold for toggle mode

/*
 * MAX7456 config
 */
#define MAX7456_DEFAULT_MODE MAX7456_MODE_PAL             // default video mode, if jumper closed
#define MAX7456_DEFAULT_BRIGHTNESS 0x00                   // 120% white, 0% black

#endif /* CONFIG_H_ */
