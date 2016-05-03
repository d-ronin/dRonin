/**
 ******************************************************************************
 * @addtogroup dRonin Targets
 * @{
 * @addtogroup BrainRE1 support files
 * @{
 *
 * @file       fpga_drv.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @brief      Driver for the RE1 custom FPGA
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include <pios.h>

#if defined(PIOS_INCLUDE_RE1_FPGA)

#include "fpga_drv.h"
#include "stm32f4xx_rcc.h"
#include "pios_spi_priv.h"

/**
* RE1 Register Specification
* ##########################
*
* Address  Name   Type  Length  Default  Description
* 0x00     HWREV  R     1       NA       Hardware/FPGA revision
* 0x01     CFG    R/W   1       0x00     CFG[0]: Serial RX inverter
*                                                0: normal 1: invert
*                                        CFG[1]: MultiPort TX mode
*                                                0: output 1: bidirectional
*                                        CFG[2]: MultiPort TX inverter
*                                                0: normal 1: inverted
*                                        CFG[3]: MultiPort TX pullup/down
*                                                0: pullup / down disabled
*                                                1: pullup / down enabled
*                                        CFG[4]: MultiPort TX pullup/down
*                                                0: pull-down
*                                                1: pull-up
*                                        CFG[5]: LEDCFG 0: (status: blue, custom: green)
*                                                       0: (status: green, custom: blue)
* 0x02     CTL    R/W   1       0x00     CTL[0]: BUZZER 0: off, 1: on
*                                        CTL[1]: LED custom: 0: off, 1: on
* 0x03     BLACK  R/W   1       XXX      OSD black level
* 0x04     WHITE  R/W   1       XXX      OSD white level
* 0x05     THR    R/W   1       XXX      OSD sync detect threshold
* 0x06     XCFG   R/W   1       0x00     OSD X axis configuration
*                                        XCFG[7:4] x-axis stretch
*                                        XCFG[3:0] x-axis offset
* 0x07     XCFG2  R/W   1       0x00     XCFG2[6]  : 0: normal 1: SBS3D mode
*                                        XCFG2[5:0]: right-eye x offset
* 0x08     IRCFG  R/W   1       0x00     IRCFG[0:3] IR Protocol
*                                        0x0: OFF / STM32 controlled
*                                        0x1: I-Lap / Trackmate
*                                        0x2: XX
*                                        0x3: XX
*                                        IRCFG[7:4] IR Power
* 0x09     IRDATA W     16      0x00     IR tranponder data
* 0x0F     LED    W     3072    0x00     WS2812B LED data
*/

enum pios_re1fpga_register {
	RE1FPGA_REG_HWREV   = 0x00,
	RE1FPGA_REG_CFG     = 0x01,
	RE1FPGA_REG_CTL     = 0x02,
	RE1FPGA_REG_BLACK   = 0x03,
	RE1FPGA_REG_WHITE   = 0x04,
	RE1FPGA_REG_THR     = 0x05,
	RE1FPGA_REG_XCFG    = 0x06,
	RE1FPGA_REG_XCFG2   = 0x07,
	RE1FPGA_REG_IRCFG   = 0x08,
	RE1FPGA_REG_IRDATA  = 0x09,
	RE1FPGA_REG_LED     = 0x0F,
};

struct re1_shadow_reg {
	uint8_t reg_hwrev;
	uint8_t reg_cfg;
	uint8_t reg_ctl;
	uint8_t reg_black;
	uint8_t reg_white;
	uint8_t reg_thr;
	uint8_t reg_xcfg;
	uint8_t reg_xcfg2;
	uint8_t reg_ircfg;
};


enum pios_re1fpga_dev_magic {
	PIOS_RE1FPGA_DEV_MAGIC = 0xbadfed42,
};

struct re1fpga_dev {
	uint32_t spi_id;
	uint32_t slave_num;
	const struct pios_re1fpga_cfg *cfg;
	struct re1_shadow_reg shadow_reg;
	enum pios_re1fpga_dev_magic magic;
};

#define MIN(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

//! Global structure for this device device
static struct re1fpga_dev *dev;

