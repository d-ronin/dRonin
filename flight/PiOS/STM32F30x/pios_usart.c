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
 * @author     dRonin, http://dronin.org, Copyright (C) 2016
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

	uint32_t error_overruns;
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

	usart_dev->error_overruns = 0;

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
void USART1_EXTI25_IRQHandler(void) __attribute__ ((alias ("PIOS_USART_1_irq_handler")));
static void PIOS_USART_1_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_USART_1_id);
	PIOS_IRQ_Epilogue();
}

static uintptr_t PIOS_USART_2_id;
void USART2_EXTI26_IRQHandler(void) __attribute__ ((alias ("PIOS_USART_2_irq_handler")));
static void PIOS_USART_2_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_USART_2_id);
	PIOS_IRQ_Epilogue();
}

static uintptr_t PIOS_USART_3_id;
void USART3_EXTI28_IRQHandler(void) __attribute__ ((alias ("PIOS_USART_3_irq_handler")));
static void PIOS_USART_3_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_USART_3_id);
	PIOS_IRQ_Epilogue();
}

static uintptr_t PIOS_UART_4_id;
void UART4_EXTI34_IRQHandler(void) __attribute__ ((alias ("PIOS_UART_4_irq_handler")));
static void PIOS_UART_4_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_UART_4_id);
	PIOS_IRQ_Epilogue();
}

static uintptr_t PIOS_UART_5_id;
void UART5_EXTI35_IRQHandler(void) __attribute__ ((alias ("PIOS_UART_5_irq_handler")));
static void PIOS_UART_5_irq_handler (void)
{
	PIOS_IRQ_Prologue();
	PIOS_USART_generic_irq_handler (PIOS_UART_5_id);
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
	if (usart_dev->cfg->remap) {
		if (usart_dev->cfg->rx.gpio != 0)
			GPIO_PinAFConfig(usart_dev->cfg->rx.gpio,
				usart_dev->cfg->rx.pin_source,
				usart_dev->cfg->remap);
		if (usart_dev->cfg->tx.gpio != 0)
			GPIO_PinAFConfig(usart_dev->cfg->tx.gpio,
				usart_dev->cfg->tx.pin_source,
				usart_dev->cfg->remap);
	}

	/* Initialize the USART Rx and Tx pins */
	if (usart_dev->cfg->rx.gpio != 0)
		GPIO_Init(usart_dev->cfg->rx.gpio, (GPIO_InitTypeDef *)&usart_dev->cfg->rx.init);
	if (usart_dev->cfg->tx.gpio != 0)
		GPIO_Init(usart_dev->cfg->tx.gpio, (GPIO_InitTypeDef *)&usart_dev->cfg->tx.init);

	/* Apply inversion and swap settings */
	if (params->rx_invert == true)
		USART_InvPinCmd(usart_dev->cfg->regs, USART_InvPin_Rx, ENABLE);
	else
		USART_InvPinCmd(usart_dev->cfg->regs, USART_InvPin_Rx, DISABLE);

	if (params->tx_invert == true)
		USART_InvPinCmd(usart_dev->cfg->regs, USART_InvPin_Tx, ENABLE);
	else
		USART_InvPinCmd(usart_dev->cfg->regs, USART_InvPin_Tx, DISABLE);

	if (params->rxtx_swap == true)
		USART_SWAPPinCmd(usart_dev->cfg->regs, ENABLE);
	else
		USART_SWAPPinCmd(usart_dev->cfg->regs, DISABLE);

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
		PIOS_UART_4_id = (uintptr_t)usart_dev;
		break;
	case (uint32_t)UART5:
		PIOS_UART_5_id = (uintptr_t)usart_dev;
		break;
	}
	NVIC_Init((NVIC_InitTypeDef *)&(usart_dev->cfg->irq.init));
	USART_ITConfig(usart_dev->cfg->regs, USART_IT_RXNE, ENABLE);
	USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE,  ENABLE);

	// FIXME XXX Clear / reset uart here - sends NUL char else

	/* Enable USART */
	USART_Cmd(usart_dev->cfg->regs, ENABLE);

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
	
	USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, ENABLE);
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

	/* Write back the new configuration (Note this disables USART) */
	USART_Init(usart_dev->cfg->regs, &USART_InitStructure);

	/*
	 * Re enable USART.
	 */
	USART_Cmd(usart_dev->cfg->regs, ENABLE);
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

	bool rx_need_yield = false;
	bool tx_need_yield = false;

	bool valid = PIOS_USART_validate(usart_dev);
	PIOS_Assert(valid);
	
	/* Check if RXNE flag is set */
	if (USART_GetITStatus(usart_dev->cfg->regs, USART_IT_RXNE)) {
		uint8_t byte = (uint8_t)USART_ReceiveData(usart_dev->cfg->regs);
		if (usart_dev->rx_in_cb) {
			(void) (usart_dev->rx_in_cb)(usart_dev->rx_in_context, &byte, 1, NULL, &rx_need_yield);
		}
	}
	/* Check if TXE flag is set */
	if (USART_GetITStatus(usart_dev->cfg->regs, USART_IT_TXE)) {
		if (usart_dev->tx_out_cb) {
			uint8_t b;
			uint16_t bytes_to_send;
			
			bytes_to_send = (usart_dev->tx_out_cb)(usart_dev->tx_out_context, &b, 1, NULL, &tx_need_yield);
			
			if (bytes_to_send > 0) {
				/* Send the byte we've been given */
				USART_SendData(usart_dev->cfg->regs, b);
			} else {
				/* No bytes to send, disable TXE interrupt */
				USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, DISABLE);
			}
		} else {
			/* No bytes to send, disable TXE interrupt */
			USART_ITConfig(usart_dev->cfg->regs, USART_IT_TXE, DISABLE);
		}
	}
	/* Check for overrun condition
	 * Note i really wanted to use USART_GetITStatus but it fails on getting the
	 * ORE flag although RXNE interrupt is enabled.
	 * Probably a bug in the ST library...
	 */
	if (USART_GetFlagStatus(usart_dev->cfg->regs, USART_FLAG_ORE)) {
		USART_ClearITPendingBit(usart_dev->cfg->regs, USART_IT_ORE);
		++usart_dev->error_overruns;
	}
}

#endif

/**
  * @}
  * @}
  */
