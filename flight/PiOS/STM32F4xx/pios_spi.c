/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SPI SPI Functions
 * @brief PIOS interface to read and write from SPI ports
 * @{
 *
 * @file       pios_spi.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @author     dRonin, http://dronin.org, Copyright (C) 2016
 * @brief      Hardware Abstraction Layer for SPI ports of STM32
 * @see        The GNU Public License (GPL) Version 3
 * @notes
 *
 * Note that additional chip select lines can be easily added by using
 * the remaining free GPIOs of the core module. Shared SPI ports should be
 * arbitrated with Mutexes to avoid collisions!
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
 */
/*
 * @todo	Clocking is wrong (interface is badly defined, should be speed not prescaler magic numbers)
 */
#include <pios.h>

#if defined(PIOS_INCLUDE_SPI)

#include <pios_spi_priv.h>

static bool PIOS_SPI_validate(struct pios_spi_dev *com_dev)
{
	/* Should check device magic here */
	return (true);
}

static struct pios_spi_dev *PIOS_SPI_alloc(void)
{
	return (PIOS_malloc(sizeof(struct pios_spi_dev)));
}

/**
* Initialises SPI pins
* \param[in] mode currently only mode 0 supported
* \return < 0 if initialisation failed
*/
int32_t PIOS_SPI_Init(uint32_t *spi_id, const struct pios_spi_cfg *cfg)
{
	uint32_t	init_ssel = 0;

	PIOS_Assert(spi_id);
	PIOS_Assert(cfg);

	struct pios_spi_dev *spi_dev;

	spi_dev = (struct pios_spi_dev *) PIOS_SPI_alloc();
	if (!spi_dev) goto out_fail;

	/* Bind the configuration to the device instance */
	spi_dev->cfg = cfg;

	spi_dev->busy = PIOS_Semaphore_Create();

	/* Set rx/tx dummy bytes to a known value */
	spi_dev->rx_dummy_byte = 0xFF;
	spi_dev->tx_dummy_byte = 0xFF;

	switch (spi_dev->cfg->init.SPI_NSS) {
	case SPI_NSS_Soft:
		if (spi_dev->cfg->init.SPI_Mode == SPI_Mode_Master) {
			/* We're a master in soft NSS mode, make sure we see NSS high at all times. */
			SPI_NSSInternalSoftwareConfig(spi_dev->cfg->regs, SPI_NSSInternalSoft_Set);
			/* Init as many slave selects as the config advertises. */
			init_ssel = spi_dev->cfg->slave_count;
		} else {
			/* We're a slave in soft NSS mode, make sure we see NSS low at all times. */
			SPI_NSSInternalSoftwareConfig(spi_dev->cfg->regs, SPI_NSSInternalSoft_Reset);
		}
		break;

	case SPI_NSS_Hard:
		/* only legal for single-slave config */
		PIOS_Assert(spi_dev->cfg->slave_count == 1);
		init_ssel = 1;
		SPI_SSOutputCmd(spi_dev->cfg->regs, (spi_dev->cfg->init.SPI_Mode == SPI_Mode_Master) ? ENABLE : DISABLE);
		/* FIXME: Should this also call SPI_SSOutputCmd()? */
		break;

	default:
		PIOS_Assert(0);
	}

	/* Initialize the GPIO pins */
	/* note __builtin_ctz() due to the difference between GPIO_PinX and GPIO_PinSourceX */
	if (spi_dev->cfg->remap) {
		GPIO_PinAFConfig(spi_dev->cfg->sclk.gpio,
		                 __builtin_ctz(spi_dev->cfg->sclk.init.GPIO_Pin),
		                 spi_dev->cfg->remap);
		GPIO_PinAFConfig(spi_dev->cfg->mosi.gpio,
		                 __builtin_ctz(spi_dev->cfg->mosi.init.GPIO_Pin),
		                 spi_dev->cfg->remap);
		GPIO_PinAFConfig(spi_dev->cfg->miso.gpio,
		                 __builtin_ctz(spi_dev->cfg->miso.init.GPIO_Pin),
		                 spi_dev->cfg->remap);
		for (uint32_t i = 0; i < init_ssel; i++) {
			GPIO_PinAFConfig(spi_dev->cfg->ssel[i].gpio,
			                 __builtin_ctz(spi_dev->cfg->ssel[i].init.GPIO_Pin),
			                 spi_dev->cfg->remap);
		}
	}
	GPIO_Init(spi_dev->cfg->sclk.gpio, (GPIO_InitTypeDef *) & (spi_dev->cfg->sclk.init));
	GPIO_Init(spi_dev->cfg->mosi.gpio, (GPIO_InitTypeDef *) & (spi_dev->cfg->mosi.init));
	GPIO_Init(spi_dev->cfg->miso.gpio, (GPIO_InitTypeDef *) & (spi_dev->cfg->miso.init));

	if (spi_dev->cfg->init.SPI_NSS != SPI_NSS_Hard) {
		for (uint32_t i = 0; i < init_ssel; i++) {
			/* Since we're driving the SSEL pin in software, ensure that the slave is deselected */
			/* XXX multi-slave support - maybe have another SPI_NSS_ mode? */
			GPIO_SetBits(spi_dev->cfg->ssel[i].gpio, spi_dev->cfg->ssel[i].init.GPIO_Pin);
			GPIO_Init(spi_dev->cfg->ssel[i].gpio, (GPIO_InitTypeDef *) & (spi_dev->cfg->ssel[i].init));
		}
	}

	/* Initialize the SPI block */
	SPI_I2S_DeInit(spi_dev->cfg->regs);
	SPI_Init(spi_dev->cfg->regs, (SPI_InitTypeDef *) & (spi_dev->cfg->init));

	/* Configure CRC calculation */
	if (spi_dev->cfg->use_crc) {
		SPI_CalculateCRC(spi_dev->cfg->regs, ENABLE);
	} else {
		SPI_CalculateCRC(spi_dev->cfg->regs, DISABLE);
	}

	/* Enable SPI */
	SPI_Cmd(spi_dev->cfg->regs, ENABLE);

	/* Must store this before enabling interrupt */
	*spi_id = (uint32_t)spi_dev;

	return (0);

out_fail:
	return (-1);
}

/**
 * (Re-)initialises SPI peripheral clock rate
 *
 * \param[in] spi SPI number (0 or 1)
 * \param[in] spi_prescaler configures the SPI speed:
 * <UL>
 *   <LI>PIOS_SPI_PRESCALER_2: sets clock rate 27.7~ nS @ 72 MHz (36 MBit/s) (only supported for spi==0, spi1 uses 4 instead)
 *   <LI>PIOS_SPI_PRESCALER_4: sets clock rate 55.5~ nS @ 72 MHz (18 MBit/s)
 *   <LI>PIOS_SPI_PRESCALER_8: sets clock rate 111.1~ nS @ 72 MHz (9 MBit/s)
 *   <LI>PIOS_SPI_PRESCALER_16: sets clock rate 222.2~ nS @ 72 MHz (4.5 MBit/s)
 *   <LI>PIOS_SPI_PRESCALER_32: sets clock rate 444.4~ nS @ 72 MHz (2.25 MBit/s)
 *   <LI>PIOS_SPI_PRESCALER_64: sets clock rate 888.8~ nS @ 72 MHz (1.125 MBit/s)
 *   <LI>PIOS_SPI_PRESCALER_128: sets clock rate 1.7~ nS @ 72 MHz (0.562 MBit/s)
 *   <LI>PIOS_SPI_PRESCALER_256: sets clock rate 3.5~ nS @ 72 MHz (0.281 MBit/s)
 * </UL>
 * \return 0 if no error
 * \return -1 if disabled SPI port selected
 * \return -3 if invalid spi_prescaler selected
 */
int32_t PIOS_SPI_SetClockSpeed(uint32_t spi_id, uint32_t spi_speed)
{
	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid);

	SPI_InitTypeDef SPI_InitStructure;

	SPIPrescalerTypeDef spi_prescaler;

	//SPI clock is different depending on the bus
	uint32_t spiBusClock = 0;

	if(spi_dev->cfg->regs == SPI1)
		spiBusClock = PIOS_SYSCLK / 2;
	else
		spiBusClock = PIOS_SYSCLK;

	//The needed prescaler for desired speed
	float desiredPrescaler=(float) spiBusClock / spi_speed;

	//Choosing the existing prescaler nearest the desiredPrescaler
	if(desiredPrescaler <= 2)
		spi_prescaler = PIOS_SPI_PRESCALER_2;
	else if(desiredPrescaler <= 4)
		spi_prescaler = PIOS_SPI_PRESCALER_4;
	else if(desiredPrescaler <= 8)
		spi_prescaler = PIOS_SPI_PRESCALER_8;
	else if(desiredPrescaler <= 16)
		spi_prescaler = PIOS_SPI_PRESCALER_16;
	else if(desiredPrescaler <= 32)
		spi_prescaler = PIOS_SPI_PRESCALER_32;
	else if(desiredPrescaler <= 64)
		spi_prescaler = PIOS_SPI_PRESCALER_64;
	else if(desiredPrescaler <= 128)
		spi_prescaler = PIOS_SPI_PRESCALER_128;
	else
		spi_prescaler = PIOS_SPI_PRESCALER_256;

	/* Start with a copy of the default configuration for the peripheral */
	SPI_InitStructure = spi_dev->cfg->init;

	/* Adjust the prescaler for the peripheral's clock */
	SPI_InitStructure.SPI_BaudRatePrescaler = ((uint16_t) spi_prescaler & 7) << 3;

	/* Write back the new configuration */
	SPI_Init(spi_dev->cfg->regs, &SPI_InitStructure);

	PIOS_SPI_TransferByte(spi_id, 0xFF);

	//return set speed
	return spiBusClock / spi_prescaler;
}

