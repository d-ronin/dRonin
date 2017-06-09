/**
 ******************************************************************************
 *
 * @file       pios_flightgear.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @brief      FlightGear driver for sensors and controls.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_FLIGHTGEAR FlightGear driver
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
#include "openpilot.h"
#include "pios.h"
#include "pios_thread.h"
#include "pios_flightgear.h"
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <fcntl.h>

#include <actuatordesired.h>

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
#   include <Winsock2.h>
#else
#   include <sys/select.h>
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

struct flightgear_dev {
	int socket;

	struct pios_queue *accel_queue, *gyro_queue;

	struct sockaddr_in send_addr;
};

/**
 * RxTask
 */
static void PIOS_FLIGHTGEAR_RxTask(void *param)
{
	flightgear_dev_t fg_dev = param;

	float accels[3], rates[3], lat, lng, alt, vels[3], temp, press;

	struct pios_sensor_accel_data accel_data = { };
	struct pios_sensor_gyro_data gyro_data = { };

	int num = 0;

	while (true) {
		char buf[320];

		ssize_t cnt = recv(fg_dev->socket, buf, sizeof(buf) - 1,
				0);

		if (cnt < 0) {
			perror("fg-recv");

			return;
		}

		buf[cnt] = 0;

		int count;

		sscanf(buf, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d",
				accels, accels+1, accels+2,
				rates, rates+1, rates+2,
				&lat, &lng, &alt,
				vels, vels+1, vels+2,
				&temp, &press, &count);

		/* Feet per second^2 to m/s^2 */
		accels[0] *= .3048;
		accels[1] *= .3048;
		accels[2] *= .3048;

		ActuatorDesiredData act_desired;

		ActuatorDesiredGet(&act_desired);

		char sendbuf[128];

		snprintf(sendbuf, sizeof(sendbuf), "%f,%f,%f,%f,%d\n",
				act_desired.Roll,
				-act_desired.Pitch,
				act_desired.Yaw,
				act_desired.Thrust,
				num++);

		if (sendto(fg_dev->socket, sendbuf, strlen(sendbuf), 0,
				(struct sockaddr *) &fg_dev->send_addr,
				sizeof(fg_dev->send_addr)) < 0) {
			perror("sendto");
		}

		while (true) {
			accel_data.x = accel_data.x * 0.3 + accels[0] * 0.7;
			accel_data.y = accel_data.y * 0.3 + accels[1] * 0.7;
			accel_data.z = accel_data.z * 0.3 + accels[2] * 0.7;

			gyro_data.x = gyro_data.x * 0.3 + rates[0] * 0.7;
			gyro_data.y = gyro_data.y * 0.3 + rates[1] * 0.7;
			gyro_data.z = gyro_data.z * 0.3 + rates[2] * 0.7;

			PIOS_Queue_Send(fg_dev->accel_queue, &accel_data, 0);
			PIOS_Queue_Send(fg_dev->gyro_queue, &gyro_data, 0);

			fd_set r;

			FD_ZERO(&r);

			FD_SET(fg_dev->socket, &r);

			/* Seek ~3ms queue rate */
			struct timeval timeout = {
				.tv_sec = 0,
				.tv_usec = 2800,
			};

			select(fg_dev->socket + 1, &r, NULL, NULL, &timeout);

			if (FD_ISSET(fg_dev->socket, &r)) {
				break;
			}
		}
	}

}

/**
 * Open socket, initialize flightgear driver.
 */
int32_t PIOS_FLIGHTGEAR_Init(flightgear_dev_t *dev, uint16_t port)
{
	flightgear_dev_t fg_dev = PIOS_malloc(sizeof(*fg_dev));
	memset(fg_dev, 0, sizeof(*fg_dev));

	/* assign socket */
	fg_dev->socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
	char optval = 1;
	/* char optoff = 0;

	setsockopt(fg_dev->socket, IPPROTO_IPV6, IPV6_V6ONLY, &optoff, sizeof(optoff));*/
#else
	int optval = 1;
#endif

        /* Allow reuse of address if you restart. */
        setsockopt(fg_dev->socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	printf("binding sockets on ports %d, %d\n", port, port+1);

	const struct in_addr any_addr = {
		.s_addr = (INADDR_ANY) /* parentheses to silence clang warning */
	};

	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
		.sin_addr = any_addr,
		.sin_port = htons(port)
	};

	fg_dev->send_addr = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_addr = any_addr,
		.sin_port = htons(port + 1)
	};

	int res = bind(fg_dev->socket, (struct sockaddr*)&saddr,
			sizeof(saddr));
	if (res == -1) {
		perror("Binding socket failed");
		exit(EXIT_FAILURE);
	}

	fg_dev->accel_queue = PIOS_Queue_Create(2, sizeof(struct pios_sensor_accel_data));
	if (fg_dev->accel_queue == NULL) {
		exit(1);
	}

	fg_dev->gyro_queue = PIOS_Queue_Create(2, sizeof(struct pios_sensor_gyro_data));
	if (fg_dev->gyro_queue == NULL) {
		exit(1);
	}

	PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_ACCEL, 333);
	PIOS_SENSORS_SetSampleRate(PIOS_SENSOR_GYRO, 333);

	PIOS_SENSORS_Register(PIOS_SENSOR_ACCEL, fg_dev->accel_queue);
	PIOS_SENSORS_Register(PIOS_SENSOR_GYRO, fg_dev->gyro_queue);

	PIOS_SENSORS_SetMaxGyro(2000);

	struct pios_thread *fg_handle = PIOS_Thread_Create(
			PIOS_FLIGHTGEAR_RxTask, "pios_fgear",
			PIOS_THREAD_STACK_SIZE_MIN, fg_dev,
			PIOS_THREAD_PRIO_HIGHEST);

	(void) fg_handle;

	printf("fg dev %p - socket %i opened\n", fg_dev, fg_dev->socket);

	*dev = fg_dev;

	return res;
}

/**
 * @}
 * @}
 */
