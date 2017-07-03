/**
 ******************************************************************************
 *
 * @file       pios_i2c_priv.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @brief      I2C private definitions.
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

#ifndef PIOS_I2C_PRIV_H
#define PIOS_I2C_PRIV_H

#include <pios.h>

int32_t PIOS_I2C_Init(pios_i2c_t *i2c_id, const char *path);

#endif /* PIOS_I2C_PRIV_H */
