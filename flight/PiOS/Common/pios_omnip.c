/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_OMNIP Driver for OmniPreSense mm-wave radar modules
 * @{
 *
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @brief      Interfaces with simple serial-based doppler & ranging devices
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
#include "pios.h"
#include "pios_omnip.h"
#include "pios_sensors.h"

#if defined(PIOS_INCLUDE_OMNIP)

#define PIOS_OMNIP_DEV_MAGIC 0x506e6d4f		/* 'OmnP' */

/* TODO: Consider a supervisor that marks statuses invalid if
 * stuff has not been received for some time. (XXX, SAFETY)
 */
#define RX_BUF_LEN 30
struct omnip_dev_s {
	uint32_t magic;

	char line_buffer[RX_BUF_LEN];
	int line_pos;

	int num_received;
	int num_invalid;

	struct pios_queue *queue;

	struct pios_sensor_rangefinder_data rf_data;
};

static void PIOS_OMNIP_ProcessMessage(omnip_dev_t dev)
{
	char *endptr;

	dev->rf_data.velocity = strtof(dev->line_buffer, &endptr);

	if (endptr == dev->line_buffer) {
		goto fail;
	}

	dev->rf_data.velocity_status = true;
	dev->rf_data.range_status = false;

	if (*endptr == ',') {
		/* Neat! Expect a range too */

		endptr++;

		char *begin = endptr;

		dev->rf_data.range = strtof(begin, &endptr);

		if (endptr == begin) {
			goto fail;
		}

		dev->rf_data.range_status = true;
	}

	dev->line_pos = 0;
	dev->num_received++;

#if 0
	printf("Passing on ranging data; vel=[%0.3f]%s range=[%0.3f]%s\n",
			dev->rf_data.velocity,
			dev->rf_data.velocity_status ? "" : "(INVALID)",
			dev->rf_data.range,
			dev->rf_data.range_status ? "" : "(INVALID)");
#endif

	/* Only process/send-on after the first couple of valid messages,
	 * in case there is stuff in the buffer or our configuration messages
	 * had not taken effect yet.
	 */
	if (dev->num_received > 2) {
		PIOS_Queue_Send(dev->queue, &dev->rf_data, 0);
	}

	return;

fail:
	dev->line_pos = 0;
	dev->num_invalid++;
}

static void PIOS_OMNIP_UpdateState(omnip_dev_t dev, uint8_t c)
{
	if (c == '\r') {
		/* Completion of message. */

		if ((dev->line_pos > 0) && (dev->line_pos < RX_BUF_LEN)) {
			dev->line_buffer[dev->line_pos] = 0;

			PIOS_OMNIP_ProcessMessage(dev);
		} else {
			dev->num_invalid++;
		}
	} else if (c == '\n') {
		/* Ignore \n at the beginning because of \r\n line separation.
		 * Otherwise, mark that we are not interested in this invalid
		 * line.
		 */

		if (dev->line_pos != 0) {
			dev->line_pos = RX_BUF_LEN;
		}
	} else {
		if (dev->line_pos < RX_BUF_LEN) {
			dev->line_buffer[dev->line_pos] = (char) c;
			dev->line_pos++;
		}
	}
}

/* Comm byte received callback */
static uint16_t PIOS_OMNIP_RxInCallback(uintptr_t context,
				      uint8_t *buf,
				      uint16_t buf_len,
				      uint16_t *headroom,
				      bool *need_yield)
{
	omnip_dev_t dev = (omnip_dev_t) context;

	PIOS_Assert(dev->magic == PIOS_OMNIP_DEV_MAGIC);

	/* process byte(s) and clear receive timer */
	for (uint8_t i = 0; i < buf_len; i++) {
		PIOS_OMNIP_UpdateState(dev, buf[i]);
	}

	/* Always signal that we can accept whatever bytes they have to give */
	if (headroom)
		*headroom = 100;

	/* We never need a yield */
	*need_yield = false;

	/* Always indicate that all bytes were consumed */
	return buf_len;
}

/* Initialise OmniPreSense interface */
int32_t PIOS_OMNIP_Init(omnip_dev_t *dev,
		      const struct pios_com_driver *driver,
		      uintptr_t lower_id)
{
	*dev = PIOS_malloc_no_dma(sizeof(**dev));

	if (!*dev) {
		return -1;
	}

	**dev = (struct omnip_dev_s) {
		.magic = PIOS_OMNIP_DEV_MAGIC,
	};

	(*dev)->queue = PIOS_Queue_Create(2,
			sizeof(struct pios_sensor_rangefinder_data));

	if (!(*dev)->queue) {
		return -2;
	}

	PIOS_SENSORS_Register(PIOS_SENSOR_RANGEFINDER, (*dev)->queue);

	/* XXX send a couple of simple initialization messages to set format */

	/* Set comm driver callback */
	(driver->bind_rx_cb)(lower_id, PIOS_OMNIP_RxInCallback, (uintptr_t) *dev);

	return 0;
}

#endif	/* PIOS_INCLUDE_OMNIP */

/** 
 * @}
 * @}
 */
