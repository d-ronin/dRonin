/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_BMI160 BMI160 Functions
 * @brief Hardware functions to deal with the 6DOF gyro / accel sensor
 * @{
 *
 * @file       pios_bmi160.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @brief      BMI160 Gyro / Accel Sensor Routines
 * @see        The GNU Public License (GPL) Version 3
 ******************************************************************************/

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

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_BMI160)
#include "physical_constants.h"
#include "pios_bmi160.h"
#include "pios_semaphore.h"
#include "pios_thread.h"

/* BMI160 Registers */
#define BMI160_REG_CHIPID 0x00
#define BMI160_REG_PMU_STAT 0x03
#define BMI160_REG_GYR_DATA_X_LSB 0x0C
#define BMI160_REG_STATUS 0x1B
#define BMI160_REG_TEMPERATURE_0 0x20
#define BMI160_REG_ACC_CONF 0x40
#define BMI160_REG_ACC_RANGE 0x41
#define BMI160_REG_GYR_CONF 0x42
#define BMI160_REG_GYR_RANGE 0x43
#define BMI160_REG_INT_EN1 0x51
#define BMI160_REG_INT_OUT_CTRL 0x53
#define BMI160_REG_INT_MAP1 0x56
#define BMI160_REG_FOC_CONF 0x69
#define BMI160_REG_CONF 0x6A
#define BMI160_REG_OFFSET_0 0x77
#define BMI160_REG_CMD 0x7E

/* Register values */
#define BMI160_PMU_CMD_PMU_ACC_NORMAL 0x11
#define BMI160_PMU_CMD_PMU_GYR_NORMAL 0x15
#define BMI160_INT_EN1_DRDY 0x10
#define BMI160_INT_OUT_CTRL_INT1_CONFIG 0x0A
#define BMI160_REG_INT_MAP1_INT1_DRDY 0x80
#define BMI160_CMD_START_FOC 0x03
#define BMI160_CMD_PROG_NVM 0xA0
#define BMI160_REG_STATUS_NVM_RDY 0x10
#define BMI160_REG_STATUS_FOC_RDY 0x08
#define BMI160_REG_CONF_NVM_PROG_EN 0x02

/* Global Variables */
enum pios_bmi160_dev_magic {
	PIOS_BMI160_DEV_MAGIC = 0x76dfa5ba,
};

struct bmi160_dev {
	pios_spi_t spi_id;
	uint32_t slave_num;
	const struct pios_bmi160_cfg *cfg;
	struct pios_semaphore *data_ready_sema;
	float accel_scale;
	float gyro_scale;
	enum pios_bmi160_dev_magic magic;

	/* Doublebuffer data because it gets read in one go, but polled in two parts. */
	struct pios_sensor_accel_data accel_data;
	struct pios_sensor_gyro_data gyro_data;

	uint8_t temp_interleave_cnt;

	/* Store which sensor part has been read. */
	uint8_t sensor_read;
	uint8_t sensor_updated;

	/* Sensor handles. */
	pios_sensor_t gyro_reg;
	pios_sensor_t accel_reg;
};

#define FLAG_READ_GYRO			0x01
#define FLAG_READ_ACCEL			0x02

//! Global structure for this device device
static struct bmi160_dev *dev;

//! Private functions
static int32_t PIOS_BMI160_Config(const struct pios_bmi160_cfg *cfg);
static int32_t PIOS_BMI160_do_foc(const struct pios_bmi160_cfg *cfg);
static struct bmi160_dev *PIOS_BMI160_alloc(const struct pios_bmi160_cfg *cfg);
static int32_t PIOS_BMI160_Validate(struct bmi160_dev *dev);
static uint8_t PIOS_BMI160_ReadReg(uint8_t reg);
static int32_t PIOS_BMI160_WriteReg(uint8_t reg, uint8_t data);
static int32_t PIOS_BMI160_ClaimBus();
static int32_t PIOS_BMI160_ReleaseBus();

int PIOS_BMI160_gyro_callback(void *device, void *output);
int PIOS_BMI160_accel_callback(void *device, void *output);

/**
 * @brief Returns an integer value to the ODR setting(s) of the sensor.
 */
inline int PIOS_BMI160_get_samplerate(enum pios_bmi160_odr odr)
{
	switch(odr) {
	case PIOS_BMI160_ODR_800_Hz:
		return 800;
	case PIOS_BMI160_ODR_1600_Hz:
		return 1600;
	default:
		PIOS_Assert(0);
	}
}

