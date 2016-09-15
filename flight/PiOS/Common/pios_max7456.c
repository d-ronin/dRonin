/* Imported from MultiOSD (https://github.com/UncleRus/MultiOSD/)
 * Altered for use on STM32 Flight controllers by dRonin
 * Copyright (C) dRonin 2016
 */


/*
 * This file is part of MultiOSD <https://github.com/UncleRus/MultiOSD>
 *
 * MultiOSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pios.h>

#ifdef PIOS_INCLUDE_MAX7456
#include <pios_delay.h>
#include <pios_thread.h>

#include "pios_max7456.h"
#include "pios_max7456_priv.h"

#define MAX7456_DEFAULT_BRIGHTNESS 0x00

#define MAX7456_MASK_PAL  MAX7456_VM0_VSS_MASK
#define MAX7456_MASK_NTSC 0x00 

#define SYNC_INTERVAL_NTSC 33366
#define SYNC_INTERVAL_PAL  40000

#define bis(var, bit) (var & BV (bit))

#define BV(bit) (1 << (bit))

///////////////////////////////////////////////////////////////////////////////

struct max7456_dev_s {
#define MAX7456_MAGIC 0x36353437	/* '7456' */
	uint32_t magic;
	uint32_t spi_id;
	uint32_t slave_id;

	uint8_t mode, right, bottom, hcenter, vcenter;

	uint8_t mask;
	bool opened;

	uint32_t next_sync_expected;
};

static bool poll_vsync_spi (max7456_dev_t dev);

/* Max7456 says 100ns period (10MHz) is OK.  But it may be off-board in
 * some circumstances, so let's not push our luck.
 */
#define MAX7456_SPI_SPEED 9000000

static inline void chip_select(max7456_dev_t dev)
{
	PIOS_SPI_ClaimBus(dev->spi_id);

	PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_id, false);

	PIOS_SPI_SetClockSpeed(dev->spi_id, MAX7456_SPI_SPEED);
}

static inline void chip_unselect(max7456_dev_t dev)
{
	PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_id, true);

	PIOS_SPI_ReleaseBus(dev->spi_id);
}

// Write VM0[3] = 1 to enable the display of the OSD image.
// mode | autosync | vsync on | enable OSD
#define enable_osd(dev) do { write_register_sel(dev, MAX7456_REG_VM0, dev->mask | MAX7456_VM0_OSD_MASK | MAX7456_VM0_VS_MASK); } while (0)
#define disable_osd(dev) do { write_register_sel(dev, MAX7456_REG_VM0, 0); } while (0)

// Would be nice to collapse these to single transactions through the spi
// layer.
static uint8_t read_register(max7456_dev_t dev, uint8_t reg)
{
	PIOS_SPI_TransferByte(dev->spi_id, reg | 0x80);
	return PIOS_SPI_TransferByte(dev->spi_id, 0xff);
}

static uint8_t read_register_sel(max7456_dev_t dev, uint8_t reg)
{
	uint8_t ret;

	chip_select(dev);
	ret = read_register(dev, reg);
	chip_unselect(dev);

	return ret;
}

static void write_register(max7456_dev_t dev, uint8_t reg, uint8_t val)
{
	PIOS_SPI_TransferByte(dev->spi_id, reg);
	PIOS_SPI_TransferByte(dev->spi_id, val);
}


static void write_register_sel(max7456_dev_t dev, uint8_t reg, uint8_t val)
{
	chip_select(dev);
	write_register(dev, reg, val);
	chip_unselect(dev);
}

