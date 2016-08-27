/**
 ******************************************************************************
 * @file       pios_bmm150.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMM150
 * @{
 * @brief Driver for Bosch BMM150 IMU Sensor (part of BMX055)
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

#if defined(PIOS_INCLUDE_BMM150)

#include "pios_semaphore.h"
#include "pios_thread.h"
#include "pios_queue.h"
#include "physical_constants.h"
#include "taskmonitor.h"

#include "pios_bmm150_priv.h"

#define PIOS_BMM_TASK_PRIORITY   PIOS_THREAD_PRIO_HIGHEST

#define PIOS_BMM_TASK_STACK      640

#define PIOS_BMM_QUEUE_LEN       2

#define PIOS_BMM_SPI_SPEED       10000000


/**
 * Magic byte sequence used to validate the device state struct.
 * Should be unique amongst all PiOS drivers!
 */
enum pios_bmm150_dev_magic {
	PIOS_BMM_DEV_MAGIC = 0x586e6f42 /**< Unique byte sequence 'BnoX' */
};

/**
 * @brief The device state struct
 */
struct pios_bmm150_dev {
	enum pios_bmm150_dev_magic magic;              /**< Magic bytes to validate the struct contents */
	const struct pios_bmm150_cfg *cfg;             /**< Device configuration structure */
	uint32_t spi_id;                            /**< Handle to the communication driver */
	uint32_t spi_slave_mag;                    /**< The slave number (SPI) */

	struct pios_queue *mag_queue;

	struct pios_thread *task_handle;
};

//! Global structure for this device device
static struct pios_bmm150_dev *bmm_dev;

//! Private functions
/**
 * @brief Allocate a new device
 */
static struct pios_bmm150_dev *PIOS_BMM_Alloc(const struct pios_bmm150_cfg *cfg);

/**
 * @brief Validate the handle to the device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_BMM_Validate(struct pios_bmm150_dev *dev);

static void PIOS_BMM_Task(void *parameters);

/**
 * @brief Claim the SPI bus for the communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_BMM_ClaimBus();

/**
 * @brief Release the SPI bus for the communications and end the transaction
 * @return 0 if successful
 */
static int32_t PIOS_BMM_ReleaseBus();

static int32_t PIOS_BMM_ReadReg(int slave, uint8_t address, uint8_t *buffer);
static int32_t PIOS_BMM_WriteReg(int slave, uint8_t address, uint8_t buffer);

static struct pios_bmm150_dev *PIOS_BMM_Alloc(const struct pios_bmm150_cfg *cfg)
{
	struct pios_bmm150_dev *dev;

	dev = (struct pios_bmm150_dev *)PIOS_malloc(sizeof(*bmm_dev));
	if (!dev)
		return NULL;

	dev->magic = PIOS_BMM_DEV_MAGIC;

	dev->mag_queue = PIOS_Queue_Create(PIOS_BMM_QUEUE_LEN, sizeof(struct pios_sensor_mag_data));
	if (dev->mag_queue == NULL) {
		PIOS_free(dev);
		return NULL;
	}

	return dev;
}

static int32_t PIOS_BMM_Validate(struct pios_bmm150_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_BMM_DEV_MAGIC)
		return -2;
	if (dev->spi_id == 0)
		return -3;
	return 0;
}

static int32_t AssertReg(int slave, uint8_t address, uint8_t expect) {
	uint8_t c;

	int32_t ret = PIOS_BMM_ReadReg(slave, address, &c);

	if (ret) {
		return ret;
	}

	if (c != expect) {
		DEBUG_PRINTF(2, "BMM: Assertion failed: *(%d:%02x) == %02x (expect %02x)\n",
		slave, address, c, expect);
		return -1;
	}

	DEBUG_PRINTF(2, "BMM: Assertion passed: *(%d:%02x) == %02x\n", slave, address,
		expect);

	return 0;
}

int32_t PIOS_BMM150_SPI_Init(pios_bmm150_dev_t *dev, uint32_t spi_id, 
		uint32_t slave_mag, const struct pios_bmm150_cfg *cfg)
{
	bmm_dev = PIOS_BMM_Alloc(cfg);
	if (bmm_dev == NULL)
		return -1;
	*dev = bmm_dev;

	bmm_dev->spi_id = spi_id;
	bmm_dev->spi_slave_mag = slave_mag;
	bmm_dev->cfg = cfg;

	int32_t ret;

	/* Unfortunately can't check mag right away, so proceed with
	 * init
	 */

	DEBUG_PRINTF(2, "BMM: Resetting sensor\n");

	ret = PIOS_BMM_WriteReg(bmm_dev->spi_slave_mag,
			BMM150_REG_MAG_POWER_CONTROL,
			BMM150_VAL_MAG_POWER_CONTROL_POWEROFF);

	if (ret) return ret;

	PIOS_DELAY_WaitmS(20);

	ret = PIOS_BMM_WriteReg(bmm_dev->spi_slave_mag,
			BMM150_REG_MAG_POWER_CONTROL,
			BMM150_VAL_MAG_POWER_CONTROL_POWERON);

	if (ret) return ret;

	PIOS_DELAY_WaitmS(30);

	/* Verify expected chip id. */	
	ret = AssertReg(bmm_dev->spi_slave_mag,
			BMM150_REG_MAG_CHIPID,
			BMM150_VAL_MAG_CHIPID);

	if (ret) return ret;

	bmm_dev->task_handle = PIOS_Thread_Create(
			PIOS_BMM_Task, "pios_bmm", PIOS_BMM_TASK_STACK,
			NULL, PIOS_BMM_TASK_PRIORITY);

	PIOS_SENSORS_Register(PIOS_SENSOR_MAG, bmm_dev->mag_queue);

	return 0;
}

