/**
 ******************************************************************************
 * @file       pios_esctelemetry.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_ESCTelemetry PiOS TBS ESC telemetry decoder
 * @{
 * @brief Receives and decodes ESC telemetry packages
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

#include "pios_esctelemetry.h"

#ifdef PIOS_INCLUDE_ESCTELEMETRY

#include "pios_crc.h"
#include "pios_com.h"
#include "pios_com_priv.h"

/*

	KISS ESC telemetry format:

	Byte 0: Temperature
	Byte 1: Voltage high byte
	Byte 2: Voltage low byte
	Byte 3: Current high byte
	Byte 4: Current low byte
	Byte 5: Consumption high byte
	Byte 6: Consumption low byte
	Byte 7: Rpm high byte
	Byte 8: Rpm low byte
	Byte 9: 8-bit CRC

*/

// Internal use. Random number.
#define PIOS_ESCTELEMETRY_MAGIC		0x3e3f4182

/**
 * @brief TBS ESC telemetry internal state data
 */
struct pios_esctelemetry_dev {
	uint32_t magic;

	uintptr_t usart_id;
	const struct pios_com_driver *usart_driver;

	uint16_t rx_timer;
	uint8_t buf_pos;
	uint8_t rx_buf[ESC_TELEMETRY_LENGTH];

	struct pios_esctelemetry_info telem;
	uint8_t telem_available;
};

/**
 * @brief Validate a driver instance
 * @param[in] dev device driver instance pointer
 * @retval true on success, false on failure
 */
static bool PIOS_ESCTelemetry_Validate(const struct pios_esctelemetry_dev *dev);
/**
 * @brief Serial receive callback
 * @param[in] context Driver instance handle
 * @param[in] buf Receive buffer
 * @param[in] buf_len Number of bytes available
 * @param[out] headroom Number of bytes remaining in internal buffer
 * @param[out] task_woken Did we awake a task?
 * @retval Number of bytes consumed from the buffer
 */
static uint16_t PIOS_ESCTelemetry_Receive(uintptr_t context, uint8_t *buf, uint16_t buf_len,
		uint16_t *headroom, bool *task_woken);
/**
 * @brief Reset the internal receive buffer state
 * @param[in] dev device driver instance pointer
 */
static void PIOS_ESCTelemetry_ResetBuffer(struct pios_esctelemetry_dev *dev);
/**
 * @brief Unpack a frame from the internal receive buffer to the channel buffer
 * @param[in] dev device driver instance pointer
 */
static bool PIOS_ESCTelemetry_UnpackFrame(struct pios_esctelemetry_dev *dev);
/**
 * @brief RTC tick callback
 * @param[in] context Driver instance handle
 */
static void PIOS_ESCTelemetry_Supervisor(uintptr_t context);

static struct pios_esctelemetry_dev *esct_dev;

static bool PIOS_ESCTelemetry_Validate(const struct pios_esctelemetry_dev *dev)
{
	return dev && dev->magic == PIOS_ESCTELEMETRY_MAGIC;
}

int PIOS_ESCTelemetry_Init(uintptr_t *esctelem_id, const struct pios_com_driver *driver,
		uintptr_t usart_id)
{
	if (esct_dev)
		return -1;

	esct_dev = PIOS_malloc_no_dma(sizeof(struct pios_esctelemetry_dev));

	if (!esct_dev)
		return -1;

	memset(esct_dev, 0, sizeof(*esct_dev));
	esct_dev->magic = PIOS_ESCTELEMETRY_MAGIC;

	*esctelem_id = (uintptr_t)esct_dev;
	esct_dev->usart_id = usart_id;
	esct_dev->usart_driver = driver;

	PIOS_ESCTelemetry_ResetBuffer(esct_dev);

	if (!PIOS_RTC_RegisterTickCallback(PIOS_ESCTelemetry_Supervisor, *esctelem_id))
		PIOS_Assert(0);

	(driver->bind_rx_cb)(usart_id, PIOS_ESCTelemetry_Receive, *esctelem_id);

	return 0;
}

static uint16_t PIOS_ESCTelemetry_Receive(uintptr_t context, uint8_t *buf, uint16_t buf_len,
		uint16_t *headroom, bool *task_woken)
{
	struct pios_esctelemetry_dev *dev = (struct pios_esctelemetry_dev *)context;

	if (!PIOS_ESCTelemetry_Validate(dev))
		goto out_fail;

	for (int i = 0; i < buf_len; i++) {
		// Ignore any stuff beyond what's expected.
		if(dev->buf_pos >= ESC_TELEMETRY_LENGTH)
			break;

		dev->rx_buf[dev->buf_pos++] = buf[i];

		if (dev->buf_pos == ESC_TELEMETRY_LENGTH) {
			// Frame complete, decode.
			if(PIOS_ESCTelemetry_UnpackFrame(dev)) {
				// Unfreeze telemetry driver.
				break;
			}
		}
	}

	dev->rx_timer = 0;

	// Keep pumping data.
	if(headroom) *headroom = ESC_TELEMETRY_LENGTH;
	if(task_woken) *task_woken = false;
	return buf_len;

out_fail:
	if(headroom) *headroom = 0;
	if(task_woken) *task_woken = false;
	return 0;
}

static void PIOS_ESCTelemetry_ResetBuffer(struct pios_esctelemetry_dev *dev)
{
	dev->buf_pos = 0;
}

static bool PIOS_ESCTelemetry_UnpackFrame(struct pios_esctelemetry_dev *dev)
{
	PIOS_ESCTelemetry_ResetBuffer(dev);

	uint8_t crc = PIOS_CRC_updateCRC(0, dev->rx_buf, ESC_TELEMETRY_LENGTH-1);

	if (crc == dev->rx_buf[ESC_TELEMETRY_LENGTH-1]) {

		dev->telem.temperature = dev->rx_buf[0];
		dev->telem.voltage = (float)(dev->rx_buf[1]<<8 | dev->rx_buf[2]) * 0.01f;
		dev->telem.current = (float)(dev->rx_buf[3]<<8 | dev->rx_buf[4]) * 0.01f;
		dev->telem.mAh = (uint16_t)(dev->rx_buf[5]<<8 | dev->rx_buf[6]);
		dev->telem.rpm = (dev->rx_buf[7]<<8 | dev->rx_buf[8]);

		dev->telem_available = 1;

	}

	return false;
}

static void PIOS_ESCTelemetry_Supervisor(uintptr_t context)
{
	struct pios_esctelemetry_dev *dev = (struct pios_esctelemetry_dev *)context;
	PIOS_Assert(PIOS_ESCTelemetry_Validate(dev));

	/* ESC telemetry sends 10 bytes with no parity or stop bits at 115200baud. */
	if (++dev->rx_timer > 2)
		PIOS_ESCTelemetry_ResetBuffer(dev);
}

bool PIOS_ESCTelemetry_IsAvailable()
{
	return PIOS_ESCTelemetry_Validate(esct_dev);
}

bool PIOS_ESCTelemetry_DataAvailable()
{
	if (PIOS_ESCTelemetry_Validate(esct_dev)) {
		return esct_dev->telem_available;
	} else {
		/* Eh, don't blow up anything, if this fails. */
		return false;
	}
}

void PIOS_ESCTelemetry_Get(struct pios_esctelemetry_info *t)
{
	if (PIOS_ESCTelemetry_Validate(esct_dev) && t) {
		memcpy(t, &esct_dev->telem, sizeof(*t));
		esct_dev->telem_available = 0;
	}
}

#endif // PIOS_INCLUDE_CROSSFIRE

 /**
  * @}
  * @}
  */
