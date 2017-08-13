/**
 ******************************************************************************
 *
 * @file       pios_serial.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @brief      SERIAL communications interface
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SERIAL Serial Driver
 * @{
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

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_SERIAL)

#include <pios_serial_priv.h>
#include "pios_thread.h"
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <linux/serial.h>
#include <sys/ioctl.h>

/* Provide a COM driver */
static void PIOS_SERIAL_ChangeBaud(uintptr_t udp_id, uint32_t baud);
static void PIOS_SERIAL_RegisterRxCallback(uintptr_t udp_id, pios_com_callback rx_in_cb, uintptr_t context);
static void PIOS_SERIAL_RegisterTxCallback(uintptr_t udp_id, pios_com_callback tx_out_cb, uintptr_t context);
static void PIOS_SERIAL_TxStart(uintptr_t udp_id, uint16_t tx_bytes_avail);
static void PIOS_SERIAL_RxStart(uintptr_t udp_id, uint16_t rx_bytes_avail);

typedef struct {
	int readfd, writefd;

	// TODO: Think about the volatility of callbacks, etc.
	// (Not unique to this module)
	pios_com_callback tx_out_cb;
	uintptr_t tx_out_context;
	pios_com_callback rx_in_cb;
	uintptr_t rx_in_context;

	uint8_t rx_buffer[PIOS_SERIAL_RX_BUFFER_SIZE];
	uint8_t tx_buffer[PIOS_SERIAL_RX_BUFFER_SIZE];

	bool dont_touch_line;
} pios_ser_dev;

const struct pios_com_driver pios_serial_com_driver = {
	.set_baud   = PIOS_SERIAL_ChangeBaud,
	.tx_start   = PIOS_SERIAL_TxStart,
	.rx_start   = PIOS_SERIAL_RxStart,
	.bind_tx_cb = PIOS_SERIAL_RegisterTxCallback,
	.bind_rx_cb = PIOS_SERIAL_RegisterRxCallback,
};

static pios_ser_dev * find_ser_dev_by_id(uintptr_t serial)
{
	return (pios_ser_dev *) serial;
}

static void rx_do_cb(pios_ser_dev *ser_dev, uint8_t *incoming_buffer,
		int len) {

	if (ser_dev->rx_in_cb) {
		bool rx_need_yield = false;

		ser_dev->rx_in_cb(ser_dev->rx_in_context, incoming_buffer,
				len, NULL, &rx_need_yield);

		return;
	}
}

/**
 * RxTask
 */
static void PIOS_SERIAL_RxTask(void *ser_dev_n)
{
	pios_ser_dev *ser_dev = (pios_ser_dev*)ser_dev_n;

	const int INCOMING_BUFFER_SIZE = 16;
	uint8_t incoming_buffer[INCOMING_BUFFER_SIZE];

	while (1) {
		int result = read(ser_dev->readfd, incoming_buffer,
				INCOMING_BUFFER_SIZE);

		if (result > 0) {
			rx_do_cb(ser_dev, incoming_buffer, result);
		}

		if (result == 0) {	/* EOF */
			if (ser_dev->dont_touch_line) {
				/* In any case we don't expect a device
				 * to go away.  For true serial devices,
				 * it probably means USB or something and
				 * keeping the rest of the tasks going
				 * seems best.  If it's stdio-ish, then
				 * we really want to take the process
				 * down.
				 */
				exit(1);
			}

			break;
		}

		if (result == -1) {
			PIOS_Thread_Sleep(1);
		}
	}
}

/**
 * Open SERIAL connection
 */
int32_t PIOS_SERIAL_InitFromFd(uintptr_t *serial_id, int readfd,
		int writefd, bool dont_touch_line)
{
	pios_ser_dev *ser_dev = PIOS_malloc(sizeof(pios_ser_dev));

	memset(ser_dev, 0, sizeof(*ser_dev));

	/* initialize */
	ser_dev->rx_in_cb = NULL;
	ser_dev->tx_out_cb = NULL;

	ser_dev->dont_touch_line = dont_touch_line;

	ser_dev->readfd = readfd;
	ser_dev->writefd = writefd;

	PIOS_Thread_Create(PIOS_SERIAL_RxTask, "pios_serial_rx",
		PIOS_THREAD_STACK_SIZE_MIN, ser_dev, PIOS_THREAD_PRIO_HIGHEST);

	printf("serial dev %p - fd %i/%i opened\n", ser_dev,
		ser_dev->readfd, ser_dev->writefd);

	*serial_id = (uintptr_t) ser_dev;

	PIOS_SERIAL_ChangeBaud(*serial_id, 9600);

	return 0;
}

