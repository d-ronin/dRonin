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
#include <avr/io.h>
#include <util/delay.h>
#include "lib/pgmspace.h"
#include "config.h"
#include "settings.h"

#include "lib/timer/timer.h"
#include "lib/uart/uart.h"
#include "lib/spi/spi.h"
#include "lib/max7456/max7456.h"
#include "telemetry/telemetry.h"
#include "console/console.h"
#include "console/commands.h"
#include "boot.h"
#include "osd/osd.h"

#ifdef DEBUG
	#include "lib/dbgconsole.h"
#endif

inline void init ()
{
#ifdef DEBUG
	dbgconsole::init ();
#endif
	settings::init ();
	timer::init ();
	_delay_ms (500);
	CONSOLE_UART::init (uart_utils::get_bitrate (CONSOLE_BITRATE));
	spi::init ();
	max7456::init ();
	if (boot::show ())
	{
		// magic word was typed so let's run console
		max7456::clear ();
		max7456::puts_p (max7456::hcenter - 4, max7456::vcenter, PSTR ("\xfc CONFIG"));
		fprintf_P (&CONSOLE_UART::stream, PSTR ("MultiOSD v%u.%u\r\n"), VERSION >> 8, VERSION);
		console::run (console::process);
	}
	CONSOLE_UART::send_string_p (PSTR ("BOOT\r\n"));
	telemetry::init ();
	osd::init ();
}

int main ()
{
	init ();

	// main loop
	osd::main ();

	return 0;
}