static inline void set_mode(max7456_dev_t dev, uint8_t value)
{
	dev->mode = value;

	uint8_t vm0;

	vm0 = read_register_sel(dev, MAX7456_REG_VM0);

	if (value == MAX7456_MODE_NTSC)
	{
		dev->mask = MAX7456_MASK_NTSC;
		dev->right = MAX7456_NTSC_COLUMNS - 1;
		dev->bottom = MAX7456_NTSC_ROWS - 1;
		dev->hcenter = MAX7456_NTSC_HCENTER;
		dev->vcenter = MAX7456_NTSC_VCENTER;

		vm0 = MAX7456_VM0_VSS_W(vm0, MAX7456_VM0_VSS_NTSC);
	} else {
		PIOS_Assert(value == MAX7456_MODE_PAL);

		dev->mask = MAX7456_MASK_PAL;
		dev->right = MAX7456_PAL_COLUMNS - 1;
		dev->bottom = MAX7456_PAL_ROWS - 1;
		dev->hcenter = MAX7456_PAL_HCENTER;
		dev->vcenter = MAX7456_PAL_VCENTER;

		vm0 = MAX7456_VM0_VSS_W(vm0, MAX7456_VM0_VSS_PAL);
	}

	write_register_sel(dev, MAX7456_REG_VM0, vm0);
}

static inline void detect_mode (max7456_dev_t dev)
{
	// read STAT and auto detect video mode PAL/NTSC
	uint8_t stat = read_register_sel(dev, MAX7456_REG_STAT);

	if (MAX7456_STAT_PAL_R(stat) == MAX7456_STAT_PAL_TRUE)
	{
		set_mode(dev, MAX7456_MODE_PAL);
		return;
	}

	if (MAX7456_STAT_NTSC_R(stat) == MAX7456_STAT_NTSC_TRUE)
	{
		set_mode(dev, MAX7456_MODE_NTSC);
		return;
	}

	// Give up / guess PAL
	set_mode (dev, MAX7456_MODE_NTSC); // XXX TODO
}

static void auto_black_level(max7456_dev_t dev, bool val)
{
	uint8_t osdbl = read_register_sel(dev, MAX7456_REG_OSDBL);

	if (val)
		osdbl = MAX7456_OSDBL_CTL_W(osdbl, MAX7456_OSDBL_CTL_ENABLE);
	else
		osdbl = MAX7456_OSDBL_CTL_W(osdbl, MAX7456_OSDBL_CTL_DISABLE);

	write_register_sel(dev, MAX7456_REG_OSDBL, osdbl);
}

void PIOS_MAX7456_wait_vsync(max7456_dev_t dev)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);

	uint32_t now = PIOS_DELAY_GetuS();
	uint32_t sync_interval =
		(dev->mode == MAX7456_MODE_NTSC) ? SYNC_INTERVAL_NTSC :
			SYNC_INTERVAL_PAL;

	if (now > dev->next_sync_expected) {
		// Oh. we maybe missed it.  Update...
		dev->next_sync_expected += sync_interval;
	}

	if (now > dev->next_sync_expected) {
		// We're *REALLY* late
		dev->next_sync_expected = now + sync_interval;
	}

	while (!poll_vsync_spi(dev)) {
		now = PIOS_DELAY_GetuS();

		if (now + 200 > (dev->next_sync_expected + sync_interval)) {
			/* We missed at least one vsync in this loop.
			 * Draw open-loop, hoping we're at about the right
			 * time.
			 */
			break;
		} else {
			PIOS_Thread_Sleep(1);
		}
	}

	dev->next_sync_expected = now + sync_interval;
}

int PIOS_MAX7456_init (max7456_dev_t *dev_out,
		uint32_t spi_handle, uint32_t slave_idx)
{
	// Reset
	max7456_dev_t dev = PIOS_malloc(sizeof(*dev));

	bzero(dev, sizeof(*dev));
	dev->magic = MAX7456_MAGIC;
	dev->spi_id = spi_handle;
	dev->slave_id = slave_idx;

	write_register_sel(dev, MAX7456_REG_VM0, BV(1));

	PIOS_DELAY_WaituS(100);

	while(read_register_sel(dev, MAX7456_REG_VM0) & BV (1));

	// Detect video mode
	detect_mode(dev);

	auto_black_level(dev, true);

	uint8_t vm1 = 0;
	vm1 = MAX7456_VM1_BDUTY_W(vm1, MAX7456_VM1_BDUTY_13BT);
	vm1 = MAX7456_VM1_BTIME_W(vm1, MAX7456_VM1_BTIME_6FIELD);
	vm1 = MAX7456_VM1_BGLVL_W(vm1, MAX7456_VM1_BGLVL_42);
	vm1 = MAX7456_VM1_BGMODE_W(vm1, MAX7456_VM1_BGMODE_LOCAL);

	write_register_sel(dev, MAX7456_REG_VM1, vm1);

	enable_osd(dev);

	// set all rows to the same character brightness black/white level
	uint8_t brightness = MAX7456_DEFAULT_BRIGHTNESS;

	for (uint8_t r = MAX7456_REG_RB0; r < MAX7456_REG_RB15; r++) {
		write_register_sel(dev, r, brightness);
	}

	PIOS_MAX7456_clear(dev);

	*dev_out = dev;

	return 0;
}

