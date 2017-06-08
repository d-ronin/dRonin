/**
 ******************************************************************************
 *
 * @file       pios_flightgear.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @brief      FlightGear driver header.
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

#ifndef PIOS_FLIGHTGEAR_H
#define PIOS_FLIGHTGEAR_H

#include <pios.h>
#include <stdio.h>
#include <pthread.h>

#if !(defined(_WIN32) || defined(WIN32) || defined(__MINGW32__))
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#else
#include <ws2tcpip.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct flightgear_dev * flightgear_dev_t;

int32_t PIOS_FLIGHTGEAR_Init(flightgear_dev_t *dev,
		uint16_t port);

#endif /* PIOS_TCP_PRIV_H */