/**
 * @brief Initialize the BMI160 6-axis sensor.
 * @return 0 for success, -1 for failure to allocate, -10 for failure to get irq
 */
int32_t PIOS_BMI160_Init(pios_spi_t spi_id, uint32_t slave_num, const struct pios_bmi160_cfg *cfg, bool do_foc)
{
	dev = PIOS_BMI160_alloc(cfg);
	if (dev == NULL)
		return -1;

	dev->spi_id = spi_id;
	dev->slave_num = slave_num;
	dev->cfg = cfg;

	/* Read this address to acticate SPI (see p. 84) */
	PIOS_BMI160_ReadReg(0x7F);
	PIOS_DELAY_WaitmS(10); // Give SPI some time to start up

	/* Check the chip ID */
	if (PIOS_BMI160_ReadReg(BMI160_REG_CHIPID) != 0xd1){
		return -2;
	}

	/* Configure the MPU9250 Sensor */
	if (PIOS_BMI160_Config(cfg) != 0){
		return -3;
	}

	/* Perform fast offset compensation if requested */
	if (do_foc) {
		if (PIOS_BMI160_do_foc(cfg) != 0) {
			return -4;
		}
	}

	/* Set up EXTI line */
	PIOS_EXTI_Init(cfg->exti_cfg);

	// Wait 20 ms for data ready interrupt and make sure it happens
	// twice
	if ((PIOS_Semaphore_Take(dev->data_ready_sema, 20) != true) ||
		(PIOS_Semaphore_Take(dev->data_ready_sema, 20) != true)) {
		return -10;
	}
	dev->gyro_reg = PIOS_Sensors_Register(PIOS_SENSOR_GYRO, (void*)dev, PIOS_BMI160_gyro_callback, PIOS_SENSORS_FLAG_SEMAPHORE);
	dev->accel_reg = PIOS_Sensors_Register(PIOS_SENSOR_ACCEL, (void*)dev, PIOS_BMI160_accel_callback, PIOS_SENSORS_FLAG_NONE);

	PIOS_Sensors_SetUpdateRate(dev->gyro_reg, PIOS_BMI160_get_samplerate(cfg->odr));
	PIOS_Sensors_SetUpdateRate(dev->accel_reg, PIOS_BMI160_get_samplerate(cfg->odr));

	/* Configure the scales */
	switch (cfg->acc_range){
		case PIOS_BMI160_RANGE_2G:
			dev->accel_scale = GRAVITY / 16384.f;
			break;
		case PIOS_BMI160_RANGE_4G:
			dev->accel_scale = GRAVITY / 8192.f;
			break;
		case PIOS_BMI160_RANGE_8G:
			dev->accel_scale = GRAVITY / 4096.f;
			break;
		case PIOS_BMI160_RANGE_16G:
			dev->accel_scale = GRAVITY / 2048.f;
			break;
	}

	switch (cfg->gyro_range){
		case PIOS_BMI160_RANGE_125DPS:
			dev->gyro_scale = 1.f / 262.4f;
			PIOS_Sensors_SetRange(dev->gyro_reg, 125);
			break;
		case PIOS_BMI160_RANGE_250DPS:
			dev->gyro_scale = 1.f / 131.2f;
			PIOS_Sensors_SetRange(dev->gyro_reg, 250);
			break;
		case PIOS_BMI160_RANGE_500DPS:
			dev->gyro_scale = 1.f / 65.6f;
			PIOS_Sensors_SetRange(dev->gyro_reg, 500);
			break;
		case PIOS_BMI160_RANGE_1000DPS:
			dev->gyro_scale = 1.f / 32.8f;
			PIOS_Sensors_SetRange(dev->gyro_reg, 1000);
			break;
		case PIOS_BMI160_RANGE_2000DPS:
			dev->gyro_scale = 1.f / 16.4f;
			PIOS_Sensors_SetRange(dev->gyro_reg, 2000);
			break;
	}

	return 0;
}


/**
 * @brief Configure the sensor
 */
