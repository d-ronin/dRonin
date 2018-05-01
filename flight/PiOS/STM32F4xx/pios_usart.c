/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_USART USART Functions
 * @brief PIOS interface for USART port
 * @{
 *
 * @file       pios_usart.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @brief      USART commands. Inits USARTs, controls USARTs & Interupt handlers. (STM32 dependent)
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

/*
 * @todo This is virtually identical to the F1xx driver and should be merged.
 */

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_USART)

#include <pios_usart_priv.h>

/* DMA buffer sizes. */
#define PIOS_USART_DMA_SEND_BUFFER_SZ	64
#define PIOS_USART_DMA_RECV_BUFFER_SZ	64

/* Provide a COM driver */
static void PIOS_USART_ChangeBaud(uintptr_t usart_id, uint32_t baud);
static void PIOS_USART_RegisterRxCallback(uintptr_t usart_id, pios_com_callback rx_in_cb, uintptr_t context);
static void PIOS_USART_RegisterTxCallback(uintptr_t usart_id, pios_com_callback tx_out_cb, uintptr_t context);
static void PIOS_USART_TxStart(uintptr_t usart_id, uint16_t tx_bytes_avail);
static void PIOS_USART_RxStart(uintptr_t usart_id, uint16_t rx_bytes_avail);

const struct pios_com_driver pios_usart_com_driver = {
	.set_baud   = PIOS_USART_ChangeBaud,
	.tx_start   = PIOS_USART_TxStart,
	.rx_start   = PIOS_USART_RxStart,
	.bind_tx_cb = PIOS_USART_RegisterTxCallback,
	.bind_rx_cb = PIOS_USART_RegisterRxCallback,
};

enum pios_usart_dev_magic {
	PIOS_USART_DEV_MAGIC = 0x4152834A,
};

struct pios_usart_dev {
	enum pios_usart_dev_magic     magic;
	const struct pios_usart_cfg * cfg;

	pios_com_callback rx_in_cb;
	uintptr_t rx_in_context;
	pios_com_callback tx_out_cb;
	uintptr_t tx_out_context;
	uint32_t dma_send_buffer;
	uint32_t dma_recv_buffer;
};

static bool PIOS_USART_validate(struct pios_usart_dev * usart_dev)
{
	return (usart_dev->magic == PIOS_USART_DEV_MAGIC);
}

static struct pios_usart_dev * PIOS_USART_alloc(void)
{
	struct pios_usart_dev * usart_dev;

	usart_dev = (struct pios_usart_dev *)PIOS_malloc(sizeof(*usart_dev));
	if (!usart_dev) return(NULL);

	memset(usart_dev, 0, sizeof(*usart_dev));
	usart_dev->magic = PIOS_USART_DEV_MAGIC;
	return(usart_dev);
}

/* Bind Interrupt Handlers
 *
 * Map all valid USART IRQs to the common interrupt handler
 * and provide storage for a 32-bit device id IRQ to map
 * each physical IRQ to a specific registered device instance.
 */
static void PIOS_USART_generic_irq_handler(uintptr_t usart_id);

static uintptr_t PIOS_USART_1_id;
void USART1_IRQHandler(void) __attribute__ ((alias ("PIOS_USART_1_irq_handler")));
static void PIOS_USART_1_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_USART_1_id);
	PIOS_IRQ_Epilogue();
}

static uintptr_t PIOS_USART_2_id;
void USART2_IRQHandler(void) __attribute__ ((alias ("PIOS_USART_2_irq_handler")));
static void PIOS_USART_2_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_USART_2_id);
	PIOS_IRQ_Epilogue();
}

static uintptr_t PIOS_USART_3_id;
void USART3_IRQHandler(void) __attribute__ ((alias ("PIOS_USART_3_irq_handler")));
static void PIOS_USART_3_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_USART_3_id);
	PIOS_IRQ_Epilogue();
}

static uintptr_t PIOS_USART_4_id;
void USART4_IRQHandler(void) __attribute__ ((alias ("PIOS_USART_4_irq_handler")));
static void PIOS_USART_4_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_USART_4_id);
	PIOS_IRQ_Epilogue();
}

static uintptr_t PIOS_USART_5_id;
void USART5_IRQHandler(void) __attribute__ ((alias ("PIOS_USART_5_irq_handler")));
static void PIOS_USART_5_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_USART_5_id);
	PIOS_IRQ_Epilogue();
}

