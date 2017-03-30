/**
 ******************************************************************************
 * @file       pios_crossfire.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_Crossfire PiOS TBS Crossfire receiver driver
 * @{
 * @brief Receives and decodes CRSF protocol receiver packets
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

#include "pios_crossfire.h"
#include "pios_crc.h"

#ifdef PIOS_INCLUDE_CROSSFIRE

// Addresses with one-based index, because they're used after a pointer
// increase only.
#define CRSF_LEN_IDX				2
#define CRSF_HEADER_IDX				3

#include "pios_crc.h"
#include "pios_com.h"
#include "pios_com_priv.h"

#define CRSF_CRCFIELD(frame)		(frame.payload[frame.length - CRSF_CRC_LEN - CRSF_TYPE_LEN])

// Internal use. Random number.
#define PIOS_CROSSFIRE_MAGIC		0xcdf19cf5 

/**
 * @brief TBS Crossfire receiver driver internal state data
 */
struct pios_crossfire_dev {
	uint32_t magic;
	uint8_t buf_pos;
	uint16_t rx_timer;
	uint16_t failsafe_timer;
	uint8_t bytes_expected;

	uint16_t channel_data[PIOS_CROSSFIRE_CHANNELS];

	union {
		struct crsf_frame_t frame;
		uint8_t rx_buf[sizeof(struct crsf_frame_t)];
	} u;

	// Need a copy of the USART/driver ref to initialize a COM device
	// when necessary.
	uintptr_t usart_id;
	const struct pios_com_driver *usart_driver;

	// COM device ID for sending telemetry. Might be this USART
	// or a separate one.
	uintptr_t telem_com_id;

	// To track frame starts to track whether telemetry is OK to send.
	uint32_t time_frame_start;
};

/**
 * @brief Allocates a driver instance
 * @retval pios_crossfire_dev pointer on success, NULL on failure
 */
static struct pios_crossfire_dev *PIOS_Crossfire_Alloc(void);
/**
 * @brief Validate a driver instance
 * @param[in] dev device driver instance pointer
 * @retval true on success, false on failure
 */
static bool PIOS_Crossfire_Validate(const struct pios_crossfire_dev *dev);
/**
 * @brief Read a channel from the last received frame
 * @param[in] id Driver instance
 * @param[in] channel 0-based channel index
 * @retval raw channel value, or error value (see pios_rcvr.h)
 */
static int32_t PIOS_Crossfire_Read(uintptr_t id, uint8_t channel);
/**
 * @brief Set all channels in the last frame buffer to a given value
 * @param[in] dev Driver instance
 * @param[in] value channel value
 */
static void PIOS_Crossfire_SetAllChannels(struct pios_crossfire_dev *dev, uint16_t value);
/**
 * @brief Serial receive callback
 * @param[in] context Driver instance handle
 * @param[in] buf Receive buffer
 * @param[in] buf_len Number of bytes available
 * @param[out] headroom Number of bytes remaining in internal buffer
 * @param[out] task_woken Did we awake a task?
 * @retval Number of bytes consumed from the buffer
 */
static uint16_t PIOS_Crossfire_Receive(uintptr_t context, uint8_t *buf, uint16_t buf_len,
		uint16_t *headroom, bool *task_woken);
/**
 * @brief Reset the internal receive buffer state
 * @param[in] dev device driver instance pointer
 */
static void PIOS_Crossfire_ResetBuffer(struct pios_crossfire_dev *dev);
/**
 * @brief Unpack a frame from the internal receive buffer to the channel buffer
 * @param[in] dev device driver instance pointer
 */
static bool PIOS_Crossfire_UnpackFrame(struct pios_crossfire_dev *dev);
/**
 * @brief RTC tick callback
 * @param[in] context Driver instance handle
 */
static void PIOS_Crossfire_Supervisor(uintptr_t context);

// public
const struct pios_rcvr_driver pios_crossfire_rcvr_driver = {
	.read = PIOS_Crossfire_Read,
};


static struct pios_crossfire_dev *PIOS_Crossfire_Alloc(void)
{
	struct pios_crossfire_dev *dev = PIOS_malloc_no_dma(sizeof(struct pios_crossfire_dev));
	if (!dev)
		return NULL;

	memset(dev, 0, sizeof(*dev));
	dev->magic = PIOS_CROSSFIRE_MAGIC;

	return dev;
}

static bool PIOS_Crossfire_Validate(const struct pios_crossfire_dev *dev)
{
	return dev && dev->magic == PIOS_CROSSFIRE_MAGIC;
}

