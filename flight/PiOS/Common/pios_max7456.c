/**
 ******************************************************************************
 * @file       pios_max7456.c
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

#include <pios.h>

#ifdef PIOS_INCLUDE_MAX7456

#include "pios_max7456.h"
#include "pios_max7456_priv.h"


/* Private method forward declerations */
bool PIOS_MAX7456_Validate(struct pios_max7456_dev *dev);
bool PIOS_MAX7456_ClaimBus(struct pios_max7456_dev *dev);
bool PIOS_MAX7456_ReleaseBus(struct pios_max7456_dev *dev);
int32_t PIOS_MAX7456_ReadReg(struct pios_max7456_dev *dev, enum pios_max7456_read_reg reg, uint8_t *val);
int32_t PIOS_MAX7456_WriteReg(struct pios_max7456_dev *dev, enum pios_max7456_write_reg reg, uint8_t val);
int32_t PIOS_MAX7456_Reset(struct pios_max7456_dev *dev);
int32_t PIOS_MAX7456_ClearOSD(struct pios_max7456_dev *dev);
int32_t PIOS_MAX7456_AutoBlackLevel(struct pios_max7456_dev *dev, bool val);

/* Public methods */
int32_t PIOS_MAX7456_Init(struct pios_max7456_dev **max7456_dev, struct pios_spi_dev *spi_dev, uint32_t spi_slave, const struct pios_max7456_cfg *cfg)
{
	struct pios_max7456_dev *dev = PIOS_malloc(sizeof(*dev));
	if (!dev)
		return -1;

	dev->cfg = cfg;
	dev->spi_dev = spi_dev;
	dev->spi_slave = spi_slave;
	dev->magic = PIOS_MAX7456_MAGIC;

	*max7456_dev = dev;

	if (PIOS_MAX7456_Reset(dev))
		return -2;

	if (PIOS_MAX7456_AutoBlackLevel(dev, true))
		return -3;

	return 0;
}

enum pios_max7456_video_type PIOS_MAX7456_GetVideoType(struct pios_max7456_dev *dev)
{
	if (!PIOS_MAX7456_Validate(dev))
		goto out_fail;

	uint8_t stat;
	if (PIOS_MAX7456_ReadReg(dev, PIOS_MAX7456_STAT_R, &stat))
		goto out_fail;

	if (PIOS_MAX7456_STAT_PAL_R(stat) == PIOS_MAX7456_STAT_PAL_TRUE)
		return PIOS_MAX7456_VIDEO_PAL;
	if (PIOS_MAX7456_STAT_NTSC_R(stat) == PIOS_MAX7456_STAT_NTSC_TRUE)
		return PIOS_MAX7456_VIDEO_NTSC;

out_fail:
	return PIOS_MAX7456_VIDEO_UNKNOWN;
}

int32_t PIOS_MAX7456_SetOutputType(struct pios_max7456_dev *dev, enum pios_max7456_video_type type)
{
	if (!PIOS_MAX7456_Validate(dev))
		goto out_fail;

	uint8_t vm0;
	if (PIOS_MAX7456_ReadReg(dev, PIOS_MAX7456_VM0_R, &vm0))
		goto out_fail;

	switch (type) {
	case PIOS_MAX7456_VIDEO_UNKNOWN:
	case PIOS_MAX7456_VIDEO_PAL:
		vm0 = PIOS_MAX7456_VM0_VSS_W(vm0, PIOS_MAX7456_VM0_VSS_PAL);
		break;
	case PIOS_MAX7456_VIDEO_NTSC:
		vm0 = PIOS_MAX7456_VM0_VSS_W(vm0, PIOS_MAX7456_VM0_VSS_NTSC);
		break;
	}

	if (PIOS_MAX7456_WriteReg(dev, PIOS_MAX7456_VM0_W, vm0))
		goto out_fail;

	return 0;

out_fail:
	return -1;
}

/* Private methods */
bool PIOS_MAX7456_Validate(struct pios_max7456_dev *dev)
{
	if (!dev)
		return false;

	if (dev->magic != PIOS_MAX7456_MAGIC)
		return false;

	return true;
}

