/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup  PIOS_UAVTALKRCVR Receiver-over-UAVTALK Input Functions
 * @brief	Code to read the channels within the Receiver UAVObject
 * @{
 *
 * @file       pios_uavtalkrcvr.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2018
 * @brief      GCS/UAVTalk Input functions
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

#if defined(PIOS_INCLUDE_UAVTALKRCVR)

#include "pios_uavtalkrcvr_priv.h"

#ifndef PIOS_UAVTALKRCVR_TIMEOUT_MS
#define PIOS_UAVTALKRCVR_TIMEOUT_MS			350
#endif

static UAVTalkReceiverData uavreceiverdata;

/* Provide a RCVR driver */
static int32_t PIOS_UAVTALKRCVR_Get(uintptr_t rcvr_id, uint8_t channel);
static void PIOS_uavtalkrcvr_Supervisor(uintptr_t rcvr_id);

const struct pios_rcvr_driver pios_uavtalk_rcvr_driver = {
	.read = PIOS_UAVTALKRCVR_Get,
};

/* Local Variables */
enum pios_uavtalkrcvr_dev_magic {
	PIOS_UAVTALKRCVR_DEV_MAGIC = 0xe9da5c56,
};

struct pios_uavtalkrcvr_dev {
	enum pios_uavtalkrcvr_dev_magic magic;

	uint16_t supv_timer;
	volatile bool Fresh;
};

static struct pios_uavtalkrcvr_dev *global_uavtalkrcvr_dev;

static bool PIOS_uavtalkrcvr_validate(struct pios_uavtalkrcvr_dev *uavtalkrcvr_dev)
{
	return (uavtalkrcvr_dev->magic == PIOS_UAVTALKRCVR_DEV_MAGIC);
}

static struct pios_uavtalkrcvr_dev *PIOS_uavtalkrcvr_alloc(void)
{
	struct pios_uavtalkrcvr_dev * uavtalkrcvr_dev;

	uavtalkrcvr_dev = (struct pios_uavtalkrcvr_dev *)PIOS_malloc(sizeof(*uavtalkrcvr_dev));
	if (!uavtalkrcvr_dev) return(NULL);

	uavtalkrcvr_dev->magic = PIOS_UAVTALKRCVR_DEV_MAGIC;
	uavtalkrcvr_dev->Fresh = false;
	uavtalkrcvr_dev->supv_timer = 0;

	/* The update callback cannot receive the device pointer, so set it in a global */
	global_uavtalkrcvr_dev = uavtalkrcvr_dev;

	return(uavtalkrcvr_dev);
}

static void uavreceiver_updated(const UAVObjEvent *ev,
		void *ctx, void *obj, int len)
{
	struct pios_uavtalkrcvr_dev *uavtalkrcvr_dev = global_uavtalkrcvr_dev;
	if (ev->obj == UAVTalkReceiverHandle()) {
		PIOS_RCVR_Active();
		UAVTalkReceiverGet(&uavreceiverdata);

		uavtalkrcvr_dev->Fresh = true;
	}
}

extern int32_t PIOS_UAVTALKRCVR_Init(uintptr_t *uavtalkrcvr_id)
{
	struct pios_uavtalkrcvr_dev *uavtalkrcvr_dev;

	/* Allocate the device structure */
	uavtalkrcvr_dev = (struct pios_uavtalkrcvr_dev *)PIOS_uavtalkrcvr_alloc();
	if (!uavtalkrcvr_dev)
		return -1;

	for (uint8_t i = 0; i < UAVTALKRECEIVER_CHANNEL_NUMELEM; i++) {
		/* Flush channels */
		uavreceiverdata.Channel[i] = PIOS_RCVR_TIMEOUT;
	}

	/* Register uavobj callback */
	UAVTalkReceiverConnectCallback(uavreceiver_updated);

	/* Register the failsafe timer callback. */
	if (!PIOS_RTC_RegisterTickCallback(PIOS_uavtalkrcvr_Supervisor, (uintptr_t)uavtalkrcvr_dev)) {
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
static int32_t PIOS_UAVTALKRCVR_Get(uintptr_t rcvr_id, uint8_t channel)
{
	if (channel >= UAVTALKRECEIVER_CHANNEL_NUMELEM) {
		/* channel is out of range */
		return PIOS_RCVR_INVALID;
	}

	return (uavreceiverdata.Channel[channel]);
}

static void PIOS_uavtalkrcvr_Supervisor(uintptr_t uavtalkrcvr_id) {
	/* Recover our device context */
	struct pios_uavtalkrcvr_dev * uavtalkrcvr_dev = (struct pios_uavtalkrcvr_dev *)uavtalkrcvr_id;

	if (!PIOS_uavtalkrcvr_validate(uavtalkrcvr_dev)) {
		/* Invalid device specified */
		return;
	}

	/* Compare RTC ticks to timeout in ms */
	if ((++uavtalkrcvr_dev->supv_timer) <
			(PIOS_UAVTALKRCVR_TIMEOUT_MS * 1000 / PIOS_RTC_Rate())) {
		return;
	}
	uavtalkrcvr_dev->supv_timer = 0;

	if (!uavtalkrcvr_dev->Fresh) {
		for (int32_t i = 0; i < UAVTALKRECEIVER_CHANNEL_NUMELEM; i++)
			uavreceiverdata.Channel[i] = PIOS_RCVR_TIMEOUT;
	}

	uavtalkrcvr_dev->Fresh = false;
}

#endif	/* PIOS_INCLUDE_UAVTALKRCVR */

/** 
  * @}
  * @}
  */