static int32_t PIOS_RE1FPGA_WriteReg(uint8_t reg, uint8_t data, uint8_t mask);
static int32_t PIOS_RE1FPGA_WriteRegDirect(enum pios_re1fpga_register reg, uint8_t data);
static uint8_t PIOS_RE1FPGA_ReadReg(uint8_t reg);
static void update_shadow_regs();
int32_t PIOS_RE1FPGA_SetLEDs(const uint8_t * led_data, uint16_t n_leds);
int32_t PIOS_RE1FPGA_SetIRData(const uint8_t * ir_data, uint8_t n_bytes);


/**
 * @brief Allocate a new device
 */
static struct re1fpga_dev *PIOS_RE1FPGA_alloc(const struct pios_re1fpga_cfg *cfg)
{
	struct re1fpga_dev *re1fpga_dev;

	re1fpga_dev = (struct re1fpga_dev *)PIOS_malloc(sizeof(*re1fpga_dev));
	if (!re1fpga_dev)
		return NULL;

	re1fpga_dev->magic = PIOS_RE1FPGA_DEV_MAGIC;

	return re1fpga_dev;
}

volatile uint8_t test;
/**
 * @brief Initialize the driver
 * @return 0 for success
 */
int32_t PIOS_RE1FPGA_Init(uint32_t spi_id, uint32_t slave_num, const struct pios_re1fpga_cfg *cfg, bool load_config)
{
	dev = PIOS_RE1FPGA_alloc(cfg);
	if (dev == NULL)
		return -1;

	dev->spi_id = spi_id;
	dev->slave_num = slave_num;
	dev->cfg = cfg;

	if (load_config) {
		/* Configure the CDONE and CRESETB pins */
		GPIO_Init(dev->cfg->cdone_pin.gpio, (GPIO_InitTypeDef *) & (dev->cfg->cdone_pin.init));
		GPIO_Init(dev->cfg->cresetb_pin.gpio, (GPIO_InitTypeDef *) & (dev->cfg->cresetb_pin.init));

		// CRESETB low for 1ms
		GPIO_ResetBits(dev->cfg->cresetb_pin.gpio, dev->cfg->cresetb_pin.init.GPIO_Pin);
		PIOS_DELAY_WaitmS(1);
		GPIO_SetBits(dev->cfg->cresetb_pin.gpio, dev->cfg->cresetb_pin.init.GPIO_Pin);

		for (int i=0; i<100; i++) {
			if (GPIO_ReadInputDataBit(dev->cfg->cdone_pin.gpio, dev->cfg->cdone_pin.init.GPIO_Pin))
				break;
			PIOS_DELAY_WaitmS(1);
		}

		if (!GPIO_ReadInputDataBit(dev->cfg->cdone_pin.gpio, dev->cfg->cdone_pin.init.GPIO_Pin))
			return -2;
	}

	/* Configure 16MHz clock output to FPGA */
	GPIO_Init(dev->cfg->mco_pin.gpio, (GPIO_InitTypeDef *) & (dev->cfg->mco_pin.init));
	GPIO_PinAFConfig(dev->cfg->mco_pin.gpio,
					 __builtin_ctz(dev->cfg->mco_pin.init.GPIO_Pin),
					 GPIO_AF_MCO);
	RCC_MCO1Config(RCC_MCO1Source_HSE, RCC_MCO1Div_1);
	RCC_MCO1Cmd(ENABLE);

	/* Configure reset pin */
	GPIO_Init(dev->cfg->rst_pin.gpio, (GPIO_InitTypeDef *) & (dev->cfg->rst_pin.init));
	GPIO_ResetBits(dev->cfg->rst_pin.gpio, dev->cfg->rst_pin.init.GPIO_Pin);

	// Give the PLL some time to stabilize
	PIOS_DELAY_WaitmS(10);

	// Reset the FPGA (pull rst low and then high again)
	GPIO_SetBits(dev->cfg->rst_pin.gpio, dev->cfg->rst_pin.init.GPIO_Pin);
	PIOS_DELAY_WaitmS(10);
	GPIO_ResetBits(dev->cfg->rst_pin.gpio, dev->cfg->rst_pin.init.GPIO_Pin);
	PIOS_DELAY_WaitmS(1);

	// Initialize registers
	PIOS_RE1FPGA_WriteRegDirect(RE1FPGA_REG_CFG, 0x00);
	PIOS_RE1FPGA_WriteRegDirect(RE1FPGA_REG_CTL, 0x00);
	PIOS_RE1FPGA_WriteRegDirect(RE1FPGA_REG_BLACK, 35);
	PIOS_RE1FPGA_WriteRegDirect(RE1FPGA_REG_BLACK, 110);
	PIOS_RE1FPGA_WriteRegDirect(RE1FPGA_REG_THR, 122);
	PIOS_RE1FPGA_WriteRegDirect(RE1FPGA_REG_XCFG, 0x08);
	PIOS_RE1FPGA_WriteRegDirect(RE1FPGA_REG_XCFG2, 0x10);
	PIOS_RE1FPGA_WriteRegDirect(RE1FPGA_REG_IRCFG, 0x00);

	// Read all registers into shadow regsiters
	update_shadow_regs();

	return 0;
}

