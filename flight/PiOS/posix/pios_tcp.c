/**
 ******************************************************************************
 *
 * @file       pios_tcp.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @brief      TCP commands. Inits UDPs, controls UDPs & Interupt handlers.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_TCP TCP Driver
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

#if defined(PIOS_INCLUDE_TCP)

#include <pios_tcp_priv.h>
#include "pios_thread.h"
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

/* Provide a COM driver */
static void PIOS_TCP_ChangeBaud(uintptr_t udp_id, uint32_t baud);
static void PIOS_TCP_RegisterRxCallback(uintptr_t udp_id, pios_com_callback rx_in_cb, uintptr_t context);
static void PIOS_TCP_RegisterTxCallback(uintptr_t udp_id, pios_com_callback tx_out_cb, uintptr_t context);
static void PIOS_TCP_TxStart(uintptr_t udp_id, uint16_t tx_bytes_avail);
static void PIOS_TCP_RxStart(uintptr_t udp_id, uint16_t rx_bytes_avail);

typedef struct {
	const struct pios_tcp_cfg * cfg;

	int socket;
	struct sockaddr_in6 server;
	struct sockaddr_in6 client;
	int socket_connection;

	pios_com_callback tx_out_cb;
	uintptr_t tx_out_context;
	pios_com_callback rx_in_cb;
	uintptr_t rx_in_context;

	uint8_t rx_buffer[PIOS_TCP_RX_BUFFER_SIZE];
	uint8_t tx_buffer[PIOS_TCP_RX_BUFFER_SIZE];
} pios_tcp_dev;

const struct pios_com_driver pios_tcp_com_driver = {
	.set_baud   = PIOS_TCP_ChangeBaud,
	.tx_start   = PIOS_TCP_TxStart,
	.rx_start   = PIOS_TCP_RxStart,
	.bind_tx_cb = PIOS_TCP_RegisterTxCallback,
	.bind_rx_cb = PIOS_TCP_RegisterRxCallback,
};


static pios_tcp_dev * find_tcp_dev_by_id(uintptr_t tcp)
{
	return (pios_tcp_dev *) tcp;
}

static int set_nonblock(int sock) {
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
	unsigned long flag = 1;
	if (!ioctlsocket(sock, FIONBIO, &flag)) {
		return 0;
	}
#else
	int flags;
	if ((flags = fcntl(sock, F_GETFL, 0)) != -1) {
		if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1) {
			return 0;
		}
	}
#endif

	return -1;
}

/**
 * RxTask
 */
static void PIOS_TCP_RxTask(void *tcp_dev_n)
{
	pios_tcp_dev *tcp_dev = (pios_tcp_dev*)tcp_dev_n;
	
	const int INCOMING_BUFFER_SIZE = 16;
	char incoming_buffer[INCOMING_BUFFER_SIZE];
	int error;

	while (1) {
	
		do
		{
			/* Polling the fd has to be executed in thread suspended mode
			 * to get a correct errno value. */
			PIOS_Thread_Scheduler_Suspend();

			tcp_dev->socket_connection = accept(tcp_dev->socket, NULL, NULL);
			error = errno;

			PIOS_Thread_Scheduler_Resume();

			PIOS_Thread_Sleep(1);
		} while (tcp_dev->socket_connection == INVALID_SOCKET && (error == EINTR || error == EAGAIN));

		if (tcp_dev->socket_connection < 0) {
			int error = errno;
			(void)error;
			perror("Accept failed");
			close(tcp_dev->socket);
			exit(EXIT_FAILURE);
		}

		set_nonblock(tcp_dev->socket_connection);
		
		fprintf(stderr, "Connection accepted\n");

		while (1) {
			// Received is used to track the scoket whereas the dev variable is only updated when it can be

			/* Polling the fd has to be executed in thread suspended mode
			 * to get a correct errno value. */
			PIOS_Thread_Scheduler_Suspend();

			int result = read(tcp_dev->socket_connection, incoming_buffer, INCOMING_BUFFER_SIZE);
			error = errno;

			PIOS_Thread_Scheduler_Resume();
			
			if (result > 0 && tcp_dev->rx_in_cb) {

				bool rx_need_yield = false;

				tcp_dev->rx_in_cb(tcp_dev->rx_in_context, (uint8_t*)incoming_buffer, result, NULL, &rx_need_yield);

			}

			if (result == 0) {
				break;
			}

			if (result == -1) {
				if (error == EAGAIN)
					PIOS_Thread_Sleep(1);
				else if (error == EINTR)
					(void)error;
				else
					break;
			}
		}
		
		close(tcp_dev->socket_connection);
		tcp_dev->socket_connection = INVALID_SOCKET;
	}
}


/**
 * Open TCP socket
 */
