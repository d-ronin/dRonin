/**
 ******************************************************************************
 * @file       pios_srxl.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SRXL Multiplex SRXL protocol receiver driver
 * @{
 * @brief Supports receivers using the Multiplex SRXL protocol
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
 */

#include "pios.h"

#if defined(PIOS_INCLUDE_SRXL)

#if !defined(PIOS_INCLUDE_RTC)
#error PIOS_INCLUDE_RTC must be used to use SRXL
#endif

#include "openpilot.h"
#include "misc_math.h"
#include "pios_srxl.h"

/* Private Types */
enum pios_srxl_magic {
	PIOS_SRXL_MAGIC = 0xda8e23be
};

enum pios_srxl_sync {
	PIOS_SRXL_SYNC_12CHAN = 0xA1,
	PIOS_SRXL_SYNC_16CHAN = 0xA2,
};

struct pios_srxl_frame_12chan {
	uint8_t sync;
	uint16_t chan[12];
	uint16_t crc;
} __attribute__((packed));

struct pios_srxl_frame_16chan {
	uint8_t sync;
	uint16_t chan[16];
	uint16_t crc;
} __attribute__((packed));

/* 12 bits/chan, 1 byte header, 2 byte CRC */
#define PIOS_SRXL_RXBUF_LEN (PIOS_SRXL_MAX_CHANNELS * 2 + 3)

struct pios_srxl_dev {
	enum pios_srxl_magic magic;
	uint16_t channels[PIOS_SRXL_MAX_CHANNELS];
	uint8_t rx_buffer[PIOS_SRXL_RXBUF_LEN];
	uint8_t rx_buffer_pos;
	uint32_t rx_timer;
	uint32_t failsafe_timer;
	uint8_t frame_length;
	uint16_t crc;
};

/* Private Functions */
/**
 * @brief Allocate an SRXL device driver in memory
 * @return Pointer to dev on success, NULL on failure
 */
static struct pios_srxl_dev *PIOS_SRXL_AllocDev(void);
/**
 * @brief Validate an SRXL device struct
 * @param[in] dev Pointer to device structure
 * @return true if device is valid, false otherwise
 */
static bool PIOS_SRXL_ValidateDev(struct pios_srxl_dev *dev);
/**
 * @brief Update the CRC of the current frame with a new byte
 * @param[in] dev Pointer to device structure
 * @param[in] value New byte to be added to CRC
 */
static void PIOS_SRXL_UpdateCRC(struct pios_srxl_dev *dev, uint8_t value);
/**
 * @brief Serial receive callback
 * @param[in] context Pointer to device structure
 * @param[in] buf Pointer to receive buffer
 * @param[in] buf_len Length of receive buffer
 * @param[out] headroom Remaining headroom in local buffer
 * @param[out] task_woken Whether to force a yield
 * @return Number of bytes consumed
 */
static uint16_t PIOS_SRXL_RxCallback(uintptr_t context, uint8_t *buf,
	uint16_t buf_len, uint16_t *headroom, bool *task_woken);
/**
 * @brief Parse the current frame (checking CRC) and unpack channel contents
 * @param[in] dev Pointer to device structure
 */
static void PIOS_SRXL_ParseFrame(struct pios_srxl_dev *dev);
/**
 * @brief Reset all channels
 * @param[in] dev Pointer to device structure
 * @param[in] val Value each channel will be set to
 */
static void PIOS_SRXL_ResetChannels(struct pios_srxl_dev *dev, uint16_t val);
/**
 * @brief Get a channel value
 * @param[in] id Pointer to device structure
 * @param[in] channel Channel to grab, 0 based
 */
static int32_t PIOS_SRXL_Read(uintptr_t id, uint8_t channel);
/**
 * @brief Supervisor task to handle failsafe and frame timeout
 * @ param[in] id Pointer to device structure
 */
void PIOS_SRXL_Supervisor(uintptr_t id);

/* Private Variables */

/* Public Variables */
const struct pios_rcvr_driver pios_srxl_rcvr_driver = {
	.read = PIOS_SRXL_Read
};

/* Implementation */

int32_t PIOS_SRXL_Init(uintptr_t *srxl_id,
	const struct pios_com_driver *driver, uintptr_t lower_id)
{
	if (!driver || !lower_id)
		return -1;
	struct pios_srxl_dev *dev = PIOS_SRXL_AllocDev();
	if (dev == NULL)
		return -2;

	*srxl_id = (uintptr_t)dev;

	(driver->bind_rx_cb)(lower_id, PIOS_SRXL_RxCallback, *srxl_id);

	if (!PIOS_RTC_RegisterTickCallback(PIOS_SRXL_Supervisor, *srxl_id)) {
		PIOS_DEBUG_Assert(0);
	}

	return 0;
}

static struct pios_srxl_dev *PIOS_SRXL_AllocDev(void)
{
	struct pios_srxl_dev *dev = PIOS_malloc(sizeof(*dev));
	if (dev == NULL)
		return NULL;

	dev->magic = PIOS_SRXL_MAGIC;

	return dev;
}

