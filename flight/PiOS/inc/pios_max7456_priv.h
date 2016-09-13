/**
 ******************************************************************************
 * @file       pios_max7456_priv.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup Max7456 PiOS MAX7456 Driver
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

enum pios_max7456_reg {
	MAX7456_REG_VM0   = 0x00,
	MAX7456_REG_VM1   = 0x01,
	MAX7456_REG_HOS   = 0x02,
	MAX7456_REG_VOS   = 0x03,
	MAX7456_REG_DMM   = 0x04,
	MAX7456_REG_DMAH  = 0x05,
	MAX7456_REG_DMAL  = 0x06,
	MAX7456_REG_DMDI  = 0x07,
	MAX7456_REG_CMM   = 0x08,
	MAX7456_REG_CMAH  = 0x09,
	MAX7456_REG_CMAL  = 0x0A,
	MAX7456_REG_CMDI  = 0x0B,
	MAX7456_REG_OSDM  = 0x0C,
	MAX7456_REG_RB0   = 0x10,
	MAX7456_REG_RB1   = 0x11,
	MAX7456_REG_RB2   = 0x12,
	MAX7456_REG_RB3   = 0x13,
	MAX7456_REG_RB4   = 0x14,
	MAX7456_REG_RB5   = 0x15,
	MAX7456_REG_RB6   = 0x16,
	MAX7456_REG_RB7   = 0x17,
	MAX7456_REG_RB8   = 0x18,
	MAX7456_REG_RB9   = 0x19,
	MAX7456_REG_RB10  = 0x1A,
	MAX7456_REG_RB11  = 0x1B,
	MAX7456_REG_RB12  = 0x1C,
	MAX7456_REG_RB13  = 0x1D,
	MAX7456_REG_RB14  = 0x1E,
	MAX7456_REG_RB15  = 0x1F,
	MAX7456_REG_STAT  = 0x20,
	MAX7456_REG_DMDO  = 0x30,
	MAX7456_REG_CMDO  = 0x40,
	MAX7456_REG_OSDBL = 0x6C,
};


/* Video Mode 0 Register */
#define MAX7456_VM0_VBE_MASK      (0b1)
#define MAX7456_VM0_VBE_R(val)    (val & MAX7456_VM0_VBE_MASK)
#define MAX7456_VM0_VBE_W(regval, val) ((regval & ~MAX7456_VM0_VBE_MASK) | (val & MAX7456_VM0_VBE_MASK))
#define MAX7456_VM0_VBE_ENABLE    0b0
#define MAX7456_VM0_VBE_DISABLE   0b1

#define MAX7456_VM0_SRB_MASK      (0b1 << 1)
#define MAX7456_VM0_SRB_R(val)    ((val & MAX7456_VM0_SRB_MASK) >> 1)
#define MAX7456_VM0_SRB_W(regval, val) ((regval & ~MAX7456_VM0_SRB_MASK) | (val << 1 & MAX7456_VM0_SRB_MASK))
#define MAX7456_VM0_SRB_RESET     0b1
#define MAX7456_VM0_SRB_CLEAR     0b0

#define MAX7456_VM0_VS_MASK       (0b1 << 2)
#define MAX7456_VM0_VS_R(val)     ((val & MAX7456_VM0_VS_MASK) >> 2)
#define MAX7456_VM0_VS_W(regval, val) ((regval & ~MAX7456_VM0_VS_MASK) | (val << 2 & MAX7456_VM0_VS_MASK))
#define MAX7456_VM0_VS_VSYNC      0b1
#define MAX7456_VM0_VS_IMMEDIATE  0b0

#define MAX7456_VM0_OSD_MASK      (0b1 << 3)
#define MAX7456_VM0_OSD_R(val)    ((val & MAX7456_VM0_OSD_MASK) >> 3)
#define MAX7456_VM0_OSD_W(regval, val) ((regval & ~MAX7456_VM0_OSD_MASK) | (val << 3 & MAX7456_VM0_OSD_MASK))
#define MAX7456_VM0_OSD_ENABLE    0b1
#define MAX7456_VM0_OSD_DISABLE   0b0


