/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SPI SPI Functions
 * @brief PIOS interface to read and write from SPI ports
 * @{
 *
 * @file       pios_spi.c
 * @author     dRonin, http://dronin.org, Copyright (C) 2016
 * @brief      Driver for Linux spidev interface
 * @see        The GNU Public License (GPL) Version 3
 * @notes
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
 */
#include <pios.h>

#if defined(PIOS_INCLUDE_SPI)
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <pios_spi_posix_priv.h>

struct pios_spi_dev {
	const struct pios_spi_cfg *cfg;
	struct pios_semaphore *busy;
	uint32_t slave_count;
	uint32_t speed_hz;

	int fd[SPI_MAX_SUBDEV];

	int selected;
};

static bool PIOS_SPI_validate(struct pios_spi_dev *com_dev)
{
	return true;
}

static struct pios_spi_dev *PIOS_SPI_alloc(void)
{
	return (PIOS_malloc(sizeof(struct pios_spi_dev)));
}

int32_t PIOS_SPI_Init(pios_spi_t *spi_id, const struct pios_spi_cfg *cfg)
{
	PIOS_Assert(spi_id);
	PIOS_Assert(cfg);

	struct pios_spi_dev *spi_dev;

	spi_dev = (struct pios_spi_dev *)PIOS_SPI_alloc();
	if (!spi_dev) goto out_fail;

	/* Bind the configuration to the device instance */
	spi_dev->cfg = cfg;

	spi_dev->busy = PIOS_Semaphore_Create();
	spi_dev->slave_count = 0;

	for (int i = 0; i < SPI_MAX_SUBDEV; i++) {
		char path[PATH_MAX + 2];

		snprintf(path, sizeof(path), "%s.%d", cfg->base_path, i);

		int fd = open(path, O_RDWR);

		if (fd < 0) {
			break;
		}

		spi_dev->slave_count = i+1;
		spi_dev->fd[i] = fd;
	}

	for (int i = 0; i < spi_dev->slave_count; i++) {
		/* Twiddle / ensure slave selects are sane. */
		PIOS_SPI_RC_PinSet(spi_dev, i, false);
		PIOS_SPI_RC_PinSet(spi_dev, i, true);
	}


	printf("PIOS_SPI: Inited %s, %d subdevs\n", cfg->base_path,
		spi_dev->slave_count);

	if (spi_dev->slave_count < 1) {
		goto out_fail;
	}

	/* Must store this before enabling interrupt */
	*spi_id = spi_dev;

	return 0;

out_fail:
	printf("PIOS_SPI: fail.\n");
	return -1;
}

int32_t PIOS_SPI_SetClockSpeed(pios_spi_t spi_dev, uint32_t spi_speed)
{
	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid);

	spi_dev->speed_hz = spi_speed;

	return spi_speed;	// Don't know the actual speed.
}

int32_t PIOS_SPI_ClaimBus(pios_spi_t spi_dev)
{
	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid);

	if (PIOS_Semaphore_Take(spi_dev->busy, 65535) != true)
		return -1;

	return 0;
}

int32_t PIOS_SPI_ReleaseBus(pios_spi_t spi_dev)
{
	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid);

	PIOS_Semaphore_Give(spi_dev->busy);

	return 0;
}

int32_t PIOS_SPI_RC_PinSet(pios_spi_t spi_dev, uint32_t slave_id, bool pin_value)
{
	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid);
	PIOS_Assert(slave_id < spi_dev->slave_count);

        struct spi_ioc_transfer xfer = {
		.delay_usecs = 1,
	};

	if (!pin_value) {	 // Select this device
		spi_dev->selected = slave_id;

		xfer.cs_change = 1;	// In this context means "leave selected"
	} else {
		PIOS_Assert(spi_dev->selected == slave_id);

		spi_dev->selected = -1;

		xfer.cs_change = 0;
	}

	int status = ioctl(spi_dev->fd[slave_id], SPI_IOC_MESSAGE(1), &xfer);

	if (status < 0) {
                perror("ioctl-SPI_IOC_MESSAGE");
		return -1;
	}

	return 0;
}

uint8_t PIOS_SPI_TransferByte(pios_spi_t spi_dev, uint8_t b)
{
	uint8_t ret = 0;

	PIOS_SPI_TransferBlock(spi_dev, &b, &ret, 1);

	return ret;
}

int32_t PIOS_SPI_TransferBlock(pios_spi_t spi_dev, const uint8_t *send_buffer, uint8_t *receive_buffer, uint16_t len)
{
	bool valid = PIOS_SPI_validate(spi_dev);

	int slave_id = spi_dev->selected;

	PIOS_Assert(valid);
	PIOS_Assert(slave_id < spi_dev->slave_count);
	PIOS_Assert(slave_id >= 0);

        struct spi_ioc_transfer xfer = {
		.rx_buf = (uintptr_t) receive_buffer,
		.tx_buf = (uintptr_t) send_buffer,
		.len = len,
		.speed_hz = spi_dev->speed_hz,
		.cs_change = 1	// Leave selected
	};

	int status = ioctl(spi_dev->fd[slave_id], SPI_IOC_MESSAGE(1), &xfer);

	if (status < 0) {
                perror("ioctl-SPI_IOC_MESSAGE");
		return -1;
	}

	return 0;
}

#endif

/**
  * @}
  * @}
  */