void PIOS_MAX7456_clear (max7456_dev_t dev)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);
	PIOS_Assert(!dev->opened);

	uint8_t dmm;
	dmm = read_register_sel(dev, MAX7456_REG_DMM);

	dmm = MAX7456_DMM_CLR_W(dmm, MAX7456_DMM_CLR_CLEAR);

	write_register_sel(dev, MAX7456_REG_DMM, dmm);
	PIOS_DELAY_WaituS(30);

	while (MAX7456_DMM_CLR_R(dmm) != MAX7456_DMM_CLR_READY) {
		dmm = read_register_sel(dev, MAX7456_REG_DMM);
	}
}

void PIOS_MAX7456_upload_char (max7456_dev_t dev, uint8_t char_index,
		const uint8_t *data)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);
	disable_osd(dev);

	PIOS_DELAY_WaituS(10);

	// Write CMAH[7:0] = xxH to select the character (0–255) to be written
	write_register_sel(dev, MAX7456_REG_CMAH, char_index);

	for (uint8_t i = 0; i < 54; i ++)
	{
		// Write CMAL[7:0] = xxH to select the 4-pixel byte (0–53) in the character to be written
		write_register_sel(dev, MAX7456_REG_CMAL, i);
		// Write CMDI[7:0] = xxH to set the pixel values of the selected part of the character
		write_register_sel(dev, MAX7456_REG_CMDI, data[i]);
	}

	// Write CMM[7:0] = 1010xxxx to write to the NVM array from the shadow RAM
	write_register_sel(dev, MAX7456_REG_CMM, MAX7456_CMM_WRITE_NVM);

	/*
	The character memory is busy for approximately 12ms during this operation.
	STAT[5] can be read to verify that the NVM writing process is complete.
	*/

	while (MAX7456_STAT_CHMEM_R(
				read_register_sel(dev, MAX7456_REG_STAT)) ==
		       MAX7456_STAT_CHMEM_BUSY)	{
		PIOS_Thread_Sleep(1);
	}

	enable_osd(dev);
}

void PIOS_MAX7456_download_char (max7456_dev_t dev, uint8_t char_index,
		uint8_t *data)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);

	disable_osd(dev);

	PIOS_DELAY_WaituS(10);

	// Write CMAH[7:0] = xxH to select the character (0–255) to be read
	write_register_sel(dev, MAX7456_REG_CMAH, char_index);

	// Write CMM[7:0] = 0101xxxx to read the character data from the NVM to the shadow RAM
	write_register_sel(dev, MAX7456_REG_CMM, MAX7456_CMM_READ_NVM);
	/*
	 * The character memory is busy for approximately 12ms during this operation.
	 * The Character Memory Mode register is cleared and STAT[5] is reset to 0 after
	 * the write operation has been completed.
	 * 
	 * note: MPL: I don't see this being required in the datasheet for
	 * read operations.  But it doesnt hurt.
	 */
	while (MAX7456_STAT_CHMEM_R(
				read_register_sel(dev, MAX7456_REG_STAT)) ==
		       MAX7456_STAT_CHMEM_BUSY)	{
		PIOS_Thread_Sleep(1);
	}

	for (uint8_t i = 0; i < 54; i ++)
	{
		// Write CMAL[7:0] = xxH to select the 4-pixel byte (0–53) in the character to be read
		write_register_sel(dev, MAX7456_REG_CMAL, i);
		// Write CMDI[7:0] = xxH to get the pixel values of the selected part of the character
		data[i] = read_register_sel(dev, MAX7456_REG_CMDO);
	}

	enable_osd(dev);
}