static int32_t PIOS_BMI160_Config(const struct pios_bmi160_cfg *cfg)
{

	// Set normal power mode for gyro and accelerometer
	if (PIOS_BMI160_WriteReg(BMI160_REG_CMD, BMI160_PMU_CMD_PMU_GYR_NORMAL) != 0){
		return -1;
	}
	PIOS_DELAY_WaitmS(100); // can take up to 80ms

	if (PIOS_BMI160_WriteReg(BMI160_REG_CMD, BMI160_PMU_CMD_PMU_ACC_NORMAL) != 0){
		return -2;
	}
	PIOS_DELAY_WaitmS(5); // can take up to 3.8ms

	// Verify that normal power mode was entered
	uint8_t pmu_status = PIOS_BMI160_ReadReg(BMI160_REG_PMU_STAT);
	if ((pmu_status & 0x3C) != 0x14){
		return -3;
	}

	// Set odr and ranges
	// Set acc_us = 0 acc_bwp = 0b010 so only the first filter stage is used
	if (PIOS_BMI160_WriteReg(BMI160_REG_ACC_CONF, 0x20 | cfg->odr) != 0){
		return -3;
	}
	PIOS_DELAY_WaitmS(1);

	// Set gyr_bwp = 0b010 so only the first filter stage is used
	if (PIOS_BMI160_WriteReg(BMI160_REG_GYR_CONF, 0x20 | cfg->odr) != 0){
		return -4;
	}
	PIOS_DELAY_WaitmS(1);

	if (PIOS_BMI160_WriteReg(BMI160_REG_ACC_RANGE, cfg->acc_range) != 0){
		return -5;
	}
	PIOS_DELAY_WaitmS(1);

	if (PIOS_BMI160_WriteReg(BMI160_REG_GYR_RANGE, cfg->gyro_range) != 0){
		return -6;
	}
	PIOS_DELAY_WaitmS(1);

	// Enable offset compensation
	uint8_t val = PIOS_BMI160_ReadReg(BMI160_REG_OFFSET_0);
	if (PIOS_BMI160_WriteReg(BMI160_REG_OFFSET_0, val | 0xC0) != 0){
		return -7;
	}

	// Enable data ready interrupt
	if (PIOS_BMI160_WriteReg(BMI160_REG_INT_EN1, BMI160_INT_EN1_DRDY) != 0){
		return -8;
	}
	PIOS_DELAY_WaitmS(1);

	// Enable INT1 pin
	if (PIOS_BMI160_WriteReg(BMI160_REG_INT_OUT_CTRL, BMI160_INT_OUT_CTRL_INT1_CONFIG) != 0){
		return -9;
	}
	PIOS_DELAY_WaitmS(1);

	// Map data ready interrupt to INT1 pin
	if (PIOS_BMI160_WriteReg(BMI160_REG_INT_MAP1, BMI160_REG_INT_MAP1_INT1_DRDY) != 0){
		return -10;
	}
	PIOS_DELAY_WaitmS(1);

	return 0;
}


/**
 * @brief Perform fast offset callibration and store values in NVM
 *
 * The flight controller has to sit level. Can only be performed a limited number
 * of times (14 max) due to limited NVM write cycles
 */
static int32_t PIOS_BMI160_do_foc(const struct pios_bmi160_cfg *cfg)
{
	uint8_t val = 0;

	// Set the orientation: 0G for x and y, +/- 1G for z
	switch (cfg->orientation) {
		case PIOS_BMI160_TOP_0DEG:
		case PIOS_BMI160_TOP_90DEG:
		case PIOS_BMI160_TOP_180DEG:
		case PIOS_BMI160_TOP_270DEG:
			val = 0x7D;
			break;
		case PIOS_BMI160_BOTTOM_0DEG:
		case PIOS_BMI160_BOTTOM_90DEG:
		case PIOS_BMI160_BOTTOM_180DEG:
		case PIOS_BMI160_BOTTOM_270DEG:
			val = 0x7E;
	}

	if (PIOS_BMI160_WriteReg(BMI160_REG_FOC_CONF, val) != 0) {
		return -1;
	}

	// Start FOC
	if (PIOS_BMI160_WriteReg(BMI160_REG_CMD, BMI160_CMD_START_FOC) != 0) {
		return -2;
	}

	// Wait for FOC to complete
	for (int i=0; i<50; i++) {
		val = PIOS_BMI160_ReadReg(BMI160_REG_STATUS);
		if (val & BMI160_REG_STATUS_FOC_RDY) {
			break;
		}
		PIOS_DELAY_WaitmS(10);
		PIOS_WDG_Clear();
	}
	if (!(val & BMI160_REG_STATUS_FOC_RDY)) {
		return -3;
	}

	// Program NVM
	val = PIOS_BMI160_ReadReg(BMI160_REG_CONF);
	if (PIOS_BMI160_WriteReg(BMI160_REG_CONF, val | BMI160_REG_CONF_NVM_PROG_EN) != 0) {
		return -4;
	}

	if (PIOS_BMI160_WriteReg(BMI160_REG_CMD, BMI160_CMD_PROG_NVM) != 0) {
		return -5;
	}

	// Wait for NVM programming to complete
	for (int i=0; i<50; i++) {
		val = PIOS_BMI160_ReadReg(BMI160_REG_STATUS);
		if (val & BMI160_REG_STATUS_NVM_RDY) {
			break;
		}
		PIOS_DELAY_WaitmS(10);
		PIOS_WDG_Clear();
	}
	if (!(val & BMI160_REG_STATUS_NVM_RDY)) {
		return -6;
	}

	return 0;
}