int32_t PIOS_SERIAL_Init(uintptr_t *serial_id, const char *path)
{
	int fd = open(path, O_RDWR | O_NOCTTY);

	if (fd < 0) {
		perror("serial-open");
		return -1;
	}

	int ret = PIOS_SERIAL_InitFromFd(serial_id, fd, fd, false);

	if (ret < 0) {
		close(fd);

		return ret;
	}

	printf("serial dev %lu - (path %s)\n", *serial_id, path);

	return ret;
}


void PIOS_SERIAL_ChangeBaud(uintptr_t serial_id, uint32_t baud)
{
	if (!baud) {
		return;
	}

	pios_ser_dev *ser_dev = find_ser_dev_by_id(serial_id);

	PIOS_Assert(ser_dev);

	if (ser_dev->dont_touch_line) {
		return;
	}

	/* This serinfo magic for baud rate is deprecated in favor of BOTHER
	 * on Linux.  However, that requires termios2 and there's various conflict
	 * that makes it hard to do.
	 *
	 * Only consequence right now is an angry kprintf.
	 */

	if (baud != 9600) {
		struct serial_struct serinfo;

		if (ioctl(ser_dev->readfd, TIOCGSERIAL, &serinfo)) {
			perror("ioctl-TIOCGSERIAL");
		}

		serinfo.flags = (serinfo.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
		serinfo.custom_divisor = (serinfo.baud_base + (baud / 2)) / baud;
		uint32_t closest_speed = serinfo.baud_base / serinfo.custom_divisor;

		if ((closest_speed < baud * 99 / 100) ||
				(closest_speed > baud * 101 / 100)) {
			printf("Can't attain serial rate %d; using %d\n",
					baud, closest_speed);
		}

		if (ioctl(ser_dev->readfd, TIOCSSERIAL, &serinfo)) {
			perror("ioctl-TIOCSSERIAL");
		}
	}

	struct termios options;

	memset(&options, 0, sizeof(options));

	options.c_cflag = CLOCAL | CREAD | CS8;

	if (baud == 9600) {
		cfsetispeed(&options, B9600);
		cfsetospeed(&options, B9600);
	} else {
		/* The magic above replaces B38400 with the custom rate */
		cfsetispeed(&options, B38400);
		cfsetospeed(&options, B38400);
	}

	/* 1 character is enough to wake us.  Leave VTIME at 0, so we will
	 * block arbitrarily long.
	 */
	options.c_cc[VMIN] = 1;

	if (tcsetattr(ser_dev->readfd, TCSANOW, &options)) {
		perror("tcsetattr");
	}
}

static void PIOS_SERIAL_RxStart(uintptr_t tp_id, uint16_t rx_bytes_avail)
{
}

static void PIOS_SERIAL_TxStart(uintptr_t serial_id, uint16_t tx_bytes_avail)
{
	pios_ser_dev *ser_dev = find_ser_dev_by_id(serial_id);

	PIOS_Assert(ser_dev);

	int32_t length,rem;

	/**
	 * we send everything directly whenever notified of data to send
	 */
	if (ser_dev->tx_out_cb) {
		while (tx_bytes_avail > 0) {
			bool tx_need_yield = false;
			length = (ser_dev->tx_out_cb)(ser_dev->tx_out_context, ser_dev->tx_buffer, PIOS_SERIAL_RX_BUFFER_SIZE, NULL, &tx_need_yield);
			rem = length;
			while (rem > 0) {
				ssize_t len = 0;
				len = write(ser_dev->writefd, ser_dev->tx_buffer,
						length);
				if (len <= 0) {
					rem = 0;
				} else {
					rem -= len;
				}
			}
			tx_bytes_avail -= length;
		}
	}
}

static void PIOS_SERIAL_RegisterRxCallback(uintptr_t serial_id, pios_com_callback rx_in_cb, uintptr_t context)
{
	pios_ser_dev *ser_dev = find_ser_dev_by_id(serial_id);

	PIOS_Assert(ser_dev);

	ser_dev->rx_in_context = context;
	ser_dev->rx_in_cb = rx_in_cb;
}

static void PIOS_SERIAL_RegisterTxCallback(uintptr_t serial_id, pios_com_callback tx_out_cb, uintptr_t context)
{
	pios_ser_dev *ser_dev = find_ser_dev_by_id(serial_id);

	PIOS_Assert(ser_dev);

	ser_dev->tx_out_context = context;
	ser_dev->tx_out_cb = tx_out_cb;
}

#endif

/**
 * @}
 * @}
 */