/* Assumes you have already selected */
static inline void set_offset (max7456_dev_t dev, uint8_t col, uint8_t row)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);

	if (col > 29) {
		// Still will wrap to next line...
		col = 29;
	}

	uint16_t offset = (row * 30 + col) & 0x1ff;
	write_register(dev, MAX7456_REG_DMAH, offset >> 8);
	write_register(dev, MAX7456_REG_DMAL,(uint8_t) offset);
}

static bool poll_vsync_spi (max7456_dev_t dev)
{
	uint8_t status = read_register_sel(dev, MAX7456_REG_STAT);

	return MAX7456_STAT_VSYNC_R(status) == MAX7456_STAT_VSYNC_TRUE;
}

void PIOS_MAX7456_put (max7456_dev_t dev, 
		uint8_t col, uint8_t row, uint8_t chr, uint8_t attr)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);

	PIOS_Assert(!dev->opened);

	chip_select(dev);
	set_offset(dev, col, row);
	write_register(dev, MAX7456_REG_DMM,(attr & 0x07) << 3);
	write_register(dev, MAX7456_REG_DMDI, chr);
	chip_unselect(dev);
}

static void PIOS_MAX7456_open (max7456_dev_t dev, uint8_t col, uint8_t row,
		uint8_t attr)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);

	PIOS_Assert(!dev->opened);

	dev->opened = true;

	chip_select(dev);
	set_offset(dev, col > dev->right ? 0 : col, row > dev->bottom ? 0 : row);

	// 16 bits operating mode, char attributes, autoincrement
	write_register(dev, MAX7456_REG_DMM, ((attr & 0x07) << 3) | 0x01);
}

static void PIOS_MAX7456_close (max7456_dev_t dev)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);

	PIOS_Assert(dev->opened);

	// terminate autoincrement mode
	write_register(dev, MAX7456_REG_DMDI, MAX7456_DMDI_AUTOINCREMENT_STOP);

	chip_unselect(dev);
	dev->opened = false;
}

#define valid_char(c) (c == MAX7456_DMDI_AUTOINCREMENT_STOP ? 0x00 : c)
void PIOS_MAX7456_puts(max7456_dev_t dev, uint8_t col, uint8_t row, const char *s, uint8_t attr)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);

	if (col > MAX7456_COLUMNS) {
		col = ((MAX7456_COLUMNS - strlen(s)) / 2);
	}
	PIOS_MAX7456_open(dev, col, row, attr);
	while (*s)
	{
		write_register(dev, MAX7456_REG_DMDI, valid_char(*s));
		s++;
	}
	PIOS_MAX7456_close(dev);
}

void PIOS_MAX7456_get_extents(max7456_dev_t dev, 
		uint8_t *mode, uint8_t *right, uint8_t *bottom,
		uint8_t *hcenter, uint8_t *vcenter)
{
	PIOS_Assert(dev->magic == MAX7456_MAGIC);

	if (mode) {
		*mode=dev->mode;
	}

	if (right) {
		*right=dev->right;
	}

	if (bottom) {
		*bottom=dev->bottom;
	}

	if (hcenter) {
		*hcenter=dev->hcenter;
	}

	if (vcenter) {
		*vcenter=dev->vcenter;
	}
}

int32_t PIOS_MAX7456_reset(max7456_dev_t dev)
{
	uint8_t vm0 = read_register_sel(dev, MAX7456_REG_VM0);

	vm0 = MAX7456_VM0_SRB_W(vm0, MAX7456_VM0_SRB_RESET);

	write_register_sel(dev, MAX7456_REG_OSDBL, vm0);

	PIOS_DELAY_WaituS(100);

	while (MAX7456_VM0_SRB_R(vm0) == MAX7456_VM0_SRB_RESET) {
		PIOS_Thread_Sleep(1);

		vm0 = read_register_sel(dev, MAX7456_REG_VM0);
	}

	return 0;
}

#endif /* PIOS_INCLUDE_MAX7456 */