static int32_t PIOS_BMM_ClaimBus(int slave)
{
	if (PIOS_BMM_Validate(bmm_dev) != 0)
		return -1;

	if (PIOS_SPI_ClaimBus(bmm_dev->spi_id) != 0)
		return -2;

	PIOS_SPI_RC_PinSet(bmm_dev->spi_id, bmm_dev->spi_slave_mag, false);

	return 0;
}

static int32_t PIOS_BMM_ReleaseBus(int slave)
{
	if (PIOS_BMM_Validate(bmm_dev) != 0)
		return -1;

	PIOS_SPI_RC_PinSet(bmm_dev->spi_id, bmm_dev->spi_slave_mag, true);

	PIOS_SPI_ReleaseBus(bmm_dev->spi_id);

	return 0;
}

static int32_t PIOS_BMM_ReadReg(int slave, uint8_t address, uint8_t *buffer)
{
	if (PIOS_BMM_ClaimBus(slave) != 0)
		return -1;

	PIOS_SPI_TransferByte(bmm_dev->spi_id, 0x80 | address); // request byte
	*buffer = PIOS_SPI_TransferByte(bmm_dev->spi_id, 0);   // receive response

	PIOS_BMM_ReleaseBus(slave);

	return 0;
}

static int32_t PIOS_BMM_WriteReg(int slave, uint8_t address, uint8_t buffer)
{
	if (PIOS_BMM_ClaimBus(slave) != 0)
		return -1;

	PIOS_SPI_TransferByte(bmm_dev->spi_id, 0x7f & address);
	PIOS_SPI_TransferByte(bmm_dev->spi_id, buffer);

	PIOS_BMM_ReleaseBus(slave);

	return 0;
}

static void PIOS_BMM_Task(void *parameters)
{
	(void)parameters;

	while (true) {
		PIOS_Thread_Sleep(30);		/* XXX */

		// claim bus in high speed mode
		if (PIOS_SPI_ClaimBus(bmm_dev->spi_id) != 0)
			continue;

		PIOS_SPI_SetClockSpeed(bmm_dev->spi_id, 10000000);

		struct pios_sensor_mag_data mag_data;

#define PACK_REG12_ADDR_OFFSET(b, reg, off) (int16_t) ( (b[reg-off] & 0xf0) | (b[reg-off+1] << 8) )
#define PACK_REG16_ADDR_OFFSET(b, reg, off) (int16_t) ( (b[reg-off]) | (b[reg-off+1] << 8) )

		// XXX TODO Verify all this
		float mag_x = 0, mag_y = 0, mag_z = 0;

		switch (bmm_dev->cfg->orientation) {
		case PIOS_BMM_TOP_0DEG:
			mag_data.x   =  mag_x;
			mag_data.y   =  mag_y;
			mag_data.z   =  mag_z;
			break;
		case PIOS_BMM_TOP_90DEG:
			mag_data.x   = -mag_y;
			mag_data.y   =  mag_x;
			mag_data.z   =  mag_z;
			break;
		case PIOS_BMM_TOP_180DEG:
			mag_data.x   = -mag_x;
			mag_data.y   = -mag_y;
			mag_data.z   =  mag_z;
			break;
		case PIOS_BMM_TOP_270DEG:
			mag_data.x   =  mag_y;
			mag_data.y   = -mag_x;
			mag_data.z   =  mag_z;
			break;
		case PIOS_BMM_BOTTOM_0DEG:
			mag_data.x   =  mag_x;
			mag_data.y   = -mag_y;
			mag_data.z   = -mag_z;
			break;
		case PIOS_BMM_BOTTOM_90DEG:
			mag_data.x   = -mag_y;
			mag_data.y   = -mag_x;
			mag_data.z   = -mag_z;
			break;
		case PIOS_BMM_BOTTOM_180DEG:
			mag_data.x   = -mag_x;
			mag_data.y   =  mag_y;
			mag_data.z   = -mag_z;
			break;
		case PIOS_BMM_BOTTOM_270DEG:
			mag_data.x   =  mag_y;
			mag_data.y   =  mag_x;
			mag_data.z   = -mag_z;
			break;
		}

		float mag_scale = 33.0f;	/* XXX */
		mag_data.x *= mag_scale;
		mag_data.y *= mag_scale;
		mag_data.z *= mag_scale;
		PIOS_Queue_Send(bmm_dev->mag_queue, &mag_data, 0);
	}
}

#endif // PIOS_INCLUDE_BMM150

/**
 * @}
 * @}
 */