static uintptr_t PIOS_USART_6_id;
void USART6_IRQHandler(void) __attribute__ ((alias ("PIOS_USART_6_irq_handler")));
static void PIOS_USART_6_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_USART_6_id);
	PIOS_IRQ_Epilogue();
}


/**
* Initialise a single USART device
*/
int32_t PIOS_USART_Init(uintptr_t * usart_id, const struct pios_usart_cfg * cfg, struct pios_usart_params * params)
{
	PIOS_DEBUG_Assert(usart_id);
	PIOS_DEBUG_Assert(cfg);

	struct pios_usart_dev * usart_dev;

	usart_dev = (struct pios_usart_dev *) PIOS_USART_alloc();
	if (!usart_dev) goto out_fail;

	/* Bind the configuration to the device instance */
	usart_dev->cfg = cfg;

	/* Map pins to USART function */
	/* note __builtin_ctz() due to the difference between GPIO_PinX and GPIO_PinSourceX */
	if (usart_dev->cfg->remap) {
		if (usart_dev->cfg->rx.gpio != 0)
			GPIO_PinAFConfig(usart_dev->cfg->rx.gpio,
				__builtin_ctz(usart_dev->cfg->rx.init.GPIO_Pin),
				usart_dev->cfg->remap);
		if (usart_dev->cfg->tx.gpio != 0)
			GPIO_PinAFConfig(usart_dev->cfg->tx.gpio,
				__builtin_ctz(usart_dev->cfg->tx.init.GPIO_Pin),
				usart_dev->cfg->remap);
	}

	/* Initialize the USART Rx and Tx pins */
	if (usart_dev->cfg->rx.gpio != 0)
		GPIO_Init(usart_dev->cfg->rx.gpio, (GPIO_InitTypeDef *)&usart_dev->cfg->rx.init);
	if (usart_dev->cfg->tx.gpio != 0)
		GPIO_Init(usart_dev->cfg->tx.gpio, (GPIO_InitTypeDef *)&usart_dev->cfg->tx.init);

	/* Enable single wire mode if requested */
	if (params->single_wire == true)
		USART_HalfDuplexCmd(usart_dev->cfg->regs, ENABLE);
	else
		USART_HalfDuplexCmd(usart_dev->cfg->regs, DISABLE);

	/* Configure the USART */
	USART_Init(usart_dev->cfg->regs, (USART_InitTypeDef *)&params->init);

	*usart_id = (uintptr_t)usart_dev;

	/* Configure USART Interrupts */
	switch ((uint32_t)usart_dev->cfg->regs) {
	case (uint32_t)USART1:
		PIOS_USART_1_id = (uintptr_t)usart_dev;
		break;
	case (uint32_t)USART2:
		PIOS_USART_2_id = (uintptr_t)usart_dev;
		break;
	case (uint32_t)USART3:
		PIOS_USART_3_id = (uintptr_t)usart_dev;
		break;
	case (uint32_t)UART4:
		PIOS_USART_4_id = (uintptr_t)usart_dev;
		break;
	case (uint32_t)UART5:
		PIOS_USART_5_id = (uintptr_t)usart_dev;
		break;
	case (uint32_t)USART6:
		PIOS_USART_6_id = (uintptr_t)usart_dev;
		break;
	}
	NVIC_Init((NVIC_InitTypeDef *)&(usart_dev->cfg->irq.init));

	// FIXME XXX Clear / reset uart here - sends NUL char else

	/* Check DMA transmit configuration and prepare DMA. */
	if (cfg->dma_send && (params->flags & PIOS_USART_ALLOW_TRANSMIT_DMA)) {
		usart_dev->dma_send_buffer = (uint32_t)PIOS_malloc(PIOS_USART_DMA_SEND_BUFFER_SZ);

		if (usart_dev->dma_send_buffer) {
			DMA_DeInit(cfg->dma_send->stream);
			DMA_Init(cfg->dma_send->stream, &cfg->dma_send->init);
			NVIC_Init(&cfg->dma_send->irq);
		}
	}

	/* Check DMA receive configuration, and setup and start DMA. */
	if (cfg->dma_recv && (params->flags & PIOS_USART_ALLOW_RECEIVE_DMA)) {
		usart_dev->dma_recv_buffer = (uint32_t)PIOS_malloc(PIOS_USART_DMA_RECV_BUFFER_SZ);

		if (usart_dev->dma_recv_buffer) {
			DMA_DeInit(cfg->dma_recv->stream);
			DMA_Init(cfg->dma_recv->stream, &cfg->dma_recv->init);
			NVIC_Init(&cfg->dma_recv->irq);

			USART_ITConfig(cfg->regs, USART_IT_IDLE, ENABLE);

			/* Start DMA here already. */
			DMA_ITConfig(cfg->dma_recv->stream, DMA_IT_TC, ENABLE);
			DMA_MemoryTargetConfig(cfg->dma_recv->stream, usart_dev->dma_recv_buffer, DMA_Memory_0);
			DMA_SetCurrDataCounter(cfg->dma_recv->stream, PIOS_USART_DMA_RECV_BUFFER_SZ);

			USART_DMACmd(cfg->regs, USART_DMAReq_Rx, ENABLE);
			DMA_Cmd(cfg->dma_recv->stream, ENABLE);
		}
	}

	if (!usart_dev->dma_recv_buffer) {
		/* If DMA isn't in-use or we failed to allocate the buffer, use RXNE interrupt for data. */
		USART_ITConfig(cfg->regs, USART_IT_RXNE, ENABLE);
	}

	USART_ITConfig(cfg->regs, USART_IT_TXE,  ENABLE);

	/* Enable USART */
	USART_Cmd(cfg->regs, ENABLE);

	return(0);

out_fail:
	return(-1);
}

