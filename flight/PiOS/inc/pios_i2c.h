/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_I2C I2C Functions
 * @{
 *
 * @file       pios_i2c.h  
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      I2C functions header.
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

#ifndef PIOS_I2C_H
#define PIOS_I2C_H

#include <stdbool.h>

/* Global Types */
enum pios_i2c_txn_direction {
	PIOS_I2C_TXN_READ,
	PIOS_I2C_TXN_WRITE
};

struct pios_i2c_txn {
	const char *info;
	uint16_t addr;
	enum pios_i2c_txn_direction rw;
	uint32_t len;
	uint8_t *buf;
};

typedef struct pios_i2c_adapter *pios_i2c_t;

/* Public Functions */
extern int32_t PIOS_I2C_CheckClear(pios_i2c_t i2c_id);
extern int32_t PIOS_I2C_Transfer(pios_i2c_t i2c_id, const struct pios_i2c_txn txn_list[], uint32_t num_txns);
extern int32_t PIOS_I2C_Transfer_Callback(pios_i2c_t i2c_id, const struct pios_i2c_txn txn_list[], uint32_t, void *callback);
extern void PIOS_I2C_EV_IRQ_Handler(pios_i2c_t i2c_id);
extern void PIOS_I2C_ER_IRQ_Handler(pios_i2c_t i2c_id);
extern int32_t PIOS_I2C_TransferAsync(pios_i2c_t i2c_adapter,
		const struct pios_i2c_txn txn_list[], uint32_t num_txns,
		bool block_on_starting);
extern int32_t PIOS_I2C_WaitAsync(pios_i2c_t i2c_adapter, uint32_t timeout,
		bool *txn_completed);

#endif /* PIOS_I2C_H */

/**
  * @}
  * @}
  */