/**
 * Claim the SPI bus semaphore.  Calling the SPI functions does not require this
 * \param[in] spi SPI number (0 or 1)
 * \return 0 if no error
 * \return -1 if timeout before claiming semaphore
 */
int32_t PIOS_SPI_ClaimBus(uint32_t spi_id)
{
	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid)

	if (PIOS_Semaphore_Take(spi_dev->busy, 65535) != true)
		return -1;

	return 0;
}

/**
 * Claim the SPI bus semaphore from an ISR.  Has no timeout.
 * \param[in] spi SPI number (0 or 1)
 * \param[in] pointer which receives if a task has been woken
 * \return 0 if no error
 * \return -1 if timeout before claiming semaphore
 */
int32_t PIOS_SPI_ClaimBusISR(uint32_t spi_id, bool *woken)
{
	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid)

	if (PIOS_Semaphore_Take_FromISR(spi_dev->busy, woken) != true)
		return -1;

	return 0;
}


/**
 * Release the SPI bus semaphore.  Calling the SPI functions does not require this
 * \param[in] spi SPI number (0 or 1)
 * \return 0 if no error
 */
int32_t PIOS_SPI_ReleaseBus(uint32_t spi_id)
{
	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid)

	PIOS_Semaphore_Give(spi_dev->busy);

	return 0;
}