static void PIOS_USART_RxStart(uintptr_t usart_id, uint16_t rx_bytes_avail)
{
	struct pios_usart_dev * usart_dev = (struct pios_usart_dev *)usart_id;
	
	bool valid = PIOS_USART_validate(usart_dev);
	PIOS_Assert(valid);
	
	USART_ITConfig(usart_dev->cfg->regs, USART_IT_RXNE, ENABLE);
}
static void PIOS_USART_TxStart(uintptr_t usart_id, uint16_t tx_bytes_avail)
{
	struct pios_usart_dev * usart_dev = (struct pios_usart_dev *)usart_id;
	
	bool valid = PIOS_USART_validate(usart_dev);
	PIOS_Assert(valid);

	if (!usart_dev->dma_send_buffer || DMA_GetCmdStatus(usart_dev->cfg->dma_send->stream) == DISABLE) {
		USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, ENABLE);
	}
}

/**
* Changes the baud rate of the USART peripheral without re-initialising.
* \param[in] usart_id USART name (GPS, TELEM, AUX)
* \param[in] baud Requested baud rate
*/
static void PIOS_USART_ChangeBaud(uintptr_t usart_id, uint32_t baud)
{
	struct pios_usart_dev * usart_dev = (struct pios_usart_dev *)usart_id;

	bool valid = PIOS_USART_validate(usart_dev);
	PIOS_Assert(valid);

	USART_InitTypeDef USART_InitStructure;

	/* Adjust the baud rate */
	USART_InitStructure.USART_BaudRate = baud;

	/* Get current parameters */
	USART_InitStructure.USART_WordLength          = usart_dev->cfg->regs->CR1 &  (uint32_t)USART_CR1_M;
	USART_InitStructure.USART_Parity              = usart_dev->cfg->regs->CR1 & ((uint32_t)USART_CR1_PCE  | (uint32_t)USART_CR1_PS);
	USART_InitStructure.USART_StopBits            = usart_dev->cfg->regs->CR2 &  (uint32_t)USART_CR2_STOP;
	USART_InitStructure.USART_HardwareFlowControl = usart_dev->cfg->regs->CR3 & ((uint32_t)USART_CR3_CTSE | (uint32_t)USART_CR3_RTSE);
	USART_InitStructure.USART_Mode                = usart_dev->cfg->regs->CR1 & ((uint32_t)USART_CR1_TE   | (uint32_t)USART_CR1_RE) ;

	/* Write back the new configuration */
	USART_Init(usart_dev->cfg->regs, &USART_InitStructure);
}

static void PIOS_USART_RegisterRxCallback(uintptr_t usart_id, pios_com_callback rx_in_cb, uintptr_t context)
{
	struct pios_usart_dev * usart_dev = (struct pios_usart_dev *)usart_id;

	bool valid = PIOS_USART_validate(usart_dev);
	PIOS_Assert(valid);
	
	/* 
	 * Order is important in these assignments since ISR uses _cb
	 * field to determine if it's ok to dereference _cb and _context
	 */
	usart_dev->rx_in_context = context;
	usart_dev->rx_in_cb = rx_in_cb;
}

