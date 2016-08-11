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

#include "actuatorcommand.h"

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
};

//! Global structure for this device device
static struct pios_flyingpio_dev *fpio_dev;

static struct flyingpi_msg actuator_cfg;
static float channel_values[FPPROTO_MAX_SERVOS];

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

static void PIOS_FLYINGPIO_ActuatorUpdate();
static void PIOS_FLYINGPIO_ActuatorSetMode(const uint16_t *out_rate,
	const int banks, const uint16_t *channel_max,
	const uint16_t *channel_min);
static void PIOS_FLYINGPIO_ActuatorSet(uint8_t servo, float position);

const struct pios_servo_callbacks flyingpio_callbacks = {
	.update = PIOS_FLYINGPIO_ActuatorUpdate,
	.set_mode = PIOS_FLYINGPIO_ActuatorSetMode,
	.set = PIOS_FLYINGPIO_ActuatorSet
};


static struct pios_flyingpio_dev *PIOS_FLYINGPIO_Alloc()
{
	struct pios_flyingpio_dev *dev;

	dev = (struct pios_flyingpio_dev *)PIOS_malloc(sizeof(*fpio_dev));
	if (!dev)
		return NULL;

	dev->magic = PIOS_FLYINGPIO_DEV_MAGIC;

	PIOS_Servo_SetCallbacks(&flyingpio_callbacks);

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

	return 0;
}

static void PIOS_FLYINGPIO_SendCmd(struct flyingpi_msg *msg) {
	int len = 0;

	flyingpi_calc_crc(msg, true, &len);

	if (PIOS_SPI_ClaimBus(fpio_dev->spi_id) != 0)
		return;		/* No bueno. */

	PIOS_SPI_SetClockSpeed(fpio_dev->spi_id, 15000000);

	PIOS_SPI_RC_PinSet(fpio_dev->spi_id, fpio_dev->spi_slave,
		false);

	if (!PIOS_SPI_TransferBlock(fpio_dev->spi_id, (uint8_t *)msg, NULL, len)) {
	}

	PIOS_SPI_RC_PinSet(fpio_dev->spi_id, fpio_dev->spi_slave,
		true);
	PIOS_SPI_ReleaseBus(fpio_dev->spi_id);

	/* XXX handle response data */
}

static void PIOS_FLYINGPIO_ActuatorUpdate()
{
	/* Make sure we've already been configured */
	PIOS_Assert(actuator_cfg.id == FLYINGPICMD_CFG);

	static int upd_num = 0;

	upd_num++;

	PIOS_Assert(!PIOS_FLYINGPIO_Validate(fpio_dev));

	struct flyingpi_msg tx_buf;

	tx_buf.id = FLYINGPICMD_ACTUATOR;

	struct flyingpicmd_actuator_fc *cmd = &tx_buf.body.actuator_fc;
	struct flyingpicmd_cfg_fa *cfg = &actuator_cfg.body.cfg_fa;

	for (int i=0; i < FPPROTO_MAX_SERVOS; i++) {
		uint16_t span = cfg->actuators[i].max - cfg->actuators[i].min;
		if (span == 0) {
			// Prevent div by 0, this is a simple case.
			cmd->values[i] = cfg->actuators[i].max;
		} else {
			float fraction = channel_values[i] -
				cfg->actuators[i].min;

			fraction /= span;
			fraction *= 65535.0f;
			fraction += 0.5f;

			if (fraction > 65535) {
				fraction = 65535;
			} else if (fraction < 0) {
				fraction = 0;
			}

			cmd->values[i] = fraction;

#if 0
			if (!(upd_num & 1023)) {
				printf("%d %f\n", i, fraction);
			}
#endif
		}
	}

	tx_buf.body.actuator_fc.led_status =
		PIOS_LED_GetStatus(PIOS_LED_HEARTBEAT);

	PIOS_FLYINGPIO_SendCmd(&tx_buf);
}

static void PIOS_FLYINGPIO_ActuatorSetMode(const uint16_t *out_rate,
	const int banks, const uint16_t *channel_max,
	const uint16_t *channel_min) {

	actuator_cfg.id = FLYINGPICMD_CFG;

	struct flyingpicmd_cfg_fa *cmd = &actuator_cfg.body.cfg_fa;

	PIOS_Assert(banks >= FPPROTO_MAX_BANKS);

	for (int i=0; i < FPPROTO_MAX_BANKS; i++) {
		cmd->rate[i] = out_rate[i];
	}

	for (int i=0; i < ACTUATORCOMMAND_CHANNEL_NUMELEM; i++) {
		uint16_t true_min, true_max;

		if (channel_min[i] > channel_max[i]) {
			true_min = channel_max[i];
			true_max = channel_min[i];
		} else {
			true_min = channel_min[i];
			true_max = channel_max[i];
		}

		cmd->actuators[i].min = true_min;
		cmd->actuators[i].max = true_max;
	}

	cmd->receiver_protocol = 0;

	PIOS_FLYINGPIO_SendCmd(&actuator_cfg);
}

static void PIOS_FLYINGPIO_ActuatorSet(uint8_t servo, float position) {
	PIOS_Assert(servo < FPPROTO_MAX_SERVOS);

	channel_values[servo] = position;
}

#endif // PIOS_INCLUDE_FLYINGPIO

/**
 * @}
 * @}
 */