/**
 * Release the SPI bus from an ISR.
 * \param[in] spi SPI number (0 or 1)
 * \param[in] pointer which receives if a task has been woken
 * \return 0 if no error
 */
int32_t PIOS_SPI_ReleaseBusISR(uint32_t spi_id, bool *woken)
{
	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid)

	PIOS_Semaphore_Give_FromISR(spi_dev->busy, woken);

	return 0;
}

/**
* Controls the RC (Register Clock alias Chip Select) pin of a SPI port
* \param[in] spi SPI number (0 or 1)
* \param[in] pin_value 0 or 1
* \return 0 if no error
*/
int32_t PIOS_SPI_RC_PinSet(uint32_t spi_id, uint32_t slave_id, uint8_t pin_value)
{
	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid)
	PIOS_Assert(slave_id <= spi_dev->cfg->slave_count)

	/* XXX multi-slave support? */
	if (pin_value) {
		GPIO_SetBits(spi_dev->cfg->ssel[slave_id].gpio, spi_dev->cfg->ssel[slave_id].init.GPIO_Pin);
	} else {
		GPIO_ResetBits(spi_dev->cfg->ssel[slave_id].gpio, spi_dev->cfg->ssel[slave_id].init.GPIO_Pin);
	}

	return 0;
}

/**
* Transfers a byte to SPI output and reads back the return value from SPI input
* \param[in] spi SPI number (0 or 1)
* \param[in] b the byte which should be transfered
*/
int32_t PIOS_SPI_TransferByte(uint32_t spi_id, uint8_t b)
{
	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid)

	uint8_t rx_byte;

	/*
	 * Procedure taken from STM32F10xxx Reference Manual section 23.3.5
	 */

	/* Make sure the RXNE flag is cleared by reading the DR register */
	(void)spi_dev->cfg->regs->DR;

	/* Wait until the TXE goes high */
	while (SPI_I2S_GetFlagStatus(spi_dev->cfg->regs, SPI_I2S_FLAG_TXE) == RESET);

	/* Start the transfer */
	spi_dev->cfg->regs->DR = b;

	/* Wait until there is a byte to read */
	while (SPI_I2S_GetFlagStatus(spi_dev->cfg->regs, SPI_I2S_FLAG_RXNE) == RESET);

	/* Read the rx'd byte */
	rx_byte = spi_dev->cfg->regs->DR;

	/* Wait until the TXE goes high */
	while (!(spi_dev->cfg->regs->SR & SPI_I2S_FLAG_TXE));

	/* Wait for SPI transfer to have fully completed */
	while (spi_dev->cfg->regs->SR & SPI_I2S_FLAG_BSY);

	/* Return received byte */
	return rx_byte;
}