static void PIOS_USART_RegisterTxCallback(uintptr_t usart_id, pios_com_callback tx_out_cb, uintptr_t context)
{
	struct pios_usart_dev * usart_dev = (struct pios_usart_dev *)usart_id;

	bool valid = PIOS_USART_validate(usart_dev);
	PIOS_Assert(valid);
	
	/* 
	 * Order is important in these assignments since ISR uses _cb
	 * field to determine if it's ok to dereference _cb and _context
	 */
	usart_dev->tx_out_context = context;
	usart_dev->tx_out_cb = tx_out_cb;
}

static void PIOS_USART_generic_irq_handler(uintptr_t usart_id)
{
	struct pios_usart_dev * usart_dev = (struct pios_usart_dev *)usart_id;

	bool valid = PIOS_USART_validate(usart_dev);
	PIOS_Assert(valid);
	
	/* Force read of dr after sr to make sure to clear error flags */
	volatile uint16_t sr = usart_dev->cfg->regs->SR;
	volatile uint8_t dr = usart_dev->cfg->regs->DR;
	
	/* Check if RXNE flag is set */
	bool rx_need_yield = false;
	if (sr & USART_SR_RXNE) {
		uint8_t byte = dr;
		if (usart_dev->rx_in_cb) {
			(void) (usart_dev->rx_in_cb)(usart_dev->rx_in_context, &byte, 1, NULL, &rx_need_yield);
		}
	}

	if (sr & USART_SR_IDLE) {
		/* If line goes idle, trigger DMA IRQ for data processing. */
		DMA_Cmd(usart_dev->cfg->dma_recv->stream, DISABLE);
		while (DMA_GetCmdStatus(usart_dev->cfg->dma_recv->stream) == ENABLE) ;
	}
	
	/* Check if TXE flag is set */
	bool tx_need_yield = false;
	if (sr & USART_SR_TXE) {
		if (usart_dev->tx_out_cb) {

			if (usart_dev->dma_send_buffer) {
				/* Here's how it goes:

				   TXE interrupt triggers, because USART has nothing to do. If there's more than 1 byte to send,
				   the TXE IRQ gets disabled and a single DMA transfer gets triggered. When that transfer is done,
				   it'll reenable TXE IRQ, to repeat this whole thing. */

				if (DMA_GetCmdStatus(usart_dev->cfg->dma_send->stream) == ENABLE) {
					/* If we get here while DMA is enabled, for whatever reason, GTFO.
					   Also, re-disable TXE. */
					USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, DISABLE);
					return;
				}

				uint16_t bytes_to_send = (usart_dev->tx_out_cb)(usart_dev->tx_out_context, (uint8_t*)usart_dev->dma_send_buffer,
						PIOS_USART_DMA_SEND_BUFFER_SZ, NULL, &tx_need_yield);

				if (bytes_to_send > 1) {
					USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, DISABLE);

					DMA_MemoryTargetConfig(usart_dev->cfg->dma_send->stream, usart_dev->dma_send_buffer, DMA_Memory_0);
					DMA_SetCurrDataCounter(usart_dev->cfg->dma_send->stream, bytes_to_send);
					DMA_ITConfig(usart_dev->cfg->dma_send->stream, DMA_IT_TC, ENABLE);

					USART_DMACmd(usart_dev->cfg->regs, USART_DMAReq_Tx, ENABLE);
					DMA_Cmd(usart_dev->cfg->dma_send->stream, ENABLE);
				} else if (bytes_to_send == 1) {
					/* Don't use DMA for single byte. */
					usart_dev->cfg->regs->DR = *((uint8_t*)usart_dev->dma_send_buffer);
				} else {
					USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, DISABLE);
				}
			} else {
				/* Send byte-by-byte. */
				uint8_t byte;
				uint16_t bytes_to_send = (usart_dev->tx_out_cb)(usart_dev->tx_out_context, &byte, 1, NULL, &tx_need_yield);

				if (bytes_to_send > 0) {
					/* Send the byte we've been given */
					usart_dev->cfg->regs->DR = byte;
				} else {
					/* No bytes to send, disable TXE interrupt */
					USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, DISABLE);
				}
			}
		/* if (usart_dev->dma_send_buffer) */
		} else {
			/* No bytes to send, disable TXE interrupt */
			USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, DISABLE);
		}
	}
}

