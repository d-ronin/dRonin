/**
 ******************************************************************************
 * @file       pios_flyingpio.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

#ifndef PIOS_FLYINGPIO_H
#define PIOS_FLYINGPIO_H

#include "pios.h"

typedef struct pios_flyingpio_dev * pios_flyingpio_dev_t;

/**
 * @brief Initialize the FlyingPIO programmed-IO expander
 * @return 0 for success, -1 for failure to allocate
 */
int32_t PIOS_FLYINGPIO_SPI_Init(pios_flyingpio_dev_t *dev, uint32_t spi_id, uint32_t slave_idx);

/**
 * @brief Returns receiver data returned by flyingpio.
 * @return Channel value, or an error code from pios_rcvr.h
 */
int32_t PIOS_FLYINGPIO_Receiver_Get(uintptr_t dev_int, uint8_t channel);

extern const struct pios_rcvr_driver pios_flyingpio_rcvr_driver;
extern const struct pios_adc_driver pios_flyingpio_adc_driver;

#endif /* PIOS_FLYINGPIO_H */

/** 
  * @}
  * @}
  */