struct pios_thread *tcpRxTaskHandle;
int32_t PIOS_TCP_Init(uintptr_t *tcp_id, const struct pios_tcp_cfg * cfg)
{
	pios_tcp_dev *tcp_dev = PIOS_malloc(sizeof(pios_tcp_dev));

	memset(tcp_dev, 0, sizeof(*tcp_dev));

	/* initialize */
	tcp_dev->rx_in_cb = NULL;
	tcp_dev->tx_out_cb = NULL;
	tcp_dev->cfg=cfg;
	
	/* assign socket */
	tcp_dev->socket = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
	tcp_dev->socket_connection = INVALID_SOCKET;

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
	char optval = 1;
#else
	int optval = 1;
#endif

        /* Allow reuse of address if you restart. */
        setsockopt(tcp_dev->socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        /* Also request low-latency (don't store up data to conserve packets */
        setsockopt(tcp_dev->socket, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));

	memset(&tcp_dev->server, 0, sizeof(tcp_dev->server));
	memset(&tcp_dev->client, 0, sizeof(tcp_dev->client));

	tcp_dev->server.sin6_family = AF_INET6;
	tcp_dev->server.sin6_addr = in6addr_any;
	tcp_dev->server.sin6_port = htons(tcp_dev->cfg->port);

	/* set socket options */
	int value = 1;
	if (setsockopt(tcp_dev->socket, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value)) == -1) {

	}

	int res= bind(tcp_dev->socket, (struct sockaddr*)&tcp_dev->server, sizeof(tcp_dev->server));
	if (res == -1) {
		perror("Binding socket failed");
		exit(EXIT_FAILURE);
	}
	
	res = listen(tcp_dev->socket, 10);
	if (res == -1) {
		perror("Socket listen failed");
		exit(EXIT_FAILURE);
	}
	
	/* Set socket nonblocking. */
	set_nonblock(tcp_dev->socket);

	tcpRxTaskHandle = PIOS_Thread_Create(
			PIOS_TCP_RxTask, "pios_tcp_rx", PIOS_THREAD_STACK_SIZE_MIN, tcp_dev, PIOS_THREAD_PRIO_HIGHEST);
	
	printf("tcp dev %p - socket %i opened - result %i\n", tcp_dev, tcp_dev->socket, res);
	
	*tcp_id = (uintptr_t) tcp_dev;
	
	return res;
}


void PIOS_TCP_ChangeBaud(uintptr_t tcp_id, uint32_t baud)
{
	/**
	 * doesn't apply!
	 */
}


static void PIOS_TCP_RxStart(uintptr_t tp_id, uint16_t rx_bytes_avail)
{
	/**
	 * lazy!
	 */
}


static void PIOS_TCP_TxStart(uintptr_t tcp_id, uint16_t tx_bytes_avail)
{
	pios_tcp_dev *tcp_dev = find_tcp_dev_by_id(tcp_id);
	
	PIOS_Assert(tcp_dev);
	
	int32_t length,rem;
	
	/**
	 * we send everything directly whenever notified of data to send (lazy!)
	 */
	if (tcp_dev->tx_out_cb) {
		while (tx_bytes_avail > 0) {
			bool tx_need_yield = false;
			length = (tcp_dev->tx_out_cb)(tcp_dev->tx_out_context, tcp_dev->tx_buffer, PIOS_TCP_RX_BUFFER_SIZE, NULL, &tx_need_yield);
			rem = length;
			while (rem > 0) {
				ssize_t len = 0;
				if (tcp_dev->socket_connection != INVALID_SOCKET) {
					len = write(tcp_dev->socket_connection, tcp_dev->tx_buffer, length);
				}
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

static void PIOS_TCP_RegisterRxCallback(uintptr_t tcp_id, pios_com_callback rx_in_cb, uintptr_t context)
{
	pios_tcp_dev *tcp_dev = find_tcp_dev_by_id(tcp_id);
	
	PIOS_Assert(tcp_dev);
	
	/*
	 * Order is important in these assignments since ISR uses _cb
	 * field to determine if it's ok to dereference _cb and _context
	 */
	tcp_dev->rx_in_context = context;
	tcp_dev->rx_in_cb = rx_in_cb;
}

static void PIOS_TCP_RegisterTxCallback(uintptr_t tcp_id, pios_com_callback tx_out_cb, uintptr_t context)
{
	pios_tcp_dev *tcp_dev = find_tcp_dev_by_id(tcp_id);
	
	PIOS_Assert(tcp_dev);
	
	/*
	 * Order is important in these assignments since ISR uses _cb
	 * field to determine if it's ok to dereference _cb and _context
	 */
	tcp_dev->tx_out_context = context;
	tcp_dev->tx_out_cb = tx_out_cb;
}

#endif

/**
 * @}
 * @}
 */