#define MAX7456_VM0_SYNC_MASK     (0b11 << 4)
#define MAX7456_VM0_SYNC_R(val)   ((val & MAX7456_VM0_SYNC_MASK) >> 4)
#define MAX7456_VM0_SYNC_W(regval, val) ((regval & ~MAX7456_VM0_SYNC_MASK) | (val << 4 & MAX7456_VM0_SYNC_MASK))
#define MAX7456_VM0_SYNC_AUTO     0b00
#define MAX7456_VM0_SYNC_EXT      0b10
#define MAX7456_VM0_SYNC_INT      0b11

#define MAX7456_VM0_VSS_MASK      (0b1 << 6)
#define MAX7456_VM0_VSS_R(val)    ((val & MAX7456_VM0_VSS_MASK) >> 6)
#define MAX7456_VM0_VSS_W(regval, val) ((regval & ~MAX7456_VM0_VSS_MASK) | (val << 6 & MAX7456_VM0_VSS_MASK))
#define MAX7456_VM0_VSS_PAL       0b1
#define MAX7456_VM0_VSS_NTSC      0b0

/* Video Mode 1 Register */
#define MAX7456_VM1_BDUTY_MASK    (0b11)
#define MAX7456_VM1_BDUTY_R(val)  (val & MAX7456_VM1_BDUTY_MASK)
#define MAX7456_VM1_BDUTY_W(regval, val)  ((regval & ~MAX7456_VM1_BDUTY_MASK) | (val & MAX7456_VM1_BDUTY_MASK))
#define MAX7456_VM1_BDUTY_BT      0b00
#define MAX7456_VM1_BDUTY_2BT     0b01
#define MAX7456_VM1_BDUTY_3BT     0b10
#define MAX7456_VM1_BDUTY_13BT    0b11

#define MAX7456_VM1_BTIME_MASK    (0b11 << 2)
#define MAX7456_VM1_BTIME_R(val)  ((val & MAX7456_VM1_BTIME_MASK) >> 2)
#define MAX7456_VM1_BTIME_W(regval, val) ((regval & ~MAX7456_VM1_BTIME_MASK) | (val << 2 & MAX7456_VM1_BTIME_MASK))
#define MAX7456_VM1_BTIME_2FIELD  0b00
#define MAX7456_VM1_BTIME_4FIELD  0b01
#define MAX7456_VM1_BTIME_6FIELD  0b10
#define MAX7456_VM1_BTIME_8FIELD  0b11

#define MAX7456_VM1_BGLVL_MASK    (0b111 << 4)
#define MAX7456_VM1_BGLVL_R(val)  ((val & MAX7456_VM1_BGLVL_MASK) >> 4)
#define MAX7456_VM1_BGLVL_W(regval, val) ((regval & ~MAX7456_VM1_BGLVL_MASK) | (val << 4 & MAX7456_VM1_BGLVL_MASK))
#define MAX7456_VM1_BGLVL_0       0b000
#define MAX7456_VM1_BGLVL_7       0b001
#define MAX7456_VM1_BGLVL_14      0b010
#define MAX7456_VM1_BGLVL_21      0b011
#define MAX7456_VM1_BGLVL_28      0b100
#define MAX7456_VM1_BGLVL_35      0b101
#define MAX7456_VM1_BGLVL_42      0b110
#define MAX7456_VM1_BGLVL_49      0b111

#define MAX7456_VM1_BGMODE_MASK   (0b1 << 7)
#define MAX7456_VM1_BGMODE_R(val) ((val & MAX7456_VM1_BGMODE_MASK) >> 7)
#define MAX7456_VM1_BGMODE_W(regval, val) ((regval & ~MAX7456_VM1_BGMODE_MASK) | (val << 7 & MAX7456_VM1_BGMODE_MASK))
#define MAX7456_VM1_BGMODE_LOCAL  0b0
#define MAX7456_VM1_BGMODE_GRAY   0b1

