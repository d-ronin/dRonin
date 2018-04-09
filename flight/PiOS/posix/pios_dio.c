/**
 ******************************************************************************
 * @file       pios_dio.c
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

#include <pios.h>
#include <pios_dio.h>

#ifdef PIOS_INCLUDE_DIO
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <assert.h>

#define DIO_MAX 512

static struct dio_info_s dio_descrs[DIO_MAX];
pthread_mutex_t dio_lock = PTHREAD_MUTEX_INITIALIZER;

dio_tag_t dio_init(int pin)
{
	if (pin >= DIO_MAX) {
		abort();
	}

	pthread_mutex_lock(&dio_lock);

	dio_tag_t ret = dio_descrs + pin;

	if (!ret->inited) {
		/* first export the io */
		int exp_fd = open("/sys/class/gpio/export", O_RDWR);

		char tmp[256];
		
		snprintf(tmp, 256, "%d", pin);

		_dio_fd_write(exp_fd, tmp);

		close(exp_fd);

		snprintf(tmp, sizeof(ret->val_fname),
				"/sys/class/gpio/gpio%d/value", pin);

		ret->val_fd = open(tmp, O_RDWR);

		assert(ret->val_fd);

		snprintf(tmp, 256, "/sys/class/gpio/gpio%d/direction", pin);

		ret->dir_fd = open(tmp, O_RDWR);

		assert(ret->dir_fd);

		snprintf(tmp, 256, "/sys/class/gpio/gpio%d/edge", pin);

		ret->edge_fd = open(tmp, O_RDWR);

		assert(ret->edge_fd);

		ret->inited = true;
	}

	pthread_mutex_unlock(&dio_lock);

	return ret;
}
#endif /* PIOS_INCLUDE_DIO */
