/**
 ******************************************************************************
 * @file       pios_flyingpio.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_FLYINGPIO
 * @{
 * @brief Driver for FLYINGPIO IO expander
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

#include "openpilot.h"
#include "pios.h"

#if defined(PIOS_INCLUDE_FLYINGPIO)

#include "pios_thread.h"
#include "pios_queue.h"
#include "taskmonitor.h"

#include "pios_flyingpio_priv.h"
#include "flyingpio_messages.h"

#define PIOS_FLYINGPIO_TASK_PRIORITY   PIOS_THREAD_PRIO_HIGHEST
#define PIOS_FLYINGPIO_TASK_STACK      640

/**
 * Magic byte sequence used to validate the device state struct.
 * Should be unique amongst all PiOS drivers!
 */
enum pios_flyingpio_dev_magic {
	PIOS_FLYINGPIO_DEV_MAGIC = 0x4f497046 /**< Unique byte sequence 'FpIO' */
};

/**
 * @brief The device state struct
 */
struct pios_flyingpio_dev {
	enum pios_flyingpio_dev_magic magic;              /**< Magic bytes to validate the struct contents */
	uint32_t spi_id;                            /**< Handle to the communication driver */
	uint32_t spi_slave;                         /**< The slave number (SPI) */

	struct pios_thread *task_handle;
};

//! Global structure for this device device
static struct pios_flyingpio_dev *fpio_dev;

//! Private functions
/**
 * @brief Allocate a new device
 */
static struct pios_flyingpio_dev *PIOS_FLYINGPIO_Alloc();

/**
 * @brief Validate the handle to the device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_FLYINGPIO_Validate(struct pios_flyingpio_dev *dev);

static void PIOS_FLYINGPIO_Task(void *parameters);

static struct pios_flyingpio_dev *PIOS_FLYINGPIO_Alloc()
{
	struct pios_flyingpio_dev *dev;

	dev = (struct pios_flyingpio_dev *)PIOS_malloc(sizeof(*fpio_dev));
	if (!dev)
		return NULL;

	dev->magic = PIOS_FLYINGPIO_DEV_MAGIC;

	return dev;
}

static int32_t PIOS_FLYINGPIO_Validate(struct pios_flyingpio_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_FLYINGPIO_DEV_MAGIC)
		return -2;
	if (dev->spi_id == 0)
		return -3;
	return 0;
}

int32_t PIOS_FLYINGPIO_SPI_Init(pios_flyingpio_dev_t *dev, uint32_t spi_id, uint32_t slave_idx)
{
	fpio_dev = PIOS_FLYINGPIO_Alloc();
	if (fpio_dev == NULL)
		return -1;
	*dev = fpio_dev;

	fpio_dev->spi_id = spi_id;
	fpio_dev->spi_slave = slave_idx;

        fpio_dev->task_handle = PIOS_Thread_Create(
                        PIOS_FLYINGPIO_Task, "pios_fpio",
			PIOS_FLYINGPIO_TASK_STACK, NULL,
			PIOS_FLYINGPIO_TASK_PRIORITY);

        PIOS_Assert(fpio_dev->task_handle != NULL);
        TaskMonitorAdd(TASKINFO_RUNNING_IOEXPANDER, fpio_dev->task_handle);

	return 0;
}

static void PIOS_FLYINGPIO_Task(void *parameters)
{
	(void)parameters;
	PIOS_Assert(!PIOS_FLYINGPIO_Validate(fpio_dev));

	while (true) {
		PIOS_Thread_Sleep(1);		/* XXX */

		if (PIOS_SPI_ClaimBus(fpio_dev->spi_id) != 0)
			continue;

		PIOS_SPI_RC_PinSet(fpio_dev->spi_id, fpio_dev->spi_slave, 
			false);

		struct flyingpi_msg tx_buf;
		int len = 0;

		tx_buf.id = FLYINGPICMD_ACTUATOR;
		tx_buf.body.actuator_fc.led_status =
			PIOS_LED_GetStatus(PIOS_LED_HEARTBEAT);

		flyingpi_calc_crc(&tx_buf, true, &len);

		if (!PIOS_SPI_TransferBlock(fpio_dev->spi_id, (uint8_t *)&tx_buf, NULL, len)) {
		}

		PIOS_SPI_RC_PinSet(fpio_dev->spi_id, fpio_dev->spi_slave, 
			true);
		PIOS_SPI_ReleaseBus(fpio_dev->spi_id);
	}
}

#endif // PIOS_INCLUDE_FLYINGPIO

/**
 * @}
 * @}
 */