int32_t PIOS_MAX7456_ReadReg(struct pios_max7456_dev *dev, enum pios_max7456_read_reg reg, uint8_t *val)
{
	if (!PIOS_MAX7456_ClaimBus(dev))
		return -1;

	PIOS_SPI_TransferByte((uint32_t)dev->spi_dev, reg); // request byte
	*val = PIOS_SPI_TransferByte((uint32_t)dev->spi_dev, 0); // receive response

	PIOS_MAX7456_ReleaseBus(dev);

	return 0;
}

int32_t PIOS_MAX7456_WriteReg(struct pios_max7456_dev *dev, enum pios_max7456_write_reg reg, uint8_t val)
{
	if (!PIOS_MAX7456_ClaimBus(dev))
		return -1;

	PIOS_SPI_TransferByte((uint32_t)dev->spi_dev, reg);
	PIOS_SPI_TransferByte((uint32_t)dev->spi_dev, val);

	PIOS_MAX7456_ReleaseBus(dev);

	return 0;
}

bool PIOS_MAX7456_ClaimBus(struct pios_max7456_dev *dev)
{
	if (PIOS_SPI_ClaimBus((uint32_t)dev->spi_dev))
		return false;

	if (PIOS_SPI_RC_PinSet((uint32_t)dev->spi_dev, dev->spi_slave, 0)) {
		PIOS_SPI_ReleaseBus((uint32_t)dev->spi_dev);
		return false;
	}

	return true;
}

bool PIOS_MAX7456_ReleaseBus(struct pios_max7456_dev *dev)
{
	bool retval = true;

	if (PIOS_SPI_RC_PinSet((uint32_t)dev->spi_dev, dev->spi_slave, 1))
		retval = false;

	return retval && PIOS_SPI_ReleaseBus((uint32_t)dev->spi_dev) == 0;
}

int32_t PIOS_MAX7456_Reset(struct pios_max7456_dev *dev)
{
	uint8_t vm0;
	if (PIOS_MAX7456_ReadReg(dev, PIOS_MAX7456_VM0_R, &vm0))
		goto out_fail;

	vm0 = PIOS_MAX7456_VM0_SRB_W(vm0, PIOS_MAX7456_VM0_SRB_RESET);

	if (PIOS_MAX7456_WriteReg(dev, PIOS_MAX7456_OSDBL_W, vm0))
		goto out_fail;

	PIOS_DELAY_WaituS(100);

	while (PIOS_MAX7456_VM0_SRB_R(vm0) == PIOS_MAX7456_VM0_SRB_RESET) {
		if (PIOS_MAX7456_ReadReg(dev, PIOS_MAX7456_VM0_R, &vm0))
			goto out_fail;
	}

	return 0;

out_fail:
	return -1;
}

int32_t PIOS_MAX7456_ClearOSD(struct pios_max7456_dev *dev)
{
	uint8_t dmm;
	if (PIOS_MAX7456_ReadReg(dev, PIOS_MAX7456_DMM_R, &dmm))
		goto out_fail;

	dmm = PIOS_MAX7456_DMM_CLR_W(dmm, PIOS_MAX7456_DMM_CLR_CLEAR);

	if (PIOS_MAX7456_WriteReg(dev, PIOS_MAX7456_DMM_W, dmm))
		goto out_fail;

	PIOS_DELAY_WaituS(100);

	while (PIOS_MAX7456_DMM_CLR_R(dmm) != PIOS_MAX7456_DMM_CLR_READY) {
		if (PIOS_MAX7456_ReadReg(dev, PIOS_MAX7456_DMM_R, &dmm))
			goto out_fail;
	}

	return 0;

out_fail:
	return -1;
}

int32_t PIOS_MAX7456_AutoBlackLevel(struct pios_max7456_dev *dev, bool val)
{
	uint8_t osdbl;
	if (PIOS_MAX7456_ReadReg(dev, PIOS_MAX7456_OSDBL_R, &osdbl))
		goto out_fail;

	if (val)
		osdbl = PIOS_MAX7456_OSDBL_CTL_W(osdbl, PIOS_MAX7456_OSDBL_CTL_ENABLE);
	else
		osdbl = PIOS_MAX7456_OSDBL_CTL_W(osdbl, PIOS_MAX7456_OSDBL_CTL_DISABLE);

	if (PIOS_MAX7456_WriteReg(dev, PIOS_MAX7456_OSDBL_W, osdbl))
		goto out_fail;

	return 0;

out_fail:
	return -1;
}

#endif // PIOS_INCLUDE_MAX7456

/**
 * @}
 * @}
 */
