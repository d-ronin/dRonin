/**
 ******************************************************************************
 * @file       pios_max7456_priv.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_Max7456 PiOS MAX7456 Driver
 * @{
 * @brief Driver for MAX7456 OSD
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef PIOS_MAX7456_PRIV_H_
#define PIOS_MAX7456_PRIV_H_

#include "pios_max7456_cfg.h"

enum pios_max7456_write_reg {
	PIOS_MAX7456_VM0_W   = 0x00,
	PIOS_MAX7456_VM1_W   = 0x01,
	PIOS_MAX7456_HOS_W   = 0x02,
	PIOS_MAX7456_VOS_W   = 0x03,
	PIOS_MAX7456_DMM_W   = 0x04,
	PIOS_MAX7456_DMAH_W  = 0x05,
	PIOS_MAX7456_DMAL_W  = 0x06,
	PIOS_MAX7456_DMDI_W  = 0x07,
	PIOS_MAX7456_CMM_W   = 0x08,
	PIOS_MAX7456_CMAH_W  = 0x09,
	PIOS_MAX7456_CMAL_W  = 0x0A,
	PIOS_MAX7456_CMDI_W  = 0x0B,
	PIOS_MAX7456_OSDM_W  = 0x0C,
	PIOS_MAX7456_RB0_W   = 0x10,
	PIOS_MAX7456_RB1_W   = 0x11,
	PIOS_MAX7456_RB2_W   = 0x12,
	PIOS_MAX7456_RB3_W   = 0x13,
	PIOS_MAX7456_RB4_W   = 0x14,
	PIOS_MAX7456_RB5_W   = 0x15,
	PIOS_MAX7456_RB6_W   = 0x16,
	PIOS_MAX7456_RB7_W   = 0x17,
	PIOS_MAX7456_RB8_W   = 0x18,
	PIOS_MAX7456_RB9_W   = 0x19,
	PIOS_MAX7456_RB10_W  = 0x1A,
	PIOS_MAX7456_RB11_W  = 0x1B,
	PIOS_MAX7456_RB12_W  = 0x1C,
	PIOS_MAX7456_RB13_W  = 0x1D,
	PIOS_MAX7456_RB14_W  = 0x1E,
	PIOS_MAX7456_RB15_W  = 0x1F,
	PIOS_MAX7456_OSDBL_W = 0x6C,
};

enum pios_max7456_read_reg {
	PIOS_MAX7456_VM0_R   = 0x80,
	PIOS_MAX7456_VM1_R   = 0x81,
	PIOS_MAX7456_HOS_R   = 0x82,
	PIOS_MAX7456_VOS_R   = 0x83,
	PIOS_MAX7456_DMM_R   = 0x84,
	PIOS_MAX7456_DMAH_R  = 0x85,
	PIOS_MAX7456_DMAL_R  = 0x86,
	PIOS_MAX7456_DMDI_R  = 0x87,
	PIOS_MAX7456_CMM_R   = 0x88,
	PIOS_MAX7456_CMAH_R  = 0x89,
	PIOS_MAX7456_CMAL_R  = 0x8A,
	PIOS_MAX7456_CMDI_R  = 0x8B,
	PIOS_MAX7456_OSDM_R  = 0x8C,
	PIOS_MAX7456_RB0_R   = 0x90,
	PIOS_MAX7456_RB1_R   = 0x91,
	PIOS_MAX7456_RB2_R   = 0x92,
	PIOS_MAX7456_RB3_R   = 0x93,
	PIOS_MAX7456_RB4_R   = 0x94,
	PIOS_MAX7456_RB5_R   = 0x95,
	PIOS_MAX7456_RB6_R   = 0x96,
	PIOS_MAX7456_RB7_R   = 0x97,
	PIOS_MAX7456_RB8_R   = 0x98,
	PIOS_MAX7456_RB9_R   = 0x99,
	PIOS_MAX7456_RB10_R  = 0x9A,
	PIOS_MAX7456_RB11_R  = 0x9B,
	PIOS_MAX7456_RB12_R  = 0x9C,
	PIOS_MAX7456_RB13_R  = 0x9D,
	PIOS_MAX7456_RB14_R  = 0x9E,
	PIOS_MAX7456_RB15_R  = 0x9F,
	PIOS_MAX7456_OSDBL_R = 0xEC,
	PIOS_MAX7456_STAT_R  = 0xA0,
	PIOS_MAX7456_DMDO_R  = 0xB0,
	PIOS_MAX7456_CMDO_R  = 0xC0,
};


/* Video Mode 0 Register */
#define PIOS_MAX7456_VM0_VBE_R(val)    (val & 0b1)
#define PIOS_MAX7456_VM0_VBE_W(val)    (val & 0b1)
#define PIOS_MAX7456_VM0_VBE_ENABLE    0b0
#define PIOS_MAX7456_VM0_VBE_DISABLE   0b1