/**
 * @brief Allocate a new device
 */
static struct bmi160_dev *PIOS_BMI160_alloc(const struct pios_bmi160_cfg *cfg)
{
	struct bmi160_dev *bmi160_dev;

	bmi160_dev = (struct bmi160_dev *)PIOS_malloc(sizeof(*bmi160_dev));
	if (!bmi160_dev)
		return NULL;
	memset(bmi160_dev, 0, sizeof(*bmi160_dev));

	bmi160_dev->magic = PIOS_BMI160_DEV_MAGIC;

	bmi160_dev->data_ready_sema = PIOS_Semaphore_Create();
	if (bmi160_dev->data_ready_sema == NULL) {
		PIOS_free(bmi160_dev);
		return NULL;
	}

	return bmi160_dev;
}

/**
 * @brief Validate the handle to the device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_BMI160_Validate(struct bmi160_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_BMI160_DEV_MAGIC)
		return -2;
	if (dev->spi_id == 0)
		return -3;
	return 0;
}

/**
 * @brief Claim the SPI bus for the communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_BMI160_ClaimBus()
{
	if (PIOS_BMI160_Validate(dev) != 0)
		return -1;

	if (PIOS_SPI_ClaimBus(dev->spi_id) != 0)
		return -2;

	PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 0);

	return 0;
}

/**
 * @brief Release the SPI bus for the communications and end the transaction
 * \param[in] must be true when bus was claimed in lowspeed mode
 * @return 0 if successful
 */
static int32_t PIOS_BMI160_ReleaseBus()
{
	if (PIOS_BMI160_Validate(dev) != 0)
		return -1;

	PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 1);

	PIOS_SPI_ReleaseBus(dev->spi_id);

	return 0;
}

/**
 * @brief Read a register from BMI160
 * @returns The register value
 * @param reg[in] Register address to be read
 */
static uint8_t PIOS_BMI160_ReadReg(uint8_t reg)
{
	uint8_t data;

	PIOS_BMI160_ClaimBus();

	PIOS_SPI_TransferByte(dev->spi_id, 0x80 | reg); // request byte
	data = PIOS_SPI_TransferByte(dev->spi_id, 0);   // receive response

	PIOS_BMI160_ReleaseBus();

	return data;
}

/**
 * @brief Writes one byte to the BMI160 register
 * \param[in] reg Register address
 * \param[in] data Byte to write
 * @returns 0 when success
 */
static int32_t PIOS_BMI160_WriteReg(uint8_t reg, uint8_t data)
{
	if (PIOS_BMI160_ClaimBus() != 0)
		return -1;

	PIOS_SPI_TransferByte(dev->spi_id, 0x7f & reg);
	PIOS_SPI_TransferByte(dev->spi_id, data);

	PIOS_BMI160_ReleaseBus();

	return 0;
}

/**
* @brief IRQ Handler
*/
bool PIOS_BMI160_IRQHandler(void)
{
	if (PIOS_BMI160_Validate(dev) != 0)
		return false;

	bool need_yield = false;

	if (dev->data_ready_sema) {
		PIOS_Semaphore_Give_FromISR(dev->data_ready_sema, &need_yield);
	}
	
	if (dev->gyro_reg) {
		dev->sensor_updated = 1;
		dev->sensor_read = 0;

		PIOS_Sensors_RaiseSignal_FromISR(dev->gyro_reg);
		PIOS_Sensors_RaiseSignal_FromISR(dev->accel_reg);
	}

	return need_yield;
}

/**
 * @brief Tries to read out the sensor.
 *
 * @returns Zero if successful.
 */