/**
 * @brief Validate the handle to the device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_RE1FPGA_Validate(struct re1fpga_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_RE1FPGA_DEV_MAGIC)
		return -2;
	if (dev->spi_id == 0)
		return -3;
	return 0;
}

/**
 * @brief Claim the SPI bus for the communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_RE1FPGA_ClaimBus()
{
	if (PIOS_RE1FPGA_Validate(dev) != 0)
		return -1;

	if (PIOS_SPI_ClaimBus(dev->spi_id) != 0)
		return -2;

	PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 0);

	return 0;
}

/**
 * @brief Release the SPI bus for the communications and end the transaction
 * @return 0 if successful
 */
static int32_t PIOS_RE1FPGA_ReleaseBus()
{
	if (PIOS_RE1FPGA_Validate(dev) != 0)
		return -1;

	PIOS_SPI_RC_PinSet(dev->spi_id, dev->slave_num, 1);

	PIOS_SPI_ReleaseBus(dev->spi_id);

	return 0;
}

/**
 * @brief Writes one byte to the RE1FPGA register
 * \param[in] reg Register address
 * \param[in] data Byte to write
 * @returns 0 when success
 */
static int32_t PIOS_RE1FPGA_WriteReg(enum pios_re1fpga_register reg, uint8_t data, uint8_t mask)
{
	uint8_t* cur_reg;

	if (PIOS_RE1FPGA_Validate(dev) != 0)
		return -1;

	switch (reg) {
		case RE1FPGA_REG_HWREV:
			return -2;
		case RE1FPGA_REG_CFG:
			cur_reg = &dev->shadow_reg.reg_cfg;
			break;
		case RE1FPGA_REG_CTL:
			cur_reg = &dev->shadow_reg.reg_ctl;
			break;
		case RE1FPGA_REG_BLACK:
			cur_reg = &dev->shadow_reg.reg_black;
			break;
		case RE1FPGA_REG_WHITE:
			cur_reg = &dev->shadow_reg.reg_white;
			break;
		case RE1FPGA_REG_THR:
			cur_reg = &dev->shadow_reg.reg_thr;
			break;
		case RE1FPGA_REG_XCFG:
			cur_reg = &dev->shadow_reg.reg_xcfg;
			break;
		case RE1FPGA_REG_XCFG2:
			cur_reg = &dev->shadow_reg.reg_xcfg2;
			break;
		case RE1FPGA_REG_IRCFG:
			cur_reg = &dev->shadow_reg.reg_ircfg;
			break;

		default:
		case RE1FPGA_REG_LED:
			return -2;
	}

	uint8_t new_data = (*cur_reg & ~mask) | (data & mask);


	if (new_data == *cur_reg) {
		return 0;
	}

	*cur_reg = new_data;

	return PIOS_RE1FPGA_WriteRegDirect(reg, new_data);
}

static int32_t PIOS_RE1FPGA_WriteRegDirect(enum pios_re1fpga_register reg, uint8_t data)
{

	if (PIOS_RE1FPGA_Validate(dev) != 0)
		return -1;

	if (PIOS_RE1FPGA_ClaimBus() != 0)
		return -3;

	PIOS_SPI_TransferByte(dev->spi_id, 0x7f & reg);
	PIOS_SPI_TransferByte(dev->spi_id, data);

	PIOS_RE1FPGA_ReleaseBus();

	return 0;
}

