/**
 ******************************************************************************
 * @file       pios_bmp280.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMP280 BMP280 Functions
 * @{
 * @brief Driver for BMP280
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

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_BMP280)

#include "pios_bmp280_priv.h"
#include "pios_semaphore.h"
#include "pios_thread.h"
#include "pios_queue.h"

/* Private constants */
#define BMP280_TASK_PRIORITY	PIOS_THREAD_PRIO_HIGHEST
#define BMP280_TASK_STACK		512

/* BMP280 Addresses */
#define BMP280_I2C_ADDR		0x76  // 0x77
#define BMP280_ID		0xD0
#define BMP280_RESET		0xE0
#define BMP280_STATUS		0xF3
#define BMP280_CTRL_MEAS	0xF4
#define BMP280_CONFIG		0xF5
#define BMP280_PRESS_MSB	0xF7
#define BMP280_PRESS_LSB	0xF8
#define BMP280_PRESS_XLSB	0xF9
#define BMP280_TEMP_MSB		0xFA
#define BMP280_TEMP_LSB		0xFB
#define BMP280_TEMP_XLSB	0xFC

#define BMP280_CAL_ADDR		0x88

#define BMP280_P0            101.3250f

#define PIOS_BMP_SPI_SPEED 9500000	/* Just shy of 10MHz */

/* Private methods */
static int32_t PIOS_BMP280_Read(uint8_t address, uint8_t *buffer, uint8_t len);
static int32_t PIOS_BMP280_WriteCommand(uint8_t address, uint8_t buffer);
static void PIOS_BMP280_Task(void *parameters);

/* Private types */

/* Local Types */

enum pios_bmp280_dev_magic {
	PIOS_BMP280_DEV_MAGIC = 0x32504d42,  /* 'BMP2' */
};

struct bmp280_dev {
	const struct pios_bmp280_cfg *cfg;
#ifndef PIOS_EXCLUDE_BMP280_I2C
	uint32_t i2c_id;
#endif
#ifdef PIOS_INCLUDE_BMP280_SPI
	uint32_t spi_id;
	uint32_t spi_slave;
#endif

	struct pios_thread *task;
	struct pios_queue *queue;

	uint32_t compensatedPressure;
	int32_t  compensatedTemperature;

	uint16_t digT1;
	int16_t  digT2;
	int16_t  digT3;
	uint16_t digP1;
	int16_t  digP2;
	int16_t  digP3;
	int16_t  digP4;
	int16_t  digP5;
	int16_t  digP6;
	int16_t  digP7;
	int16_t  digP8;
	int16_t  digP9;

	uint8_t oversampling;
	uint32_t temperature_interleaving;
	enum pios_bmp280_dev_magic magic;

	struct pios_semaphore *busy;
};

static struct bmp280_dev *dev;

/**
 * @brief Allocate a new device
 */
static struct bmp280_dev *PIOS_BMP280_alloc(void)
{
	struct bmp280_dev *bmp280_dev;

	bmp280_dev = (struct bmp280_dev *)PIOS_malloc(sizeof(*bmp280_dev));
	if (!bmp280_dev) return (NULL);

	bzero(bmp280_dev, sizeof(*bmp280_dev));

	bmp280_dev->queue = PIOS_Queue_Create(1, sizeof(struct pios_sensor_baro_data));
	if (bmp280_dev->queue == NULL) {
		PIOS_free(bmp280_dev);
		return NULL;
	}

	bmp280_dev->busy = PIOS_Semaphore_Create();

	bmp280_dev->magic = PIOS_BMP280_DEV_MAGIC;

	return(bmp280_dev);
}

/**
 * @brief Validate the handle to the i2c device
 * @returns 0 for valid device or <0 otherwise
 */
static int32_t PIOS_BMP280_Validate(struct bmp280_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_BMP280_DEV_MAGIC)
		return -2;
	return 0;
}