int PIOS_Crossfire_Init(uintptr_t *crsf_id, const struct pios_com_driver *driver,
		uintptr_t usart_id)
{
	struct pios_crossfire_dev *dev = PIOS_Crossfire_Alloc();

	if (!dev)
		return -1;

	*crsf_id = (uintptr_t)dev;
	dev->usart_id = usart_id;
	dev->usart_driver = driver;

	PIOS_Crossfire_ResetBuffer(dev);
	PIOS_Crossfire_SetAllChannels(dev, PIOS_RCVR_INVALID);

	// Get COM device for telemetry.
	if(PIOS_COM_Init(&dev->telem_com_id, dev->usart_driver, dev->usart_id, 0, CRSF_MAX_FRAMELEN))
		return -1;

	if (!PIOS_RTC_RegisterTickCallback(PIOS_Crossfire_Supervisor, *crsf_id))
		PIOS_Assert(0);

	(driver->bind_rx_cb)(usart_id, PIOS_Crossfire_Receive, *crsf_id);

	return 0;
}

int PIOS_Crossfire_InitTelemetry(uintptr_t crsf_id)
{
	struct pios_crossfire_dev *dev = (struct pios_crossfire_dev*)crsf_id;

	// What the hell? Return error code and don't assert, telemetry ain't vital.
	if(!PIOS_Crossfire_Validate(dev))
		return -1;
	else
		return 0;
}

static int32_t PIOS_Crossfire_Read(uintptr_t context, uint8_t channel)
{
	if (channel > PIOS_CROSSFIRE_CHANNELS)
		return PIOS_RCVR_INVALID;

	struct pios_crossfire_dev *dev = (struct pios_crossfire_dev *)context;
	if (!PIOS_Crossfire_Validate(dev))
		return PIOS_RCVR_NODRIVER;

	return dev->channel_data[channel];
}

static void PIOS_Crossfire_SetAllChannels(struct pios_crossfire_dev *dev, uint16_t value)
{
	for (int i = 0; i < PIOS_CROSSFIRE_CHANNELS; i++)
		dev->channel_data[i] = value;
}

static uint16_t PIOS_Crossfire_Receive(uintptr_t context, uint8_t *buf, uint16_t buf_len,
		uint16_t *headroom, bool *task_woken)
{
	struct pios_crossfire_dev *dev = (struct pios_crossfire_dev *)context;

	if (!PIOS_Crossfire_Validate(dev))
		goto out_fail;

	if(dev->buf_pos == 0) {
		dev->time_frame_start = PIOS_DELAY_GetRaw();
	}

	for (int i = 0; i < buf_len; i++) {
		// Ignore any stuff beyond what's expected.
		if(dev->buf_pos >= dev->bytes_expected)
			break;

		dev->u.rx_buf[dev->buf_pos++] = buf[i];

		if(dev->buf_pos == CRSF_LEN_IDX) {
			// Read length field and adjust. Denotes payload, plus type field, plus CRC field.
			dev->bytes_expected = CRSF_ADDRESS_LEN + CRSF_LENGTH_LEN + dev->u.frame.length;

			// If length field isn't plausible, ignore rest of the data.
			if(dev->bytes_expected >= CRSF_MAX_FRAMELEN) {
				dev->bytes_expected = 0;
				break;
			}
		}

		if (dev->buf_pos == dev->bytes_expected) {
			// Frame complete, decode.
			if(PIOS_Crossfire_UnpackFrame(dev)) {
				// Frame is valid, trigger semaphore.
				PIOS_RCVR_ActiveFromISR();
				// Ignore any stuff that might be left in the buffer.
				// There shouldn't be anything.
				break;
			}
		}
	}

	dev->rx_timer = 0;

	// Keep pumping data.
	if(headroom) *headroom = CRSF_MAX_FRAMELEN;
	if(task_woken) *task_woken = false;
	return buf_len;

out_fail:
	if(headroom) *headroom = 0;
	if(task_woken) *task_woken = false;
	return 0;
}

static void PIOS_Crossfire_ResetBuffer(struct pios_crossfire_dev *dev)
{
	dev->buf_pos = 0;
	dev->bytes_expected = CRSF_MAX_FRAMELEN;
}

