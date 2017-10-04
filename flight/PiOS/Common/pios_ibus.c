/**
 ******************************************************************************
 * @file       pios_ibus.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_IBus PiOS IBus receiver driver
 * @{
 * @brief Receives and decodes IBus protocol reciever packets
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

#include "pios_ibus.h"

#ifdef PIOS_INCLUDE_IBUS

// private
#define PIOS_IBUS_CHANNELS 10
// 1 sync byte, 1 unknown byte, 10x channels (uint16_t), 8 unknown bytes, 2 crc bytes
#define PIOS_IBUS_BUFLEN (1 + 1 + PIOS_IBUS_CHANNELS * 2 + 8 + 2)
#define PIOS_IBUS_SYNCBYTE 0x20
#define PIOS_IBUS_MAGIC 0x84fd9a39

/**
 * @brief IBus receiver driver internal state data
 */
struct pios_ibus_dev {
	uint32_t magic;
	int buf_pos;
	int rx_timer;
	int failsafe_timer;
	uint16_t checksum;
	uint16_t channel_data[PIOS_IBUS_CHANNELS];
	uint8_t rx_buf[PIOS_IBUS_BUFLEN];
};

/**
 * @brief Allocates a driver instance
 * @retval pios_ibus_dev pointer on success, NULL on failure
 */
static struct pios_ibus_dev *PIOS_IBus_Alloc(void);
/**
 * @brief Validate a driver instance
 * @param[in] dev device driver instance pointer
 * @retval true on success, false on failure
 */
static bool PIOS_IBus_Validate(const struct pios_ibus_dev *dev);
/**
 * @brief Read a channel from the last received frame
 * @param[in] id Driver instance
 * @param[in] channel 0-based channel index
 * @retval raw channel value, or error value (see pios_rcvr.h)
 */
static int32_t PIOS_IBus_Read(uintptr_t id, uint8_t channel);
/**
 * @brief Set all channels in the last frame buffer to a given value
 * @param[in] dev Driver instance
 * @param[in] value channel value
 */
static void PIOS_IBus_SetAllChannels(struct pios_ibus_dev *dev, uint16_t value);
/**
 * @brief Serial receive callback
 * @param[in] context Driver instance handle
 * @param[in] buf Receive buffer
 * @param[in] buf_len Number of bytes available
 * @param[out] headroom Number of bytes remaining in internal buffer
 * @param[out] task_woken Did we awake a task?
 * @retval Number of bytes consumed from the buffer
 */
static uint16_t PIOS_IBus_Receive(uintptr_t context, uint8_t *buf, uint16_t buf_len,
		uint16_t *headroom, bool *task_woken);
/**
 * @brief Reset the internal receive buffer state
 * @param[in] dev device driver instance pointer
 */
static void PIOS_IBus_ResetBuffer(struct pios_ibus_dev *dev);
/**
 * @brief Unpack a frame from the internal receive buffer to the channel buffer
 * @param[in] dev device driver instance pointer
 */
static void PIOS_IBus_UnpackFrame(struct pios_ibus_dev *dev);
/**
 * @brief RTC tick callback
 * @param[in] context Driver instance handle
 */
static void PIOS_IBus_Supervisor(uintptr_t context);

// public
const struct pios_rcvr_driver pios_ibus_rcvr_driver = {
	.read = PIOS_IBus_Read,
};


static struct pios_ibus_dev *PIOS_IBus_Alloc(void)
{
	struct pios_ibus_dev *dev = PIOS_malloc_no_dma(sizeof(struct pios_ibus_dev));
	if (!dev)
		return NULL;

	memset(dev, 0, sizeof(*dev));
	dev->magic = PIOS_IBUS_MAGIC;

	return dev;
}

static bool PIOS_IBus_Validate(const struct pios_ibus_dev *dev)
{
	return dev && dev->magic == PIOS_IBUS_MAGIC;
}