#define PIOS_MAX7456_VM0_SRB_MASK      (1 << 1)
#define PIOS_MAX7456_VM0_SRB_R(val)    ((val >> 1) & 0b1)
#define PIOS_MAX7456_VM0_SRB_W(regval, val) ((regval & ~PIOS_MAX7456_VM0_SRB_MASK) | ((val & 0b1) << 1))
#define PIOS_MAX7456_VM0_SRB_RESET     0b1
#define PIOS_MAX7456_VM0_SRB_CLEAR     0b0

#define PIOS_MAX7456_VM0_VS_R(val)     ((val >> 2) & 0b1)
#define PIOS_MAX7456_VM0_VS_W(val)     ((val & 0b1) << 2)
#define PIOS_MAX7456_VM0_VS_VSYNC      0b1
#define PIOS_MAX7456_VM0_VS_IMMEDIATE  0b0

#define PIOS_MAX7456_VM0_OSD_R(val)    ((val >> 3) & 0b1)
#define PIOS_MAX7456_VM0_OSD_ENABLE    0b1
#define PIOS_MAX7456_VM0_OSD_DISABLE   0b0

#define PIOS_MAX7456_VM0_SYNC_R(val)   ((val >> 4) & 0b11)
#define PIOS_MAX7456_VM0_SYNC_AUTO     0b00
#define PIOS_MAX7456_VM0_SYNC_EXT      0b10
#define PIOS_MAX7456_VM0_SYNC_INT      0b11

#define PIOS_MAX7456_VM0_VSS_MASK      (1 << 6)
#define PIOS_MAX7456_VM0_VSS_R(val)    ((val >> 6) & 0b1)
#define PIOS_MAX7456_VM0_VSS_W(regval, val) ((regval & ~PIOS_MAX7456_VM0_VSS_MASK) | ((val & 0b1) << 6))
#define PIOS_MAX7456_VM0_VSS_PAL       0b1
#define PIOS_MAX7456_VM0_VSS_NTSC      0b0

/* Video Mode 1 Register */
#define PIOS_MAX7456_VM1_BDUTY_R(val)  (val & 0b11)
#define PIOS_MAX7456_VM1_BDUTY_BT      0b00
#define PIOS_MAX7456_VM1_BDUTY_2BT     0b01
#define PIOS_MAX7456_VM1_BDUTY_3BT     0b10
#define PIOS_MAX7456_VM1_BDUTY_13BT    0b11

#define PIOS_MAX7456_VM1_BTIME_R(val)  ((val >> 2) & 0b11)
#define PIOS_MAX7456_VM1_BTIME_2FIELD  0b00
#define PIOS_MAX7456_VM1_BTIME_4FIELD  0b01
#define PIOS_MAX7456_VM1_BTIME_6FIELD  0b10
#define PIOS_MAX7456_VM1_BTIME_8FIELD  0b11

#define PIOS_MAX7456_VM1_BGLVL_R(val)  ((val >> 4) & 0b111)
#define PIOS_MAX7456_VM1_BGLVL_0       0b000
#define PIOS_MAX7456_VM1_BGLVL_7       0b001
#define PIOS_MAX7456_VM1_BGLVL_14      0b010
#define PIOS_MAX7456_VM1_BGLVL_21      0b011
#define PIOS_MAX7456_VM1_BGLVL_28      0b100
#define PIOS_MAX7456_VM1_BGLVL_35      0b101
#define PIOS_MAX7456_VM1_BGLVL_42      0b110
#define PIOS_MAX7456_VM1_BGLVL_49      0b111