/**
 * @brief Read a register from RE1FPGA
 * @returns The register value
 * @param reg[in] Register address to be read
 */
static uint8_t PIOS_RE1FPGA_ReadReg(enum pios_re1fpga_register reg)
{
	uint8_t data;

	PIOS_RE1FPGA_ClaimBus();

	PIOS_SPI_TransferByte(dev->spi_id, 0x80 | reg); // request byte
	data = PIOS_SPI_TransferByte(dev->spi_id, 0);   // receive response

	PIOS_RE1FPGA_ReleaseBus();

	return data;
}


/**
 * @brief Read all registers and update shadow registers
 */
static void update_shadow_regs(void)
{
	dev->shadow_reg.reg_hwrev = PIOS_RE1FPGA_ReadReg(RE1FPGA_REG_HWREV);
	dev->shadow_reg.reg_cfg = PIOS_RE1FPGA_ReadReg(RE1FPGA_REG_CFG);
	dev->shadow_reg.reg_ctl = PIOS_RE1FPGA_ReadReg(RE1FPGA_REG_CTL);
	dev->shadow_reg.reg_black = PIOS_RE1FPGA_ReadReg(RE1FPGA_REG_BLACK);
	dev->shadow_reg.reg_white = PIOS_RE1FPGA_ReadReg(RE1FPGA_REG_WHITE);
	dev->shadow_reg.reg_thr = PIOS_RE1FPGA_ReadReg(RE1FPGA_REG_THR);
	dev->shadow_reg.reg_xcfg = PIOS_RE1FPGA_ReadReg(RE1FPGA_REG_XCFG);
	dev->shadow_reg.reg_xcfg2 = PIOS_RE1FPGA_ReadReg(RE1FPGA_REG_XCFG2);
	dev->shadow_reg.reg_ircfg = PIOS_RE1FPGA_ReadReg(RE1FPGA_REG_IRCFG);
}

/**
 * @brief Get the Hardware
 */
uint8_t PIOS_RE1FPGA_GetHWRevision()
{
	return dev->shadow_reg.reg_hwrev;
}


/**
 * @brief Set programmable LED (WS2812B) colors
 */
int32_t PIOS_RE1FPGA_SetLEDs(const uint8_t * led_data, uint16_t n_leds)
{
	if (PIOS_RE1FPGA_ClaimBus() != 0)
		return -1;

	n_leds = MIN(n_leds, 1024);

	PIOS_SPI_TransferByte(dev->spi_id, 0x7f & RE1FPGA_REG_LED);
	PIOS_SPI_TransferBlock(dev->spi_id, led_data, NULL, 3 * n_leds, NULL);

	PIOS_RE1FPGA_ReleaseBus();

	return 0;
}

/**
 * @brief Set programmable LED (WS2812B) color
 */
#define LED_BLOCK_SIZE 16
int32_t PIOS_RE1FPGA_SetLEDColor(uint16_t n_leds, uint8_t red, uint8_t green, uint8_t blue)
{

	uint8_t LED_DATA[LED_BLOCK_SIZE * 3];

	n_leds = MIN(n_leds, 1024);

	if (PIOS_RE1FPGA_ClaimBus() != 0)
		return -1;

	for (int i=0; i<LED_BLOCK_SIZE; i++) {
		LED_DATA[3 * i] = green;
		LED_DATA[3 * i + 1] = red;
		LED_DATA[3 * i + 2] = blue;
	}

	PIOS_SPI_TransferByte(dev->spi_id, 0x7f & RE1FPGA_REG_LED);

	for (int i=0; i<n_leds/LED_BLOCK_SIZE; i++) {
		PIOS_SPI_TransferBlock(dev->spi_id, LED_DATA, NULL, 3 * LED_BLOCK_SIZE, NULL);
	}

	if (n_leds % LED_BLOCK_SIZE != 0) {
		PIOS_SPI_TransferBlock(dev->spi_id, LED_DATA, NULL, 3 * (n_leds % LED_BLOCK_SIZE), NULL);
	}

	PIOS_RE1FPGA_ReleaseBus();

	return 0;
}

/**
 * @brief Set the protocol for the IR transmitter
 */