static int32_t PIOS_BMP280_Init_Common(const struct pios_bmp280_cfg *cfg)
{
	dev->oversampling = cfg->oversampling;
	dev->temperature_interleaving = (cfg->temperature_interleaving) == 0 ? 1 : cfg->temperature_interleaving;
	dev->cfg = cfg;

	uint8_t data[24];
	if (PIOS_BMP280_Read(BMP280_CAL_ADDR, data, 2) != 0)
		return -2;
	dev->digT1 = (data[ 1] << 8) | data[ 0];
	if (PIOS_BMP280_Read(BMP280_CAL_ADDR, data, 2) != 0)
		return -2;
	dev->digT1 = (data[ 1] << 8) | data[ 0];

	if (PIOS_BMP280_Read(BMP280_CAL_ADDR, data, 24) != 0)
		return -2;

	dev->digT1 = (data[ 1] << 8) | data[ 0];
	dev->digT2 = (data[ 3] << 8) | data[ 2];
	dev->digT3 = (data[ 5] << 8) | data[ 4];
	dev->digP1 = (data[ 7] << 8) | data[ 6];
	dev->digP2 = (data[ 9] << 8) | data[ 8];
	dev->digP3 = (data[11] << 8) | data[10];
	dev->digP4 = (data[13] << 8) | data[12];
	dev->digP5 = (data[15] << 8) | data[14];
	dev->digP6 = (data[17] << 8) | data[16];
	dev->digP7 = (data[19] << 8) | data[18];
	dev->digP8 = (data[21] << 8) | data[20];
	dev->digP9 = (data[23] << 8) | data[22];

	if (PIOS_BMP280_WriteCommand(BMP280_CONFIG, 0x00) != 0)
		return -2;

	dev->task = PIOS_Thread_Create(
		PIOS_BMP280_Task, "pios_bmp280", BMP280_TASK_STACK, NULL, BMP280_TASK_PRIORITY);
	if (dev->task == NULL)
		return -3;

	PIOS_SENSORS_Register(PIOS_SENSOR_BARO, dev->queue);

	return 0;
}

/**
 * Initialise the BMP280 sensor
 */
#ifndef PIOS_EXCLUDE_BMP280_I2C
int32_t PIOS_BMP280_Init(const struct pios_bmp280_cfg *cfg, int32_t i2c_device)
{
	dev = (struct bmp280_dev *) PIOS_BMP280_alloc();
	if (dev == NULL)
		return -1;

	dev->i2c_id = i2c_device;

	return PIOS_BMP280_Init_Common(cfg);
}
#endif /* PIOS_EXCLUDE_BMP280_I2C */

#ifdef PIOS_INCLUDE_BMP280_SPI
int32_t PIOS_BMP280_Init_SPI(const struct pios_bmp280_cfg *cfg, int32_t spi_device,
	int spi_slave)
{
	dev = (struct bmp280_dev *) PIOS_BMP280_alloc();
	if (dev == NULL)
		return -1;

	dev->spi_id = spi_device;
	dev->spi_slave = spi_slave;

	return PIOS_BMP280_Init_Common(cfg);
}
#endif /* PIOS_INCLUDE_BMP280_SPI */

/**
 * Claim the BMP280 device semaphore.
 * \return 0 if no error
 * \return -1 if timeout before claiming semaphore
 */
static int32_t PIOS_BMP280_ClaimDevice(void)
{
	PIOS_Assert(PIOS_BMP280_Validate(dev) == 0);

	return PIOS_Semaphore_Take(dev->busy, PIOS_SEMAPHORE_TIMEOUT_MAX) == true ? 0 : 1;
}

/**
 * Release the BMP280 device semaphore.
 * \return 0 if no error
 */
static int32_t PIOS_BMP280_ReleaseDevice(void)
{
	PIOS_Assert(PIOS_BMP280_Validate(dev) == 0);

	return PIOS_Semaphore_Give(dev->busy) == true ? 0 : 1;
}


/**
* Start the ADC conversion
* \param[in] PRESSURE_CONV or TEMPERATURE_CONV to select which measurement to make
* \return 0 for success, -1 for failure (to start conversion)
*/
static int32_t PIOS_BMP280_StartADC(void)
{
	if (PIOS_BMP280_Validate(dev) != 0)
		return -1;

	/* Start the conversion */
	return(PIOS_BMP280_WriteCommand(BMP280_CTRL_MEAS, dev->oversampling));

	return 0;
}

/**
 * @brief Return the delay for the current osr
 */
static int32_t PIOS_BMP280_GetDelay() {
	if (PIOS_BMP280_Validate(dev) != 0)
		return 100;

	switch(dev->oversampling) {
		case BMP280_STANDARD_RESOLUTION:
			return 14;
		case BMP280_HIGH_RESOLUTION:
			return 23;
		case BMP280_ULTRA_HIGH_RESOLUTION:
			return 44;
		default:
			break;
	}
	return 44;
}