#define PIOS_MAX7456_VM1_BGMODE_R(val) ((val >> 7) & 0b1)
#define PIOS_MAX7456_VM1_BGMODE_LOCAL  0b0
#define PIOS_MAX7456_VM1_BGMODE_GRAY   0b1

/* Horizontal Offset Register */
#define PIOS_MAX7456_HOS_POS_R(val)    (val & 0x3f)

/* Vertical Offset Register */
#define PIOS_MAX7456_VOS_POS_R(val)    (val & 0x3f)

/* Display Memory Mode Register */
#define PIOS_MAX7456_DMM_CLR_MASK      (1 << 2)
#define PIOS_MAX7456_DMM_CLR_R(val)    ((val >> 2) & 0b1)
#define PIOS_MAX7456_DMM_CLR_W(regval, val) ((regval & ~PIOS_MAX7456_DMM_CLR_MASK) | ((val & 0b1) << 2))
#define PIOS_MAX7456_DMM_CLR_CLEAR     0b1
#define PIOS_MAX7456_DMM_CLR_READY     0b0

/* OSD Black Level Register */
#define PIOS_MAX7456_OSDBL_CTL_MASK    (1 << 4)
#define PIOS_MAX7456_OSDBL_CTL_R(val)  ((val >> 4) & 0b1)
#define PIOS_MAX7456_OSDBL_CTL_W(regval, val) ((regval & ~PIOS_MAX7456_OSDBL_CTL_MASK) | ((val & 0b1) << 4))
#define PIOS_MAX7456_OSDBL_CTL_ENABLE  0b0
#define PIOS_MAX7456_OSDBL_CTL_DISABLE 0b1

/* Status Register */
#define PIOS_MAX7456_STAT_PAL_R(val)   (val & 0b1)
#define PIOS_MAX7456_STAT_PAL_TRUE     0b1
#define PIOS_MAX7456_STAT_PAL_FALSE    0b0
#define PIOS_MAX7456_STAT_NTSC_R(val)  ((val >> 1) & 0b1)
#define PIOS_MAX7456_STAT_NTSC_TRUE    0b1
#define PIOS_MAX7456_STAT_NTSC_FALSE   0b0
#define PIOS_MAX7456_STAT_SYNC_R(val)  ((val >> 2) & 0b1)
#define PIOS_MAX7456_STAT_SYNC_FALSE   0b1
#define PIOS_MAX7456_STAT_SYNC_TRUE    0b0
#define PIOS_MAX7456_STAT_HSYNC_R(val) ((val >> 3) & 0b1)
#define PIOS_MAX7456_STAT_HSYNC_FALSE  0b1
#define PIOS_MAX7456_STAT_HSYNC_TRUE   0b0
#define PIOS_MAX7456_STAT_VSYNC_R(val) ((val >> 4) & 0b1)
#define PIOS_MAX7456_STAT_VSYNC_FALSE  0b1
#define PIOS_MAX7456_STAT_VSYNC_TRUE   0b0
#define PIOS_MAX7456_STAT_CHMEM_R(val) ((val >> 5) & 0b1)
#define PIOS_MAX7456_STAT_CHMEM_RDY    0b0
#define PIOS_MAX7456_STAT_CHMEM_BUSY   0b1
#define PIOS_MAX7456_STAT_RESET_R(val) ((val >> 6) & 0b1)
#define PIOS_MAX7456_STAT_RESET_DONE   0b0
#define PIOS_MAX7456_STAT_RESET_BUSY   0b1


#define PIOS_MAX7456_MAGIC 0xf3fc51c4

struct pios_max7456_dev {
	const struct pios_max7456_cfg *cfg;
	struct pios_spi_dev *spi_dev;
	uint32_t spi_slave;
	uint32_t magic;
};

#endif // PIOS_MAX7456_PRIV_H_

/**
 * @}
 * @}
 */