static int PIOS_BMI160_parse_data(void *p)
{
	struct bmi160_dev *dev = (struct bmi160_dev *)p;

	/* Clear update flag. If this callback function fails, sample will be lost.
	   Sensors module shouldn't make an effort to get it at all cost. */
	dev->sensor_updated = 0;

	enum {
		IDX_REG = 0,
		IDX_GYRO_XOUT_L,
		IDX_GYRO_XOUT_H,
		IDX_GYRO_YOUT_L,
		IDX_GYRO_YOUT_H,
		IDX_GYRO_ZOUT_L,
		IDX_GYRO_ZOUT_H,
		IDX_ACCEL_XOUT_L,
		IDX_ACCEL_XOUT_H,
		IDX_ACCEL_YOUT_L,
		IDX_ACCEL_YOUT_H,
		IDX_ACCEL_ZOUT_L,
		IDX_ACCEL_ZOUT_H,
		BUFFER_SIZE,
	};

	uint8_t bmi160_rec_buf[BUFFER_SIZE];
	uint8_t bmi160_tx_buf[BUFFER_SIZE] = {BMI160_REG_GYR_DATA_X_LSB | 0x80, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0};

	if (PIOS_BMI160_ClaimBus() != 0)
		return -1;

	if (PIOS_SPI_TransferBlock(dev->spi_id, bmi160_tx_buf, bmi160_rec_buf, BUFFER_SIZE) < 0) {
		PIOS_BMI160_ReleaseBus();
		return -1;
	}

	PIOS_BMI160_ReleaseBus();

	float accel_x = (int16_t)(bmi160_rec_buf[IDX_ACCEL_XOUT_H] << 8 | bmi160_rec_buf[IDX_ACCEL_XOUT_L]);
	float accel_y = (int16_t)(bmi160_rec_buf[IDX_ACCEL_YOUT_H] << 8 | bmi160_rec_buf[IDX_ACCEL_YOUT_L]);
	float accel_z = (int16_t)(bmi160_rec_buf[IDX_ACCEL_ZOUT_H] << 8 | bmi160_rec_buf[IDX_ACCEL_ZOUT_L]);
	float gyro_x = (int16_t)(bmi160_rec_buf[IDX_GYRO_XOUT_H] << 8 | bmi160_rec_buf[IDX_GYRO_XOUT_L]);
	float gyro_y = (int16_t)(bmi160_rec_buf[IDX_GYRO_YOUT_H] << 8 | bmi160_rec_buf[IDX_GYRO_YOUT_L]);
	float gyro_z = (int16_t)(bmi160_rec_buf[IDX_GYRO_ZOUT_H] << 8 | bmi160_rec_buf[IDX_GYRO_ZOUT_L]);

	// Apply sensor scaling
	accel_x *= dev->accel_scale;
	accel_y *= dev->accel_scale;
	accel_z *= dev->accel_scale;

	gyro_x *= dev->gyro_scale;
	gyro_y *= dev->gyro_scale;
	gyro_z *= dev->gyro_scale;

	/* 
	 * Convert from sensor frame (x: forward y: left z: up) to
	 * TL convention (x: forward y: right z: down).
	 * See flight/Doc/imu_orientation.md for more detail
	 */
	switch (dev->cfg->orientation) {
	case PIOS_BMI160_TOP_0DEG:
		dev->accel_data.x = accel_x;
		dev->accel_data.y = -accel_y;
		dev->accel_data.z = -accel_z;
		dev->gyro_data.x  = gyro_x;
		dev->gyro_data.y  = -gyro_y;
		dev->gyro_data.z  = -gyro_z;
		break;
	case PIOS_BMI160_TOP_90DEG:
		dev->accel_data.x = accel_y;
		dev->accel_data.y = accel_x;
		dev->accel_data.z = -accel_z;
		dev->gyro_data.x  = gyro_y;
		dev->gyro_data.y  = gyro_x;
		dev->gyro_data.z  = -gyro_z;
		break;
	case PIOS_BMI160_TOP_180DEG:
		dev->accel_data.x = -accel_x;
		dev->accel_data.y = accel_y;
		dev->accel_data.z = -accel_z;
		dev->gyro_data.x  = -gyro_x;
		dev->gyro_data.y  = gyro_y;
		dev->gyro_data.z  = -gyro_z;
		break;
	case PIOS_BMI160_TOP_270DEG:
		dev->accel_data.x = -accel_y;
		dev->accel_data.y = -accel_x;
		dev->accel_data.z = -accel_z;
		dev->gyro_data.x  = -gyro_y;
		dev->gyro_data.y  = -gyro_x;
		dev->gyro_data.z  = -gyro_z;
		break;
	case PIOS_BMI160_BOTTOM_0DEG:
		dev->accel_data.x = accel_x;
		dev->accel_data.y = accel_y;
		dev->accel_data.z = accel_z;
		dev->gyro_data.x  = gyro_x;
		dev->gyro_data.y  = gyro_y;
		dev->gyro_data.z  = gyro_z;
		break;
	case PIOS_BMI160_BOTTOM_90DEG:
		dev->accel_data.x = -accel_y;
		dev->accel_data.y = accel_x;
		dev->accel_data.z = accel_z;
		dev->gyro_data.x  = -gyro_y;
		dev->gyro_data.y  = gyro_x;
		dev->gyro_data.z  = gyro_z;
		break;
	case PIOS_BMI160_BOTTOM_180DEG:
		dev->accel_data.x = -accel_x;
		dev->accel_data.y = -accel_y;
		dev->accel_data.z = accel_z;
		dev->gyro_data.x  = -gyro_x;
		dev->gyro_data.y  = -gyro_y;
		dev->gyro_data.z  = gyro_z;
		break;
	case PIOS_BMI160_BOTTOM_270DEG:
		dev->accel_data.x = accel_y;
		dev->accel_data.y = -accel_x;
		dev->accel_data.z = accel_z;
		dev->gyro_data.x  = gyro_y;
		dev->gyro_data.y  = -gyro_x;
		dev->gyro_data.z  = gyro_z;
		break;
	}

	/*  Get the temperature
	   NOTE: We do this down here so the chip-select has some time to go low. Strange things happen
	   When this is done right after readin the accels / gyros */
	if (dev->temp_interleave_cnt % dev->cfg->temperature_interleaving == 0){
		if (PIOS_BMI160_ClaimBus() != 0) {
			/* If we got here, we got gyro data, who cares if temp readout is flaky. */
			return 0;
		}

		uint8_t bmi160_tx_buf[BUFFER_SIZE] = {BMI160_REG_TEMPERATURE_0 | 0x80, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0};
		if (PIOS_SPI_TransferBlock(dev->spi_id, bmi160_tx_buf, bmi160_rec_buf, 3) < 0) {
			PIOS_BMI160_ReleaseBus();
			/* If we got here, we got gyro data, who cares if temp readout is flaky. */
			goto out;
		}

		PIOS_BMI160_ReleaseBus();
		float temperature =  23.f + (int16_t)(bmi160_rec_buf[2] << 8 | bmi160_rec_buf[1]) / 512.f;
		
		dev->accel_data.temperature = temperature;
		dev->gyro_data.temperature = temperature;
	}

out:
	dev->temp_interleave_cnt++;

	return 0;
}

