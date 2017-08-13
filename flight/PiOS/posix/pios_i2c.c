/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_I2C I2C Functions
 * @brief      Linux I2C functionality
 * @{
 *
 * @file       pios_i2c.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 *
 * @brief      I2C routines
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
 */

/* Project Includes */
#include <pios.h>
#include <pios_mutex.h>
#include <pios_i2c.h>
#include <pios_i2c_priv.h>

#if defined(PIOS_INCLUDE_I2C)

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define PIOS_I2C_MAGIC 0x63324950	/* 'PI2c' */

struct pios_i2c_adapter {
	uint32_t magic;

	int fd;

	struct pios_mutex *lock;
};

int32_t PIOS_I2C_Init(pios_i2c_t *i2c_id, const char *path)
{
	struct pios_i2c_adapter *dev;

	dev = malloc(sizeof(*dev));

	if (dev == NULL) {
		return -1;
	}

	*dev = (struct pios_i2c_adapter) {
		.magic = PIOS_I2C_MAGIC,
	};

	dev->lock = PIOS_Mutex_Create();

	if (!dev->lock) {
		return -1;
	}
 
	dev->fd = open(path, O_RDWR);

	if (dev->fd < 0) {
		perror("i2c-open");
		return -2;
	}

	*i2c_id = dev;

	/* No error */
	return 0;
}

int32_t PIOS_I2C_Transfer(pios_i2c_t i2c_id, const struct pios_i2c_txn txn_list[], uint32_t num_txns)
{
	struct pios_i2c_adapter *dev = (struct pios_i2c_adapter *)i2c_id;

	int ret = 0;

	PIOS_Assert(dev->magic == PIOS_I2C_MAGIC);

	if (num_txns > 10) {
		return -1;
	}

	struct i2c_msg msgs[num_txns];

	for (int i = 0; i < num_txns; i++) {
		msgs[i].addr = txn_list[i].addr;
		if (txn_list[i].rw == PIOS_I2C_TXN_WRITE) {
			msgs[i].flags = 0;
		} else {
			msgs[i].flags = I2C_M_RD;
		}
		msgs[i].buf = txn_list[i].buf;
		msgs[i].len = txn_list[i].len;
	}

	struct i2c_rdwr_ioctl_data msgset = {
		.msgs = msgs,
		.nmsgs = num_txns
	};

	PIOS_Mutex_Lock(dev->lock, PIOS_MUTEX_TIMEOUT_MAX);

	if (ioctl(dev->fd, I2C_RDWR, &msgset) < 0) {
		perror("i2c-ioctl"); /* XXX remove */
		ret = -1;
	}

	PIOS_Mutex_Unlock(dev->lock);

	return ret;
}

#endif /* PIOS_INCLUDE_I2C */

/**
  * @}
  * @}
  */