/**
* Transfers a block of bytes via PIO.
*
* \param[in] spi_id SPI device handle
* \param[in] send_buffer pointer to buffer which should be sent.<BR>
* If NULL, 0xff (all-one) will be sent.
* \param[in] receive_buffer pointer to buffer which should get the received values.<BR>
* If NULL, received bytes will be discarded.
* \param[in] len number of bytes which should be transfered
* \return >= 0 if no error during transfer
* \return -1 if disabled SPI port selected
*/
static int32_t SPI_PIO_TransferBlock(uint32_t spi_id, const uint8_t *send_buffer, uint8_t *receive_buffer, uint16_t len)
{
	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;
	uint8_t b;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid)

	while (len--) {
		/* get the byte to send */
		b = send_buffer ? *(send_buffer++) : 0xff;

		/* Wait until the TXE goes high */
		while (SPI_I2S_GetFlagStatus(spi_dev->cfg->regs, SPI_I2S_FLAG_TXE) == RESET);

		/* Start the transfer */
		spi_dev->cfg->regs->DR = b;

		/* Wait until there is a byte to read */
		while (SPI_I2S_GetFlagStatus(spi_dev->cfg->regs, SPI_I2S_FLAG_RXNE) == RESET);

		/* Read the rx'd byte */
		b = spi_dev->cfg->regs->DR;

		/* save the received byte */
		if (receive_buffer)
			*(receive_buffer++) = b;
	}

	/* Wait for SPI transfer to have fully completed */
	while (spi_dev->cfg->regs->SR & SPI_I2S_FLAG_BSY);

	return 0;
}


/**
* Transfers a block of bytes
* \param[in] spi_id SPI device handle
* \param[in] send_buffer pointer to buffer which should be sent.<BR>
* If NULL, 0xff (all-one) will be sent.
* \param[in] receive_buffer pointer to buffer which should get the received values.<BR>
* If NULL, received bytes will be discarded.
* \param[in] len number of bytes which should be transfered
* \return >= 0 if no error during transfer
* \return -1 if disabled SPI port selected
*/
int32_t PIOS_SPI_TransferBlock(uint32_t spi_id, const uint8_t *send_buffer, uint8_t *receive_buffer, uint16_t len)
{
	return SPI_PIO_TransferBlock(spi_id, send_buffer, receive_buffer, len);
}

/**
* Check if a transfer is in progress
* \param[in] spi SPI number (0 or 1)
* \return >= 0 if no transfer is in progress
* \return -3 if transfer in progress
*/
int32_t PIOS_SPI_Busy(uint32_t spi_id)
{
	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid)

	if (!SPI_I2S_GetFlagStatus(spi_dev->cfg->regs, SPI_I2S_FLAG_TXE) ||
	    SPI_I2S_GetFlagStatus(spi_dev->cfg->regs, SPI_I2S_FLAG_BSY)) {
		return -3;
	}

	return (0);
}

void PIOS_SPI_IRQ_Handler(uint32_t spi_id)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	struct pios_spi_dev *spi_dev = (struct pios_spi_dev *)spi_id;

	bool valid = PIOS_SPI_validate(spi_dev);
	PIOS_Assert(valid)

	if (spi_dev->cfg->init.SPI_Mode == SPI_Mode_Master) {
		/* Wait for the final bytes of the transfer to complete, including CRC byte(s). */
		while (!(SPI_I2S_GetFlagStatus(spi_dev->cfg->regs, SPI_I2S_FLAG_TXE))) ;

		/* Wait for the final bytes of the transfer to complete, including CRC byte(s). */
		while (SPI_I2S_GetFlagStatus(spi_dev->cfg->regs, SPI_I2S_FLAG_BSY)) ;
	}

#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */
}

#endif

/**
  * @}
  * @}
  */