static bool PIOS_SRXL_ValidateDev(struct pios_srxl_dev *dev)
{
	if (dev != NULL && dev->magic == PIOS_SRXL_MAGIC)
		return true;
	return false;
}

static void PIOS_SRXL_UpdateCRC(struct pios_srxl_dev *dev, uint8_t value)
{
	if (!PIOS_SRXL_ValidateDev(dev))
		return;

	uint8_t i;
	dev->crc = dev->crc ^ (int16_t)value << 8;
	for(i = 0; i < 8; i++)
	{
		if(dev->crc & 0x8000)
			dev->crc = dev->crc << 1^0x1021;
		else
			dev->crc = dev->crc << 1;
		}
}

static uint16_t PIOS_SRXL_RxCallback(uintptr_t context, uint8_t *buf,
	uint16_t buf_len, uint16_t *headroom, bool *task_woken)
{
	struct pios_srxl_dev *dev = (struct pios_srxl_dev *)context;
	if (!PIOS_SRXL_ValidateDev(dev))
		return PIOS_RCVR_INVALID;

	dev->rx_timer = 0;
	uint16_t consumed = 0;

	for (int i = 0; i < buf_len; i++) {
		if (dev->rx_buffer_pos == 0) {
			dev->crc = 0;
			if (buf[i] == PIOS_SRXL_SYNC_12CHAN) {
				dev->frame_length = sizeof(struct pios_srxl_frame_12chan);
			} else if (buf[i] == PIOS_SRXL_SYNC_16CHAN) {
				dev->frame_length = sizeof(struct pios_srxl_frame_16chan);
			} else {
				continue;
			}
		}
		dev->rx_buffer[dev->rx_buffer_pos++] = buf[i];
		PIOS_SRXL_UpdateCRC(dev, buf[i]);
		if (dev->rx_buffer_pos == dev->frame_length)
				PIOS_SRXL_ParseFrame(dev);
		consumed++;
	}

	if (headroom)
		*headroom = PIOS_SRXL_RXBUF_LEN - dev->rx_buffer_pos;
	*task_woken = false;

	return consumed;
}

static void PIOS_SRXL_ParseFrame(struct pios_srxl_dev *dev)
{
	if (!PIOS_SRXL_ValidateDev(dev))
		return;

	if (dev->crc == 0) {
		switch (dev->rx_buffer[0]) {
		case PIOS_SRXL_SYNC_16CHAN:
			{
				struct pios_srxl_frame_16chan *frame =
					(struct pios_srxl_frame_16chan *)dev->rx_buffer;
				/* divide by 4095 would be correct but 4096 will end up a 12 bit shift */
				for (int i = 0; i < 16; i++)
					dev->channels[i] = 800 + (frame->chan[i] >> 8 | (frame->chan[i] & 0xf) << 8) * 1400 / 4096;
			}
			break;
		case PIOS_SRXL_SYNC_12CHAN:
			{
				struct pios_srxl_frame_12chan *frame =
					(struct pios_srxl_frame_12chan *)dev->rx_buffer;
				for (int i = 0; i < 12; i++)
					dev->channels[i] = 800 + (frame->chan[i] >> 8 | (frame->chan[i] & 0xf) << 8) * 1400 / 4096;
			}
			break;
		}
		dev->failsafe_timer = 0;
	}

	dev->rx_buffer_pos = 0;
}

static int32_t PIOS_SRXL_Read(uintptr_t id, uint8_t channel)
{
	struct pios_srxl_dev *dev = (struct pios_srxl_dev *)id;
	if (!PIOS_SRXL_ValidateDev(dev))
		return PIOS_RCVR_INVALID;
	return dev->channels[channel];
}

static void PIOS_SRXL_ResetChannels(struct pios_srxl_dev *dev, uint16_t val)
{
	if (!PIOS_SRXL_ValidateDev(dev))
		return;

	for (int i = 0; i < NELEMENTS(dev->channels); i++)
		dev->channels[i] = val;
}

void PIOS_SRXL_Supervisor(uintptr_t id)
{
	struct pios_srxl_dev *dev = (struct pios_srxl_dev *)id;
 	if (!PIOS_SRXL_ValidateDev(dev))
		return;
	/*
	 * RTC runs this callback at 625 Hz (T=1.6 ms)
	 * Frame period is either 14 ms or 21 ms
	 * We will reset the rx buffer if nothing is received for 8 ms (5 ticks)
	 * We will failsafe if 4 frames (low rate) are lost
	 * (4 * 21) / 1.6 ~ 53 ticks
	 */
	if (++dev->rx_timer >= 5)
		dev->rx_buffer_pos = 0;
	if (++dev->failsafe_timer >= 53)
		PIOS_SRXL_ResetChannels(dev, PIOS_RCVR_TIMEOUT);
}

DONT_BUILD_IF(sizeof(struct pios_srxl_frame_12chan) > PIOS_SRXL_RXBUF_LEN, SRXLBufferSize);
DONT_BUILD_IF(sizeof(struct pios_srxl_frame_16chan) > PIOS_SRXL_RXBUF_LEN, SRXLBufferSize);

#endif /* defined(PIOS_INCLUDE_SRXL) */

/**
 * @}
 * @}
 */
