/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SPI SPI Functions
 * @{
 *
 * @file       pios_spi.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      SPI functions header.
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

#ifndef PIOS_SPI_H
#define PIOS_SPI_H

struct pios_spi_cfg;

struct pios_spi_dev;

/* Public Functions */

/**
* Initializes a SPI device.
* \param[out] spi_id The handle of the device
* \param[in] cfg the SPI configuration
* \return zero on success, nonzero on failure
*/
int32_t PIOS_SPI_Init(uint32_t * spi_id, const struct pios_spi_cfg * cfg);

/**
 * (Re-)initialises SPI peripheral clock rate
 *
 * \param[in] spi SPI number (0 or 1)
 * \param[in] spi_speed configures the SPI speed in Hz
 * \return The actual attained/configured speed.
 */
int32_t PIOS_SPI_SetClockSpeed(uint32_t spi_id, uint32_t speed);

/**
* Controls the slave select on a SPI port.
* \param[in] spi_id the SPI handle
* \param[in] slave_id the index of the slave to select/deselect
* \param[in] pin_value true to deselect slave, false to select
* \return 0 if no error
*/
int32_t PIOS_SPI_RC_PinSet(uint32_t spi_id, uint32_t slave_id, bool pin_value);

/**
* Transfers a byte to SPI output and reads back the return value from SPI input
* \param[in] spi_id SPI handle
* \param[in] b the byte which should be transfered out
* \return The received byte
*/
uint8_t PIOS_SPI_TransferByte(uint32_t spi_id, uint8_t b);

/**
* Transfers a block of bytes
* \param[in] spi_id SPI device handle
* \param[in] send_buffer pointer to buffer which should be sent.<BR>
* If NULL, 0xff (all-one) will be sent.
* \param[in] receive_buffer pointer to buffer which should get the received values.<BR>
* If NULL, received bytes will be discarded.
* \param[in] len number of bytes which should be transfered
* \return >= 0 if no error during transfer
*/
int32_t PIOS_SPI_TransferBlock(uint32_t spi_id, const uint8_t *send_buffer, uint8_t *receive_buffer, uint16_t len);

/**
 * Claim the SPI bus semaphore.
 * \param[in] spi_id SPI handle
 * \return 0 if no error
 * \return -1 if timeout before claiming semaphore
 */
int32_t PIOS_SPI_ClaimBus(uint32_t spi_id);

/**
 * Release the SPI bus semaphore.
 * \param[in] spi_id SPI handle
 * \return 0 if no error
 */
int32_t PIOS_SPI_ReleaseBus(uint32_t spi_id);

void    PIOS_SPI_IRQ_Handler(uint32_t spi_id);

#endif /* PIOS_SPI_H */

/**
 * @}
 * @}
 */
