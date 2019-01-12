/**
 ******************************************************************************
 * @file       pios_esctelemetry.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_ESCTelemetry PiOS TBS ESC telemetry decoder
 * @{
 * @brief Receives and decodes ESC telemetry packages
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

#include "pios.h"

#ifndef PIOS_DSHOTTELEMETRY_H
#define PIOS_DSHOTTELEMETRY_H

#define DSHOT_TELEMETRY_LENGTH			10

struct pios_dshottelemetry_info {
	float voltage;
	float current;
	uint16_t mAh;
	uint32_t rpm;
	uint8_t temperature;
};

int PIOS_DShotTelemetry_Init(uintptr_t *esctelem_id, const struct pios_com_driver *driver,
		uintptr_t usart_id);

bool PIOS_DShotTelemetry_IsAvailable();

bool PIOS_DShotTelemetry_DataAvailable();

void PIOS_DShotTelemetry_Get(struct pios_dshottelemetry_info *t);

#endif // PIOS_ESCTELEMETRY_H

  /**
  * @}
  * @}
  */