/* Horizontal Offset Register */
#define MAX7456_HOS_POS_MASK      (0x3f)
#define MAX7456_HOS_POS_R(val)    (val & MAX7456_HOS_POS_MASK)
#define MAX7456_HOS_POS_W(regval, val) (val & MAX7456_HOS_POS_MASK)

/* Vertical Offset Register */
#define MAX7456_VOS_POS_MASK      (0x3f)
#define MAX7456_VOS_POS_R(val)    (val & MAX7456_VOS_POS_MASK)
#define MAX7456_VOS_POS_W(regval, val) (val & MAX7456_VOS_POS_MASK)

/* Display Memory Mode Register */
#define MAX7456_DMM_CLR_MASK      (0b1 << 2)
#define MAX7456_DMM_CLR_R(val)    ((val & MAX7456_DMM_CLR_MASK) >> 2)
#define MAX7456_DMM_CLR_W(regval, val) ((regval & ~MAX7456_DMM_CLR_MASK) | (val << 2 & MAX7456_DMM_CLR_MASK))
#define MAX7456_DMM_CLR_CLEAR     0b1
#define MAX7456_DMM_CLR_READY     0b0

/* OSD Black Level Register */
#define MAX7456_OSDBL_CTL_MASK    (0b1 << 4)
#define MAX7456_OSDBL_CTL_R(val)  ((val & MAX7456_OSDBL_CTL_MASK)  >> 4)
#define MAX7456_OSDBL_CTL_W(regval, val) ((regval & ~MAX7456_OSDBL_CTL_MASK) | (val << 4 & MAX7456_OSDBL_CTL_MASK))
#define MAX7456_OSDBL_CTL_ENABLE  0b0
#define MAX7456_OSDBL_CTL_DISABLE 0b1

/* Status Register */
#define MAX7456_STAT_PAL_R(val)   (val & 0b1)
#define MAX7456_STAT_PAL_TRUE     0b1
#define MAX7456_STAT_PAL_FALSE    0b0
#define MAX7456_STAT_NTSC_R(val)  ((val >> 1) & 0b1)
#define MAX7456_STAT_NTSC_TRUE    0b1
#define MAX7456_STAT_NTSC_FALSE   0b0
#define MAX7456_STAT_SYNC_R(val)  ((val >> 2) & 0b1)
#define MAX7456_STAT_SYNC_FALSE   0b1
#define MAX7456_STAT_SYNC_TRUE    0b0
#define MAX7456_STAT_HSYNC_R(val) ((val >> 3) & 0b1)
#define MAX7456_STAT_HSYNC_FALSE  0b1
#define MAX7456_STAT_HSYNC_TRUE   0b0
#define MAX7456_STAT_VSYNC_R(val) ((val >> 4) & 0b1)
#define MAX7456_STAT_VSYNC_FALSE  0b1
#define MAX7456_STAT_VSYNC_TRUE   0b0
#define MAX7456_STAT_CHMEM_R(val) ((val >> 5) & 0b1)
#define MAX7456_STAT_CHMEM_RDY    0b0
#define MAX7456_STAT_CHMEM_BUSY   0b1
#define MAX7456_STAT_RESET_R(val) ((val >> 6) & 0b1)
#define MAX7456_STAT_RESET_DONE   0b0
#define MAX7456_STAT_RESET_BUSY   0b1

/* Display memory data register magical 'no autoincrement' value */
#define MAX7456_DMDI_AUTOINCREMENT_STOP 0xff

/* Magical "write to NVM array" command */
#define MAX7456_CMM_WRITE_NVM           0xa0

/* "read char from NVM array" command */
#define MAX7456_CMM_READ_NVM            0x50

#endif // PIOS_MAX7456_PRIV_H_

/**
 * @}
 * @}
 */