int32_t PIOS_RE1FPGA_SetIRProtocol(enum pios_re1fpga_ir_protocols ir_protocol)
{
	uint8_t value = 0;

	switch (ir_protocol) {
		case PIOS_RE1FPGA_IR_PROTOCOL_OFF:
			value = 0;
			break;
		case PIOS_RE1FPGA_IR_PROTOCOL_ILAP:
		case PIOS_RE1FPGA_IR_PROTOCOL_TRACKMATE:
			/* They use the same basic protocol */
			value = 0x01;
			break;
	}

	return PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_IRCFG, value, 0x0F);
}

/**
 * @brief Set IR emitter data
 */
int32_t PIOS_RE1FPGA_SetIRData(const uint8_t * ir_data, uint8_t n_bytes)
{
	if (n_bytes > 16)
		return - 1;

	if (PIOS_RE1FPGA_ClaimBus() != 0)
		return -2;


	PIOS_SPI_TransferByte(dev->spi_id, 0x7f & RE1FPGA_REG_IRDATA);
	PIOS_SPI_TransferBlock(dev->spi_id, ir_data, NULL, n_bytes, NULL);

	PIOS_RE1FPGA_ReleaseBus();

	return 0;
}

/**
 * @brief Enable / disable the serial RX inverter
 */
int32_t PIOS_RE1FPGA_SerialRxInvert(bool invert)
{
	uint8_t data;
	if (invert) {
		data = 0x01;
	}
	else {
		data = 0x00;
	}
	return PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_CFG, data, 0x01);
}

/**
 * @brief Set MultiPort TX pin mode
 */
int32_t PIOS_RE1FPGA_MPTxPinMode(bool bidrectional, bool invert)
{
	uint8_t data = 0;

	data |= bidrectional << 1;
	data |= invert << 2;

	return PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_CFG, data, 0x06);
}

/**
 * @brief Set MultiPort TX pull-up / pull-down resistor
 */
int32_t PIOS_RE1FPGA_MPTxPinPullUpDown(bool enable, bool pullup)
{
	uint8_t data = 0;

	data |= enable << 3;
	data |= pullup << 4;

	return PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_CFG, data, 0x18);
}

/**
 * @brief Enable / disable buzzer
 */
int32_t PIOS_RE1FPGA_Buzzer(bool enable)
{
	uint8_t data = enable;

	return PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_CTL, data, 0x01);
}

/**
 * @brief Set the notification LED colors
 */
int32_t PIOS_RE1FPGA_SetNotificationLedColor(enum pios_re1fpga_led_colors led_colors)
{
	uint8_t value;

	if (led_colors == PIOS_RE1FPGA_STATUS_BLUE_CUSTOM_GREEN)
		value = 0;
	else
		value = 255;

	return PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_CFG, value, 0x20);
}

/**
 * @brief Set OSD black and white levels
 */
void PIOS_RE1FPGA_SetBwLevels(uint8_t black, uint8_t white)
{
	PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_BLACK, black, 0xFF);
	PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_WHITE, white, 0xFF);
}

/**
 * @brief Set OSD x offset
 */
void PIOS_RE1FPGA_SetXOffset(int8_t x_offset)
{
	//x_offset += 5;
	if (x_offset >= 7)
		x_offset = 7;
	if (x_offset < -8)
		x_offset = -8;

	uint8_t value = 8 + x_offset;
	PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_XCFG, value, 0x0F);
}

/**
 * @brief Set OSD x scale
 */
void PIOS_RE1FPGA_SetXScale(uint8_t x_scale)
{
	x_scale = (x_scale & 0x0F) << 4;
	PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_XCFG, x_scale, 0xF0);
}

/**
 * @brief Set 3D mode configuration
 */
void PIOS_RE1FPGA_Set3DConfig(enum pios_video_3d_mode mode, uint8_t x_shift_right)
{
	uint8_t cfg;
	uint8_t enabled = (mode == PIOS_VIDEO_3D_SBS3D);

	cfg = (enabled << 6) | (x_shift_right & 0x3F);
	PIOS_RE1FPGA_WriteReg(RE1FPGA_REG_XCFG2, cfg, 0xFF);
}

#endif /* defined(PIOS_INCLUDE_RE1_FPGA) */




