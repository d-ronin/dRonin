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
#ifndef OSD_OSD_H_
#define OSD_OSD_H_

#include "panel.h"
#include "screen.h"

#define OSD_SWITCH_MODE_SELECTOR 0
#define OSD_SWITCH_MODE_TOGGLE 1

namespace osd
{

namespace settings
{
	void init ();
	void reset ();
}

// enabled screens count
uint8_t screens_enabled ();

// load settings
void init ();

// Main loop
void main ();

}


#endif /* OSD_OSD_H_ */
