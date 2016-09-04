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
#ifndef OSD_SCREEN_H_
#define OSD_SCREEN_H_

#include <stdint.h>

namespace osd
{

namespace screen
{

	struct panel_pos_t
	{
		uint8_t panel;
		uint8_t x;
		uint8_t y;
	};

	void load (uint8_t num);
	void update ();
	void draw ();

	extern volatile bool updated;

	uint8_t *eeprom_offset (uint8_t num, uint8_t panel = 0)  __attribute__ ((noinline));

	namespace settings
	{

		void reset ();

	}  // namespace settings

}  // namespace screen

}


#endif /* OSD_SCREEN_H_ */