/**
* Read the ADC conversion value (once ADC conversion has completed)
* \return 0 if successfully read the ADC, -1 if failed
*/
static int32_t PIOS_BMP280_ReadADC(bool calcTemp)
{
	if (PIOS_BMP280_Validate(dev) != 0)
		return -1;

	uint8_t data[6];

	/* Read and store results */

	if (PIOS_BMP280_Read(BMP280_PRESS_MSB, data, 6) != 0)
			return -1;

	if (calcTemp) {
		int32_t raw_temperature = (int32_t)((((uint32_t)(data[3])) << 12) | (((uint32_t)(data[4])) << 4) | ((uint32_t)data[5] >> 4));

		int32_t varT1, varT2, T;

		varT1 =  ((((raw_temperature >> 3) - ((int32_t)dev->digT1 << 1))) * ((int32_t)dev->digT2)) >> 11;
		varT2 = (((((raw_temperature >> 4) - ((int32_t)dev->digT1)) * ((raw_temperature >> 4) - ((int32_t)dev->digT1))) >> 12) * ((int32_t)dev->digT3)) >> 14;
		T = varT1 + varT2;
		dev->compensatedTemperature = (T * 5 + 128) >> 8;
	}

	int32_t raw_pressure = (int32_t)((((uint32_t)(data[0])) << 12) | (((uint32_t)(data[1])) << 4) | ((uint32_t)data[2] >> 4));

	int64_t varP1, varP2, P;

	varP1 = ((int64_t)T) - 128000;
	varP2 = varP1 * varP1 * (int64_t)dev->digP6;
	varP2 = varP2 + ((varP1 * (int64_t)dev->digP5) << 17);
	varP2 = varP2 + (((int64_t)dev->digP4) << 35);
	varP1= ((varP1 * varP1 * (int64_t)dev->digP3) >> 8) + ((varP1 * (int64_t)dev->digP2) << 12);
	varP1 = (((((int64_t)1) << 47) + varP1)) * ((int64_t)dev->digP1) >> 33;
	if (varP1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}
	P = 1048576 - raw_pressure;
	P = (((P << 31) - varP2) * 3125) / varP1;
	varP1 = (((int64_t)dev->digP9) * (P >> 13) * (P >> 13)) >> 25;
	varP2 = (((int64_t)dev->digP8) * P) >> 19;
	dev->compensatedPressure = (uint32_t)((P + varP1 + varP2) >> 8) + (((int64_t)dev->digP7) << 4);
	return 0;
}

#ifndef PIOS_EXCLUDE_BMP280_I2C
static int32_t PIOS_BMP280_Read_I2C(uint8_t address, uint8_t *buffer, uint8_t len)
{
	const struct pios_i2c_txn txn_list[] = {
		{
			.info = __func__,
			.addr = BMP280_I2C_ADDR,
			.rw = PIOS_I2C_TXN_WRITE,
			.len = 1,
			.buf = &address,
		}
		,
		{
		 .info = __func__,
		 .addr = BMP280_I2C_ADDR,
		 .rw = PIOS_I2C_TXN_READ,
		 .len = len,
		 .buf = buffer,
		 }
	};

	return PIOS_I2C_Transfer(dev->i2c_id, txn_list, NELEMENTS(txn_list));
}

static int32_t PIOS_BMP280_Write_I2C(uint8_t *buffer, uint8_t len) {
	const struct pios_i2c_txn txn_list[] = {
		{
		 .info = __func__,
		 .addr = BMP280_I2C_ADDR,
		 .rw = PIOS_I2C_TXN_WRITE,
		 .len = len,
		 .buf = buffer,
		 }
		,
	};

	return PIOS_I2C_Transfer(dev->i2c_id, txn_list, NELEMENTS(txn_list));
}
#endif /* PIOS_EXCLUDE_BMP280_I2C */

#ifdef PIOS_INCLUDE_BMP280_SPI
static int32_t PIOS_BMP280_ClaimBus(struct bmp280_dev *bmp_dev)
{
	if (PIOS_SPI_ClaimBus(bmp_dev->spi_id) != 0)
		return -2;

	PIOS_SPI_RC_PinSet(bmp_dev->spi_id, bmp_dev->spi_slave, false);

	PIOS_SPI_SetClockSpeed(bmp_dev->spi_id, PIOS_BMP_SPI_SPEED);

	return 0;
}

static int32_t PIOS_BMP280_ReleaseBus(struct bmp280_dev *bmp_dev)
{
	PIOS_SPI_RC_PinSet(bmp_dev->spi_id, bmp_dev->spi_slave, true);

	PIOS_SPI_ReleaseBus(bmp_dev->spi_id);

	return 0;
}

static int32_t PIOS_BMP280_Read_SPI(uint8_t address, uint8_t *buffer, uint8_t len) {
	if (PIOS_BMP280_ClaimBus(dev) != 0)
		return -1;

	PIOS_SPI_TransferByte(dev->spi_id, 0x80 | address);

	int ret = PIOS_SPI_TransferBlock(dev->spi_id, NULL, buffer, len);

	PIOS_BMP280_ReleaseBus(dev);

	return ret;
}