/**
 * @brief Common callback function.
 */
int PIOS_BMI160_callback_common(void *device, void *output, bool gyro)
{
	PIOS_Assert(device);
	PIOS_Assert(output);

	struct bmi160_dev *dev = (struct bmi160_dev *)device;

	if (dev->sensor_updated) {
		if (PIOS_BMI160_parse_data(device)) {
			/* If the function reports an error, block readouts until next update. */
			dev->sensor_read = FLAG_READ_ACCEL | FLAG_READ_GYRO;
			return PIOS_SENSORS_DATA_NONE;
		}
	}

	if (gyro) {
		if (!(dev->sensor_read & FLAG_READ_GYRO)) {
			dev->sensor_read |= FLAG_READ_GYRO;
			memcpy(output, &dev->gyro_data, sizeof(dev->gyro_data));
			return PIOS_SENSORS_DATA_AVAILABLE;
		}
	} else {
		if (!(dev->sensor_read & FLAG_READ_ACCEL)) {
			dev->sensor_read |= FLAG_READ_ACCEL;
			memcpy(output, &dev->accel_data, sizeof(dev->accel_data));
			return PIOS_SENSORS_DATA_AVAILABLE;
		}
	}

	return PIOS_SENSORS_DATA_NONE;
}


/**
 * @brief Callback function to read out gyro data.
 */
int PIOS_BMI160_gyro_callback(void *device, void *output)
{
	return PIOS_BMI160_callback_common(device, output, true);
}

/**
 * @brief Callback function to read out accel data.
 */
int PIOS_BMI160_accel_callback(void *device, void *output)
{
	return PIOS_BMI160_callback_common(device, output, false);
}


#endif /* PIOS_INCLUDE_BMI160 */
