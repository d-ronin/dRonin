/**
 ******************************************************************************
 *
 * @file       pios_fileout_priv.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      FILEOUT private definitions.
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

#ifndef PIOS_FILEOUT_PRIV_H
#define PIOS_FILEOUT_PRIV_H

#include <pios.h>
#include <stdio.h>

#ifndef PIOS_FILEOUT_TX_BUFFER_SIZE
#define PIOS_FILEOUT_TX_BUFFER_SIZE 2048
#endif

extern const struct pios_com_driver pios_fileout_com_driver;

typedef struct {
	FILE *fh;
	pios_com_callback tx_out_cb;
	uintptr_t tx_out_context;

	uint8_t tx_buffer[PIOS_FILEOUT_TX_BUFFER_SIZE];
} pios_fileout_dev;

extern int32_t PIOS_FILEOUT_Init(uintptr_t *fileout_id, const char *filename,
		const char *mode);

#endif /* PIOS_FILEOUT_PRIV_H */