void PIOS_USART_dma_irq_tx_handler(uintptr_t usart_id)
{
	struct pios_usart_dev *usart_dev = (struct pios_usart_dev*)usart_id;
	if (!usart_dev)
		return;

	const struct pios_usart_cfg *cfg = usart_dev->cfg;

	struct pios_usart_dma_cfg *dmacfg = cfg->dma_send;
	PIOS_Assert(dmacfg);

	if (DMA_GetFlagStatus(dmacfg->stream, dmacfg->tcif) == SET) {
		/* TX handler*/
		USART_DMACmd(cfg->regs, USART_DMAReq_Tx, DISABLE);

		DMA_Cmd(dmacfg->stream, DISABLE);
		while (DMA_GetCmdStatus(dmacfg->stream) == ENABLE) ;

		DMA_ITConfig(dmacfg->stream, DMA_IT_TC, DISABLE);
		DMA_ClearITPendingBit(dmacfg->stream, dmacfg->tcif);
		DMA_ClearFlag(dmacfg->stream, dmacfg->tcif);

		/* Re-enable TXE interrupt, which kicks off the next data transfer, if there's stuff. */
		USART_ITConfig(cfg->regs, USART_IT_TXE, ENABLE);
	}
}

void PIOS_USART_dma_irq_rx_handler(uintptr_t usart_id)
{
	struct pios_usart_dev *usart_dev = (struct pios_usart_dev*)usart_id;
	if (!usart_dev)
		return;

	struct pios_usart_dma_cfg *dmacfg = usart_dev->cfg->dma_recv;
	PIOS_Assert(dmacfg);

	/* RX handler*/
	if (DMA_GetFlagStatus(dmacfg->stream, dmacfg->tcif) == SET) {

		DMA_Cmd(dmacfg->stream, DISABLE);
		while (DMA_GetCmdStatus(dmacfg->stream) == ENABLE) ;

		/* Need to figure out how much data was captured either way, because USART idle stops the DMA
		   causing this to trigger, too.*/
		uint32_t data_read = PIOS_USART_DMA_RECV_BUFFER_SZ - DMA_GetCurrDataCounter(dmacfg->stream);

		if (usart_dev->rx_in_cb && data_read > 0) {
			bool rx_need_yield = false;
			(usart_dev->rx_in_cb)(usart_dev->rx_in_context, (uint8_t*)usart_dev->dma_recv_buffer, data_read,
				NULL, &rx_need_yield);
		}

		DMA_ClearITPendingBit(dmacfg->stream, dmacfg->tcif);
		DMA_ClearFlag(dmacfg->stream, dmacfg->tcif);

		/* Restart DMA. */
		DMA_MemoryTargetConfig(dmacfg->stream, usart_dev->dma_recv_buffer, DMA_Memory_0);
		DMA_SetCurrDataCounter(dmacfg->stream, PIOS_USART_DMA_RECV_BUFFER_SZ);
		DMA_Cmd(dmacfg->stream, ENABLE);
	}
}

void PIOS_USART_1_dmarx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_rx_handler(PIOS_USART_1_id);
	PIOS_IRQ_Epilogue();
}
void PIOS_USART_1_dmatx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_tx_handler(PIOS_USART_1_id);
	PIOS_IRQ_Epilogue();
}

void PIOS_USART_2_dmarx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_rx_handler(PIOS_USART_2_id);
	PIOS_IRQ_Epilogue();
}
void PIOS_USART_2_dmatx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_tx_handler(PIOS_USART_2_id);
	PIOS_IRQ_Epilogue();
}

void PIOS_USART_3_dmarx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_rx_handler(PIOS_USART_3_id);
	PIOS_IRQ_Epilogue();
}
void PIOS_USART_3_dmatx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_tx_handler(PIOS_USART_3_id);
	PIOS_IRQ_Epilogue();
}

void PIOS_USART_4_dmarx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_rx_handler(PIOS_USART_4_id);
	PIOS_IRQ_Epilogue();
}
void PIOS_USART_4_dmatx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_tx_handler(PIOS_USART_4_id);
	PIOS_IRQ_Epilogue();
}

void PIOS_USART_5_dmarx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_rx_handler(PIOS_USART_5_id);
	PIOS_IRQ_Epilogue();
}
void PIOS_USART_5_dmatx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_tx_handler(PIOS_USART_5_id);
	PIOS_IRQ_Epilogue();
}

void PIOS_USART_6_dmarx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_rx_handler(PIOS_USART_6_id);
	PIOS_IRQ_Epilogue();
}
void PIOS_USART_6_dmatx_irq_handler(void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_dma_irq_tx_handler(PIOS_USART_6_id);
	PIOS_IRQ_Epilogue();
}

#endif

/**
  * @}
  * @}
  */