static int32_t PIOS_BMP280_Write_SPI(uint8_t *buffer, uint8_t len) {
	if (PIOS_BMP280_ClaimBus(dev) != 0)
		return -1;

	int ret = PIOS_SPI_TransferBlock(dev->spi_id, buffer, NULL, len);

	PIOS_BMP280_ReleaseBus(dev);

	return ret;
}
#endif /* PIOS_INCLUDE_BMP280_SPI */

/*
* Reads one or more bytes into a buffer
* \param[in] the command indicating the address to read
* \param[out] buffer destination buffer
* \param[in] len number of bytes which should be read
* \return 0 if operation was successful
* \return -1 if error during I2C transfer
*/
static int32_t PIOS_BMP280_Read(uint8_t address, uint8_t *buffer, uint8_t len)
{
	PIOS_Assert(PIOS_BMP280_Validate(dev) == 0);

	if (0) {
#ifndef PIOS_EXCLUDE_BMP280_I2C
	} else if (dev->i2c_id) {
		return PIOS_BMP280_Read_I2C(address, buffer, len);
#endif
#ifdef PIOS_INCLUDE_BMP280_SPI
	} else if (dev->spi_id) {
		return PIOS_BMP280_Read_SPI(address, buffer, len);
#endif
	}

	PIOS_Assert(0);
	return -1;
}

/**
* Writes one address and one byte to the BMP280
* \param[in] address Register address
* \param[in] buffer source buffer
* \return 0 if operation was successful
* \return -1 if error during I2C transfer
*/
static int32_t PIOS_BMP280_WriteCommand(uint8_t address, uint8_t buffer)
{
	uint8_t data[] = {
		address,
		buffer,
	};

	PIOS_Assert(PIOS_BMP280_Validate(dev) == 0);

	if (0) {
#ifndef PIOS_EXCLUDE_BMP280_I2C
	} else if (dev->i2c_id) {
		return PIOS_BMP280_Write_I2C(data, sizeof(data));
#endif
#ifdef PIOS_INCLUDE_BMP280_SPI
	} else if (dev->spi_id) {
		data[0] &= 0x7f;	/* Clear high bit */
		return PIOS_BMP280_Write_SPI(data, sizeof(data));
#endif
	}

	PIOS_Assert(0);
	return -1;
}

/**
* @brief Run self-test operation.
* \return 0 if self-test succeed, -1 if failed
*/
int32_t PIOS_BMP280_Test()
{
	if (PIOS_BMP280_Validate(dev) != 0)
		return -1;

	// TODO: Is there a better way to test this than just checking that pressure/temperature has changed?
	int32_t currentTemperature = 0;
	int32_t currentPressure    = 0;

	PIOS_BMP280_ClaimDevice();

	currentTemperature = dev->compensatedTemperature;
	currentPressure    = dev->compensatedPressure;

	PIOS_BMP280_StartADC();
	PIOS_DELAY_WaitmS(PIOS_BMP280_GetDelay());
	PIOS_BMP280_ReadADC(true);
	PIOS_BMP280_ReleaseDevice();

	if (currentTemperature == dev->compensatedTemperature)
		return -1;

	if (currentPressure == dev->compensatedPressure)
		return -1;

	return 0;
}

static void PIOS_BMP280_Task(void *parameters)
{
	int32_t temp_press_interleave_count = dev->temperature_interleaving;
	int32_t read_adc_result = 0;
	bool    computeTemp = false;

	while (1) {

		temp_press_interleave_count --;

		if(temp_press_interleave_count <= 0) {
			computeTemp = true;
			temp_press_interleave_count = dev->temperature_interleaving;
		}
		else
			computeTemp = false;

		PIOS_BMP280_ClaimDevice();
		PIOS_BMP280_StartADC();
		PIOS_Thread_Sleep(PIOS_BMP280_GetDelay());
		read_adc_result = PIOS_BMP280_ReadADC(computeTemp);
		PIOS_BMP280_ReleaseDevice();

		// Compute the altitude from the pressure and temperature and send it out
		struct pios_sensor_baro_data data;
		data.temperature = ((float) dev->compensatedTemperature) / 100.0f;
		data.pressure = ((float) dev->compensatedPressure) / 256.0f / 1000.0f;
		data.altitude = 44330.0f * (1.0f - powf(data.pressure / BMP280_P0, (1.0f / 5.255f)));

		if (read_adc_result == 0) {
			PIOS_Queue_Send(dev->queue, (void*)&data, 0);
		}
	}
}

#endif /* PIOS_INCLUDE_BMP280 */
/**
 * @}
 * @}
 */
