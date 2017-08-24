/**
 ******************************************************************************
 * @file       loadable_extension.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup Libraries Libraries
 * @{
 * @brief Structures relating to loadable extensions
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

#include <stdint.h>

/* 48 byte header-- containing a high degree of redundancy/verification
 * for sanity.  Payload immediately follows.
 */
struct loadable_extension {
#define LOADABLE_EXTENSION_MAGIC        0x58655264	/* 'dReX' little endian */
#define LOADABLE_EXTENSION_UNPROGRAMMED 0xffffffff
	uint32_t magic;           /**< Magic number for structure 'dReX' */
	uint32_t length;          /**< Number of bytes (with header) of extension */

	uint32_t reserved[2];     /**< Reserved for future use, must be 0 */

#define LOADABLE_REQUIRE_VERSION_INVALID 0x00000000
#define LOADABLE_REQUIRE_VERSION_WIRED   0x00000001
	uint32_t require_version; /**< Minimum version to try loading */

	uint32_t ram_seg_len;     /**< Number of bytes of ram segments */
	uint32_t ram_seg_copylen; /**< Number of bytes to copy from flash */
	uint32_t ram_seg_copyoff; /**< Offset, from beginning of structure, of
				    * where to copy these data segs
				    */
	uint32_t ram_seg_gotlen;  /**< Length of stuff in global offset table
				    * requiring pseudo-relocs.  Assumed to be
				    * at beginning of copyoff.
				    */

	uint32_t entry_offset;    /**< Entry point, from beginning of structure */
	uint32_t header_crc;      /**< CRC of header structure before this point */
	uint32_t payload_crc;     /**< CRC of payload code */
};

/**
 * @}
 */
