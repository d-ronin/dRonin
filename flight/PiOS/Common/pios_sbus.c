/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SBus Futaba S.Bus receiver functions
 * @brief Code to read Futaba S.Bus receiver serial stream
 * @{
 *
 * @file       pios_sbus.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @brief      Code to read Futaba S.Bus receiver serial stream
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
#include "pios_sbus_priv.h"

#if defined(PIOS_INCLUDE_SBUS)

/* Forward Declarations */
static int32_t PIOS_SBus_Get(uintptr_t rcvr_id, uint8_t channel);
static uint16_t PIOS_SBus_RxInCallback(uintptr_t context,
				       uint8_t *buf,
				       uint16_t buf_len,
				       uint16_t *headroom,
				       bool *need_yield);
static void PIOS_SBus_Supervisor(uintptr_t sbus_id);


/* Local Variables */
const struct pios_rcvr_driver pios_sbus_rcvr_driver = {
	.read = PIOS_SBus_Get,
};

enum pios_sbus_dev_magic {
	PIOS_SBUS_DEV_MAGIC = 0x53427573,
};

struct pios_sbus_state {
	uint16_t channel_data[PIOS_SBUS_NUM_INPUTS];
	uint8_t received_data[SBUS_FRAME_LENGTH - 2];
	uint8_t receive_timer;
	uint8_t failsafe_timer;
	uint8_t byte_count;
};

struct pios_sbus_dev {
	enum pios_sbus_dev_magic magic;
	struct pios_sbus_state state;
};

/* Allocate S.Bus device descriptor */
static struct pios_sbus_dev *PIOS_SBus_Alloc(void)
{
	struct pios_sbus_dev *sbus_dev;

	sbus_dev = (struct pios_sbus_dev *)PIOS_malloc(sizeof(*sbus_dev));
	if (!sbus_dev) return(NULL);

	sbus_dev->magic = PIOS_SBUS_DEV_MAGIC;
	return(sbus_dev);
}

/* Validate S.Bus device descriptor */
static bool PIOS_SBus_Validate(struct pios_sbus_dev *sbus_dev)
{
	return (sbus_dev->magic == PIOS_SBUS_DEV_MAGIC);
}

/* Reset channels in case of lost signal or explicit failsafe receiver flag */
static void PIOS_SBus_ResetChannels(struct pios_sbus_state *state)
{
	for (int i = 0; i < PIOS_SBUS_NUM_INPUTS; i++) {
		state->channel_data[i] = PIOS_RCVR_TIMEOUT;
	}
}

/* Reset S.Bus receiver state */
static void PIOS_SBus_ResetState(struct pios_sbus_state *state)
{
	state->failsafe_timer = 0;
	state->receive_timer = 0;
	state->byte_count = 0;
	PIOS_SBus_ResetChannels(state);
}

/* Initialise S.Bus receiver interface */
int32_t PIOS_SBus_Init(uintptr_t *sbus_id,
		       const struct pios_com_driver *driver,
		       uintptr_t lower_id)
{
	PIOS_DEBUG_Assert(sbus_id);
	PIOS_DEBUG_Assert(cfg);
	PIOS_DEBUG_Assert(driver);

	struct pios_sbus_dev *sbus_dev;

	sbus_dev = (struct pios_sbus_dev *)PIOS_SBus_Alloc();
	if (!sbus_dev) return -1;

	PIOS_SBus_ResetState(&(sbus_dev->state));

	*sbus_id = (uintptr_t)sbus_dev;

	(driver->bind_rx_cb)(lower_id, PIOS_SBus_RxInCallback, *sbus_id);

	if (!PIOS_RTC_RegisterTickCallback(PIOS_SBus_Supervisor, *sbus_id)) {
		PIOS_DEBUG_Assert(0);
	}

	return 0;
}

/**
 * Get the value of an input channel
 * \param[in] channel Number of the channel desired (zero based)
 * \output PIOS_RCVR_INVALID channel not available
 * \output PIOS_RCVR_TIMEOUT failsafe condition or missing receiver
 * \output >=0 channel value
 */
static int32_t PIOS_SBus_Get(uintptr_t rcvr_id, uint8_t channel)
{
	struct pios_sbus_dev *sbus_dev = (struct pios_sbus_dev *)rcvr_id;

	if (!PIOS_SBus_Validate(sbus_dev))
		return PIOS_RCVR_INVALID;

	/* return error if channel is not available */
	if (channel >= PIOS_SBUS_NUM_INPUTS) {
		return PIOS_RCVR_INVALID;
	}

	return sbus_dev->state.channel_data[channel];
}

/**
 * Compute channel_data[] from received_data[].
 * For efficiency it unrolls first 8 channels without loops and does the
 * same for other 8 channels. Also 2 discrete channels will be set.
 */
static void PIOS_SBus_UnrollChannels(struct pios_sbus_state *state)
{
	uint8_t *s = state->received_data;
	uint16_t *d = state->channel_data;

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

	/* unroll discrete channels 17 and 18 */
	d[16] = (s[22] & SBUS_FLAG_DC1) ? SBUS_VALUE_MAX : SBUS_VALUE_MIN;
	d[17] = (s[22] & SBUS_FLAG_DC2) ? SBUS_VALUE_MAX : SBUS_VALUE_MIN;
}

/* Update decoder state processing input byte from the S.Bus stream */
static void PIOS_SBus_UpdateState(struct pios_sbus_state *state, uint8_t b)
{
	if (state->byte_count == 0) {
		if (b == SBUS_SOF_BYTE) {
			/* do not store the SOF byte */
			state->byte_count++;
		}
		/* Otherwise discard this byte. */
		return;
	}

	/* do not store last frame byte as well */
	if (state->byte_count < SBUS_FRAME_LENGTH - 1) {
		/* store next byte */
		state->received_data[state->byte_count - 1] = b;
		state->byte_count++;
	} else {
		if (b == SBUS_EOF_BYTE || (b & SBUS_R7008SB_EOF_COUNTER_MASK) == SBUS_R7008SB_EOF_BYTE) {
			/* full frame received */
			uint8_t flags = state->received_data[SBUS_FRAME_LENGTH - 3];
			if (flags & SBUS_FLAG_FL) {
				/* frame lost, do not update */
			} else if (flags & SBUS_FLAG_FS) {
				/* failsafe flag active */
				PIOS_SBus_ResetChannels(state);
			} else {
				/* data looking good */
				PIOS_SBus_UnrollChannels(state);
				state->failsafe_timer = 0;
				PIOS_RCVR_ActiveFromISR();
			}
		} else {
			/* discard whole frame */
		}

		/* prepare for the next frame */
		state->byte_count = 0;
	}
}

/* Comm byte received callback */
static uint16_t PIOS_SBus_RxInCallback(uintptr_t context,
				       uint8_t *buf,
				       uint16_t buf_len,
				       uint16_t *headroom,
				       bool *need_yield)
{
	struct pios_sbus_dev *sbus_dev = (struct pios_sbus_dev *)context;

	bool valid = PIOS_SBus_Validate(sbus_dev);
	PIOS_Assert(valid);

	struct pios_sbus_state *state = &(sbus_dev->state);

	/* process byte(s) and clear receive timer */
	for (uint8_t i = 0; i < buf_len; i++) {
		PIOS_SBus_UpdateState(state, buf[i]);
	}

	state->receive_timer = 0;

	/* Always signal that we can accept another frame */
	if (headroom)
		*headroom = SBUS_FRAME_LENGTH;

	/* We never need a yield */
	*need_yield = false;

	/* Always indicate that all bytes were consumed */
	return buf_len;
}

/**
 * Input data supervisor is called periodically and provides
 * failsafe triggering.
 *
 * S.Bus frames come at 7ms (HS) or 14ms (FS) rate at 100000bps.
 * RTC timer is running at 625Hz (1.6ms).
 *
 * Data receive function must clear the receive_timer to confirm new
 * data reception. If no new data received in 48ms, we must call the
 * failsafe function which clears all channels.
 */
static void PIOS_SBus_Supervisor(uintptr_t sbus_id)
{
	struct pios_sbus_dev *sbus_dev = (struct pios_sbus_dev *)sbus_id;

	bool valid = PIOS_SBus_Validate(sbus_dev);
	PIOS_Assert(valid);

	struct pios_sbus_state *state = &(sbus_dev->state);

	/* An appropriate gap of at least 3.2ms causes us to go back to
	 * expecting start of frame. */
	if (++state->receive_timer > 2) {
		state->byte_count = 0;
		state->receive_timer = 0;
	}

	/* Activate failsafe if no frames have arrived in ~250ms. */
	if (++state->failsafe_timer > 156) {
		PIOS_SBus_ResetChannels(state);
		state->failsafe_timer = 0;
	}
}

#endif	/* PIOS_INCLUDE_SBUS */

/** 
 * @}
 * @}
 */
