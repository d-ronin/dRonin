/**
 ******************************************************************************
 * @file       pios_dio.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DIO Digital IO subsystem
 * @{
 *
 * @brief Provides an interface to configure and efficiently use GPIOs.
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

#ifndef _PIOS_DIO_H
#define _PIOS_DIO_H

enum dio_drive_strength {
	DIO_DRIVE_WEAK = 0,
	DIO_DRIVE_LIGHT,
	DIO_DRIVE_MEDIUM,
	DIO_DRIVE_STRONG
};

enum dio_pin_function {
	DIO_PIN_INPUT = 0,
	DIO_PIN_OUTPUT = 1,
};

enum dio_pull {
	DIO_PULL_NONE = 0,
	DIO_PULL_UP,
	DIO_PULL_DOWN
};

struct dio_info_s {
	bool inited;

	int num;

	int val_fd;
	int dir_fd;
	int edge_fd;

	bool out_val;
	bool open_collector;
	bool is_output;

	char val_fname[256];
};

typedef struct dio_info_s *dio_tag_t;

#ifdef PIOS_INCLUDE_DIO

#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DIO_MAKE_TAG(pin) dio_init(pin)

dio_tag_t dio_init(int pin);

/*
#define DIO_MAKE_TAG(port, pin) ((uintptr_t) ((DIO_PORT_OFFSET(port) << 16) | ((uint16_t) (pin))))
*/

static inline void _dio_fd_write(int fd, const char *str)
{
	lseek(fd, 0, SEEK_SET);

	int expect = strlen(str);
	int ret;

	ret = write(fd, str, expect);

	(void) ret;
}

static inline void dio_write(dio_tag_t t, bool high)
{
	if (!t->is_output) {
		return;
	}

	/* low/high/in/out can be written to direction; low/high imply
	 * "set to output and drive to this value"
	 */
	if (high) {
		if (t->open_collector) {
			_dio_fd_write(t->dir_fd, "in");
		} else {
			_dio_fd_write(t->dir_fd, "high");
		}
	} else {
		_dio_fd_write(t->dir_fd, "low");
	}

	t->out_val = high;
}

static inline void dio_high(dio_tag_t t)
{
	dio_write(t, true);
}

static inline void dio_low(dio_tag_t t)
{
	dio_write(t, false);
}

static inline void dio_toggle(dio_tag_t t)
{
	dio_write(t, !t->out_val);
}

static inline void dio_set_output(dio_tag_t t, bool open_collector,
		enum dio_drive_strength strength, bool first_value)
{
	(void) strength;

	t->is_output = true;
	t->open_collector = open_collector;

	dio_write(t, first_value);
}

static inline void dio_set_input(dio_tag_t t, enum dio_pull pull)
{
	(void) pull;

	t->is_output = false;

	printf("setting input\n");

	_dio_fd_write(t->dir_fd, "in");
}

static inline bool dio_read(dio_tag_t t)
{
	lseek(t->val_fd, 0, SEEK_SET);

	char buf[16];

	int num = read(t->val_fd, buf, sizeof(buf));

	if (num < 1) {
		return false;	/* !!! */
	}

	if (buf[0] == '1') {
		return true;
	}

	return false;
}

static inline bool dio_wait(dio_tag_t t, bool rising, int usec)
{
	if (rising) {
		_dio_fd_write(t->edge_fd, "rising");
	} else {
		_dio_fd_write(t->edge_fd, "falling");
	}

	struct pollfd ev = {
		.fd = t->val_fd,
		.events = POLLPRI,
	};

	/* Consider ppoll / fancier stuff */
	int timeout = (usec + 999) / 1000;

	if (usec < 0) {
		timeout = -1;
	}

	int ret = poll(&ev, 1, timeout);

	if (ret == 1) {
		/* need to 'reset' this */
		dio_read(t);

		return true;
	}

	return false;
}

#endif /* PIOS_INCLUDE_DIO */

#endif /* _PIOS_DIO_H */

/**
 * @}
 * @}
 */