int PIOS_IBus_Init(uintptr_t *ibus_id, const struct pios_com_driver *driver,
		uintptr_t uart_id)
{
	struct pios_ibus_dev *dev = PIOS_IBus_Alloc();

	if (!dev)
		return -1;

	*ibus_id = (uintptr_t)dev;

	PIOS_IBus_SetAllChannels(dev, PIOS_RCVR_INVALID);

	if (!PIOS_RTC_RegisterTickCallback(PIOS_IBus_Supervisor, *ibus_id))
		PIOS_Assert(0);

	(driver->bind_rx_cb)(uart_id, PIOS_IBus_Receive, *ibus_id);

	return 0;
}

static int32_t PIOS_IBus_Read(uintptr_t context, uint8_t channel)
{
	if (channel > PIOS_IBUS_CHANNELS)
		return PIOS_RCVR_INVALID;

	struct pios_ibus_dev *dev = (struct pios_ibus_dev *)context;
	if (!PIOS_IBus_Validate(dev))
		return PIOS_RCVR_NODRIVER;

	return dev->channel_data[channel];
}

static void PIOS_IBus_SetAllChannels(struct pios_ibus_dev *dev, uint16_t value)
{
	for (int i = 0; i < PIOS_IBUS_CHANNELS; i++)
		dev->channel_data[i] = value;
}

static uint16_t PIOS_IBus_Receive(uintptr_t context, uint8_t *buf, uint16_t buf_len,
		uint16_t *headroom, bool *task_woken)
{
	struct pios_ibus_dev *dev = (struct pios_ibus_dev *)context;
	if (!PIOS_IBus_Validate(dev))
		goto out_fail;

	for (int i = 0; i < buf_len; i++) {
		if (dev->buf_pos == 0 && buf[i] != PIOS_IBUS_SYNCBYTE)
			continue;

		dev->rx_buf[dev->buf_pos++] = buf[i];
		if (dev->buf_pos <= PIOS_IBUS_BUFLEN - 2)
			dev->checksum -= buf[i];
		else if (dev->buf_pos == PIOS_IBUS_BUFLEN)
			PIOS_IBus_UnpackFrame(dev);
	}

	dev->rx_timer = 0;

	if (headroom) {
		*headroom = PIOS_IBUS_BUFLEN - dev->buf_pos;
	}

	if (task_woken) {
		*task_woken = false;
	}

	return buf_len;

out_fail:
	if (headroom) {
		*headroom = 0;
	}

	if (task_woken) {
		*task_woken = false;
	}

	return 0;
}

static void PIOS_IBus_ResetBuffer(struct pios_ibus_dev *dev)
{
	dev->checksum = 0xffff;
	dev->buf_pos = 0;
}

static void PIOS_IBus_UnpackFrame(struct pios_ibus_dev *dev)
{
	uint16_t rxsum = dev->rx_buf[PIOS_IBUS_BUFLEN - 1] << 8 |
			dev->rx_buf[PIOS_IBUS_BUFLEN - 2];
	if (dev->checksum != rxsum)
		goto out_fail;

	uint16_t *chan = (uint16_t *)&dev->rx_buf[2];
	for (int i = 0; i < PIOS_IBUS_CHANNELS; i++)
		dev->channel_data[i] = *chan++;

	dev->failsafe_timer = 0;

out_fail:
	PIOS_IBus_ResetBuffer(dev);
}

static void PIOS_IBus_Supervisor(uintptr_t context)
{
	struct pios_ibus_dev *dev = (struct pios_ibus_dev *)context;
	PIOS_Assert(PIOS_IBus_Validate(dev));

	if (++dev->rx_timer > 3)
		PIOS_IBus_ResetBuffer(dev);

	/* ~250ms to failsafe. */
	if (++dev->failsafe_timer > 156)
		PIOS_IBus_SetAllChannels(dev, PIOS_RCVR_TIMEOUT);
}

#endif // PIOS_INCLUDE_IBUS

 /**
  * @}
  * @}
  */