static bool PIOS_Crossfire_UnpackFrame(struct pios_crossfire_dev *dev)
{
	// Currently there appears to be only RC channel messages.
	// We also only care about those for now.

	if(dev->u.frame.type == CRSF_FRAME_RCCHANNELS &&
		dev->u.frame.length == (CRSF_TYPE_LEN + CRSF_PAYLOAD_RCCHANNELS + CRSF_CRC_LEN)) {

		// If there's a valid type and it matches its payload length,
		// then proceed doing the heavy lifting.
		uint8_t crc = PIOS_CRC_updateCRC_TBS(0, &dev->u.frame.type, dev->u.frame.length - CRSF_CRC_LEN);

		if(crc == CRSF_CRCFIELD(dev->u.frame)) {
			// From pios_sbus.c
			uint8_t *s = dev->u.frame.payload;
			uint16_t *d = dev->channel_data;

			#define F(v,s) (((v) >> (s)) & 0x7ff)

			/* unroll channels 1-8 */
			d[0] = F(s[0] | s[1] << 8, 0);
			d[1] = F(s[1] | s[2] << 8, 3);
			d[2] = F(s[2] | s[3] << 8 | s[4] << 16, 6);
			d[3] = F(s[4] | s[5] << 8, 1);
			d[4] = F(s[5] | s[6] << 8, 4);
			d[5] = F(s[6] | s[7] << 8 | s[8] << 16, 7);
			d[6] = F(s[8] | s[9] << 8, 2);
			d[7] = F(s[9] | s[10] << 8, 5);

			/* unroll channels 9-16 */
			d[8] = F(s[11] | s[12] << 8, 0);
			d[9] = F(s[12] | s[13] << 8, 3);
			d[10] = F(s[13] | s[14] << 8 | s[15] << 16, 6);
			d[11] = F(s[15] | s[16] << 8, 1);
			d[12] = F(s[16] | s[17] << 8, 4);
			d[13] = F(s[17] | s[18] << 8 | s[19] << 16, 7);
			d[14] = F(s[19] | s[20] << 8, 2);
			d[15] = F(s[20] | s[21] << 8, 5);

			// RC control is still happening.
			dev->failsafe_timer = 0;

			PIOS_Crossfire_ResetBuffer(dev);

			return true;
		}
	}

	PIOS_Crossfire_ResetBuffer(dev);
	return false;
}

static void PIOS_Crossfire_Supervisor(uintptr_t context)
{
	struct pios_crossfire_dev *dev = (struct pios_crossfire_dev *)context;
	PIOS_Assert(PIOS_Crossfire_Validate(dev));

	// An RC channel type packet is 26 bytes, at 420kbit with stop bit, that's 557us.
	// Maximum packet size is 36 bytes, that's 772us.
	// Minimum frame timing is supposed to be 4ms, typical is however 6.67ms (150hz).
	// So if more than 1.6ms passed without communication, safe to say that a new
	// packet is inbound.
	if (++dev->rx_timer > 1)
		PIOS_Crossfire_ResetBuffer(dev);

	// Failsafe after 50ms.
	if (++dev->failsafe_timer > 32)
		PIOS_Crossfire_SetAllChannels(dev, PIOS_RCVR_TIMEOUT);
}

int PIOS_Crossfire_SendTelemetry(uintptr_t crsf_id, uint8_t *buf, uint8_t bytes)
{
	struct pios_crossfire_dev *dev = (struct pios_crossfire_dev*)crsf_id;

	// While telemetry ain't vital, if something's giving a wrong device handle,
	// assert anyway.
	if(!PIOS_Crossfire_Validate(dev))
		PIOS_Assert(0);

	// There should be a telemetry channel regardless. And sending NULL buffers
	// ain't nice.
	if(!dev->telem_com_id || !buf)
		PIOS_Assert(0);

	// Nothing to send? Fine!
	if(!bytes)
		return 0;

	uint32_t delay = PIOS_DELAY_DiffuS(dev->time_frame_start);
	if((delay > CRSF_TIMING_MAXFRAME) & (delay < (CRSF_TIMING_FRAMEDISTANCE-CRSF_TIMING_MAXFRAME))) {

		// We're still within the send window.
		PIOS_COM_SendBuffer(dev->telem_com_id, buf, (uint16_t)bytes);
		return 0;

	} else {
		return -1;
	}
}

bool PIOS_Crossfire_IsFailsafed(uintptr_t crsf_id)
{
	struct pios_crossfire_dev *dev = (struct pios_crossfire_dev*)crsf_id;

	if(!PIOS_Crossfire_Validate(dev))
		PIOS_Assert(0);

	return dev->failsafe_timer > 32;
}

#endif // PIOS_INCLUDE_CROSSFIRE

 /**
  * @}
  * @}
  */
