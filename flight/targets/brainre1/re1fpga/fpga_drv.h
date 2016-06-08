/**
 ******************************************************************************
 * @addtogroup dRonin Targets
 * @{
 * @addtogroup BrainRE1 support files
 * @{
 *
 * @file       fpga_drv.h
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

#ifndef PIOS_RE1FPGA_H
#define PIOS_RE1FPGA_H
#include "pios_video.h"

struct pios_re1fpga_cfg {
	struct stm32_gpio mco_pin; /* pin used for clock output to FPGA */
	struct stm32_gpio rst_pin; /* pin used to reset FPGA */
	struct stm32_gpio cdone_pin; /* configuration done pin */
	struct stm32_gpio cresetb_pin; /* configuration reset pin */
};

enum pios_re1fpga_led_colors {
	PIOS_RE1FPGA_STATUS_BLUE_CUSTOM_GREEN,
	PIOS_RE1FPGA_STATUS_GREEN_CUSTOM_BLUE,
};

enum pios_re1fpga_ir_protocols {
	PIOS_RE1FPGA_IR_PROTOCOL_OFF,
	PIOS_RE1FPGA_IR_PROTOCOL_ILAP,
	PIOS_RE1FPGA_IR_PROTOCOL_TRACKMATE,
};

enum pios_re1fpga_buzzer_types {
	PIOS_RE1FPGA_BUZZER_DC,
	PIOS_RE1FPGA_BUZZER_AC,
};


int32_t PIOS_RE1FPGA_Init(uint32_t spi_id, uint32_t slave_num, const struct pios_re1fpga_cfg *cfg, bool load_config);
uint8_t PIOS_RE1FPGA_GetHWRevision();
int32_t PIOS_RE1FPGA_SerialRxInvert(bool invert);
int32_t PIOS_RE1FPGA_MPTxPinMode(bool bidrectional, bool invert);
int32_t PIOS_RE1FPGA_MPTxPinPullUpDown(bool enable, bool pullup);
int32_t PIOS_RE1FPGA_SetBuzzerType(enum pios_re1fpga_buzzer_types type);
int32_t PIOS_RE1FPGA_Buzzer(bool enable);
int32_t PIOS_RE1FPGA_SetNotificationLedColor(enum pios_re1fpga_led_colors led_colors);
void PIOS_RE1FPGA_SetBwLevels(uint8_t black, uint8_t white);
int32_t PIOS_RE1FPGA_SetSyncThreshold(uint8_t threshold);
void PIOS_RE1FPGA_SetXOffset(int8_t x_offset);
void PIOS_RE1FPGA_SetXScale(uint8_t x_scale);
void PIOS_RE1FPGA_Set3DConfig(enum pios_video_3d_mode mode, uint8_t x_shift_right);
int32_t PIOS_RE1FPGA_SetLEDColor(uint16_t n_leds, uint8_t red, uint8_t green, uint8_t blue);
int32_t PIOS_RE1FPGA_SetIRProtocol(enum pios_re1fpga_ir_protocols ir_protocol);
int32_t PIOS_RE1FPGA_SetIRData(const uint8_t * ir_data, uint8_t n_bytes);

#endif /* PIOS_RE1FPGA_H */
