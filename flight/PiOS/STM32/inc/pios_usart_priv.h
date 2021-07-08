/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_USART USART Functions
 * @brief PIOS interface for USART port
 * @{
 *
 * @file       pios_usart_priv.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      USART private definitions.
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

#ifndef PIOS_USART_PRIV_H
#define PIOS_USART_PRIV_H

#include <pios.h>
#include <pios_stm32.h>
#include "pios_usart.h"

#define PIOS_USART_ALLOW_RECEIVE_DMA	0x1
#define PIOS_USART_ALLOW_TRANSMIT_DMA	0x2

extern const struct pios_com_driver pios_usart_com_driver;

#if defined(STM32F4XX)
struct pios_usart_dma_cfg {
	DMA_Stream_TypeDef *stream;
	DMA_InitTypeDef init;
	uint32_t tcif;
	NVIC_InitTypeDef irq;
};
#endif

struct pios_usart_cfg {
	USART_TypeDef *regs;
	uint32_t remap;		/* GPIO_Remap_* */
	struct stm32_gpio rx;
	struct stm32_gpio tx;
	struct stm32_irq irq;
#if defined(STM32F4XX)
	struct pios_usart_dma_cfg *dma_send;
	struct pios_usart_dma_cfg *dma_recv;
#endif
};

struct pios_usart_params {
	USART_InitTypeDef init;
	bool rx_invert;
	bool tx_invert;
	bool rxtx_swap;
	bool single_wire;
	uint8_t flags;
};

extern int32_t PIOS_USART_Init(uintptr_t * usart_id, const struct pios_usart_cfg * cfg, struct pios_usart_params * params);
extern const struct pios_usart_cfg * PIOS_USART_GetConfig(uintptr_t usart_id);

#if defined(STM32F4XX)

extern void PIOS_USART_1_dmarx_irq_handler(void);
extern void PIOS_USART_1_dmatx_irq_handler(void);

extern void PIOS_USART_2_dmarx_irq_handler(void);
extern void PIOS_USART_2_dmatx_irq_handler(void);

extern void PIOS_USART_3_dmarx_irq_handler(void);
extern void PIOS_USART_3_dmatx_irq_handler(void);

extern void PIOS_USART_4_dmarx_irq_handler(void);
extern void PIOS_USART_4_dmatx_irq_handler(void);

extern void PIOS_USART_5_dmarx_irq_handler(void);
extern void PIOS_USART_5_dmatx_irq_handler(void);

extern void PIOS_USART_6_dmarx_irq_handler(void);
extern void PIOS_USART_6_dmatx_irq_handler(void);

#endif

#endif /* PIOS_USART_PRIV_H */

/**
  * @}
  * @}
  */
