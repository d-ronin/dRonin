/**
 ******************************************************************************
 * @file       pios_streamfs.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_FLASHFS Flash Filesystem Function
 * @{
 * @brief Streaming circular buffer for external NOR Flash
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
 */

#ifndef PIOS_FLASHFS_STREAMFS_H_
#define PIOS_FLASHFS_STREAMFS_H_

#include <stdint.h>

/* fs_id here is actually the com driver ID, to avoid having to do too
 * much bookkeepin' */
int32_t PIOS_STREAMFS_Format(uintptr_t fs_id);
int32_t PIOS_STREAMFS_OpenWrite(uintptr_t fs_id);
int32_t PIOS_STREAMFS_OpenRead(uintptr_t fs_id, uint32_t file_id);
int32_t PIOS_STREAMFS_MinFileId(uintptr_t fs_id);
int32_t PIOS_STREAMFS_MaxFileId(uintptr_t fs_id);
int32_t PIOS_STREAMFS_Close(uintptr_t fs_id);
int32_t PIOS_STREAMFS_Read(uintptr_t fs_id, uint8_t *data, uint32_t len);


#endif	/* PIOS_FLASHFS_STREAMFS_H_ */
