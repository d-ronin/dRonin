/**
 ******************************************************************************
 *
 * @file       pios_fileout.c   
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @brief      Pios COM driver that writes to a file.
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   PIOS_UDP UDP Functions
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

#include <pios_fileout_priv.h>
#include "pios_thread.h"
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

/* Provide a COM driver */
static void PIOS_FILEOUT_RegisterTxCallback(uintptr_t udp_id, pios_com_callback tx_out_cb, uintptr_t context);
static void PIOS_FILEOUT_TxStart(uintptr_t udp_id, uint16_t tx_bytes_avail);

const struct pios_com_driver pios_fileout_com_driver = {
	.tx_start   = PIOS_FILEOUT_TxStart,
	.bind_tx_cb = PIOS_FILEOUT_RegisterTxCallback,
};


static pios_fileout_dev * find_file_dev_by_id(uintptr_t tcp)
{
	return (pios_fileout_dev *) tcp;
}

/**
 * Open file for writing
 */
struct pios_thread *tcpRxTaskHandle;
int32_t PIOS_FILEOUT_Init(uintptr_t *file_id, const char *filename,
		const char *mode)
{
	pios_fileout_dev *file_dev = PIOS_malloc(sizeof(pios_fileout_dev));

	memset(file_dev, 0, sizeof(*file_dev));

	file_dev->fh = fopen(filename, mode);

	if (!file_dev->fh) {
		perror("fopen");
		return -1;
	}

	setbuf(file_dev->fh, NULL);

	*file_id = (uintptr_t) file_dev;
	
	return 0;
}

static void PIOS_FILEOUT_TxStart(uintptr_t file_id, uint16_t tx_bytes_avail)
{
	pios_fileout_dev *file_dev = find_file_dev_by_id(file_id);
	
	PIOS_Assert(file_dev);
	
	int32_t length,rem;
	
	/**
	 * we send everything directly whenever notified of data to send (lazy!)
	 */
	if (file_dev->tx_out_cb) {
		while (tx_bytes_avail > 0) {
			bool tx_need_yield = false;
			length = (file_dev->tx_out_cb)(file_dev->tx_out_context, file_dev->tx_buffer, PIOS_FILEOUT_TX_BUFFER_SIZE, NULL, &tx_need_yield);
			rem = length;
			while (rem > 0) {
				ssize_t len = 0;
				len = fwrite(file_dev->tx_buffer, 1, length, file_dev->fh);

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

static void PIOS_FILEOUT_RegisterTxCallback(uintptr_t file_id, pios_com_callback tx_out_cb, uintptr_t context)
{
	pios_fileout_dev *file_dev = find_file_dev_by_id(file_id);
	
	PIOS_Assert(file_dev);
	
	/* 
	 * Order is important in these assignments since ISR uses _cb
	 * field to determine if it's ok to dereference _cb and _context
	 */
	file_dev->tx_out_context = context;
	file_dev->tx_out_cb = tx_out_cb;
}
