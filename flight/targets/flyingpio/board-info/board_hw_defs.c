/**
 ******************************************************************************
 *
 * @file       flyingpio/board-info/board_hw_defs.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @addtogroup Targets Target Boards
 * @{
 * @addtogroup FlyingPIO FlyingPi IO Hat
 * @{
 * @brief      Defines board specific static initializers for hardware for the
 *             FlyingPIO IO expander.
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

#include <pios_config.h>
#include <pios_board_info.h>

enum board_revision {
	BOARD_REVISION_1 = 0x01,
};

#if defined(PIOS_INCLUDE_ANNUNC)

#include <pios_annunc_priv.h>
static const struct pios_annunc pios_annuncs[] = {
	[PIOS_LED_HEARTBEAT] = {
		.pin = {
			.gpio = GPIOB,
			.init = {
				.GPIO_Pin   = GPIO_Pin_0,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_OType = GPIO_OType_PP,
			},
		},
	},
};

static const struct pios_annunc_cfg pios_annunc_cfg = {
	.annunciators     = pios_annuncs,
	.num_annunciators = NELEMENTS(pios_annuncs),
};

const struct pios_annunc_cfg * PIOS_BOARD_HW_DEFS_GetLedCfg (uint32_t board_revision)
{
	return &pios_annunc_cfg;
}

#endif	/* PIOS_INCLUDE_ANNUNC */

#if defined(PIOS_INCLUDE_SPISLAVE)

#include "pios_spislave.h"

#include "flyingpio_messages.h"

static void process_pio_message(void *ctx, int len, int *resp_len);

/* SPI command link interface
 *
 * NOTE: Leave this declared as const data so that it ends up in the
 * .rodata section (ie. Flash) rather than in the .data section (RAM).
 */
struct flyingpi_msg rx_buf;
struct flyingpi_msg tx_buf;

static const struct pios_spislave_cfg pios_spislave_cfg = {
	.regs   = SPI1,
	.init   = {
		.SPI_Mode              = SPI_Mode_Slave,
		.SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
		.SPI_DataSize          = SPI_DataSize_8b,
		.SPI_NSS               = SPI_NSS_Hard,
		.SPI_FirstBit          = SPI_FirstBit_MSB,
		.SPI_CPOL              = SPI_CPOL_Low,
		.SPI_CPHA              = SPI_CPHA_1Edge,
		/* Next two not used */
		.SPI_CRCPolynomial     = 7,
		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
	},
	.sclk = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_3,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
	},
	.miso = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_4,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
	},
	.mosi = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_5,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_NOPULL
		},
	},
	.ssel = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_4,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd = GPIO_PuPd_UP
		}
	},
	.max_rx_len = sizeof(struct flyingpi_msg),
	.tx_dma = {
		.init = {
			.DMA_PeripheralBaseAddr = (uint32_t) &(SPI1->DR),
			.DMA_PeripheralInc = DMA_PeripheralInc_Disable,
			.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
			.DMA_MemoryBaseAddr = (uint32_t) &tx_buf,
			.DMA_MemoryInc = DMA_MemoryInc_Enable,
			.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
			.DMA_DIR = DMA_DIR_PeripheralDST,
			.DMA_Priority = DMA_Priority_High,
			.DMA_Mode = DMA_Mode_Normal,
			.DMA_M2M = DMA_M2M_Disable,
		},
		.channel = DMA1_Channel3,
	},
	.rx_dma = {
		.init = {
			.DMA_PeripheralBaseAddr = (uint32_t) &(SPI1->DR),
			.DMA_PeripheralInc = DMA_PeripheralInc_Disable,
			.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
			.DMA_MemoryBaseAddr = (uint32_t) &rx_buf,
			.DMA_MemoryInc = DMA_MemoryInc_Enable,
			.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
			.DMA_DIR = DMA_DIR_PeripheralSRC,
			.DMA_Priority = DMA_Priority_High,
			.DMA_Mode = DMA_Mode_Normal,
			.DMA_M2M = DMA_M2M_Disable,
		},
		.channel = DMA1_Channel2,
	},
	.process_message = process_pio_message,
};

#endif	/* PIOS_INCLUDE_SPI */

#ifdef PIOS_INCLUDE_ADC
#include "pios_adc_priv.h"
#include "pios_internal_adc_simple.h"

/**
 * ADC0 : PA4 ADC_IN4
 * ADC1 : PA5 ADC_IN5
 * ADC2 : PA1 ADC_IN1
 * ADC3 : PB1 ADC_IN9
 */
static const struct pios_internal_adc_simple_cfg internal_adc_cfg = {
	.adc_dev = ADC1,
	.adc_pins = {
		{GPIOA, GPIO_Pin_0, ADC_Channel_0 },  // VBat
		{GPIOA, GPIO_Pin_1, ADC_Channel_1 },  // ADC Pad
		{NULL,  0,          ADC_Channel_Vrefint },
		{NULL,  0,          ADC_Channel_TempSensor },
	},
	.adc_pin_count = 4,
};

#endif /* PIOS_INCLUDE_ADC */

#include "pios_tim_priv.h"

static const TIM_TimeBaseInitTypeDef tim_time_base = {
	.TIM_Prescaler = (PIOS_SYSCLK / 1000000) - 1,
	.TIM_ClockDivision = TIM_CKD_DIV1,
	.TIM_CounterMode = TIM_CounterMode_Up,
	.TIM_Period = ((1000000 / PIOS_SERVO_UPDATE_HZ) - 1),
	.TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_1_cfg = {
	.timer = TIM1,
	.time_base_init = &tim_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM1_CC_IRQn,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_clock_cfg tim_3_cfg = {
	.timer = TIM3,
	.time_base_init = &tim_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM3_IRQn,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};


static const struct pios_tim_clock_cfg tim_14_cfg = {
	.timer = TIM14,
	.time_base_init = &tim_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM14_IRQn,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};


// XXX need to confirm channel numbering on left vs board silk
// Servo pins:
// ch net    pin     timer
// 1  PWM0 - 18:PA8  TIM1CH1
// 2  PWM1 - 19:PA9  TIM1CH2
// 3  PWM2 - 20:PA10 TIM1CH3
// 4  PWM3 - 21:PA11 TIM1CH4
// 5  PWM4 - 12:PA6  TIM3CH1
// 6  PWM5 - 13:PA7  TIM3CH2

// PPM       15:PB1  TIM14CH1 (used) or TIM3CH4
static const struct pios_tim_channel pios_tim_rcvrport_pin = {
	.timer =  TIM14,
	.timer_chan = TIM_Channel_1,
	.remap = GPIO_AF_0,
	.pin = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_1,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_PuPd  = GPIO_PuPd_UP,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_Speed = GPIO_Speed_2MHz,
		},
		.pin_source = GPIO_PinSource1,
	},
};

static const struct pios_tim_channel pios_tim_servoport_pins[] = {
	{
		.timer = TIM3,
		.timer_chan = TIM_Channel_2,
		.remap = GPIO_AF_1,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_7,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_PuPd  = GPIO_PuPd_UP,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_Speed = GPIO_Speed_50MHz,
			},
			.pin_source = GPIO_PinSource7,
		},
	},
	{
		.timer = TIM3,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_1,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_6,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_PuPd  = GPIO_PuPd_UP,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_Speed = GPIO_Speed_50MHz,
			},
			.pin_source = GPIO_PinSource6,
		},
	},
	{
		.timer = TIM1,
		.timer_chan = TIM_Channel_4,
		.remap = GPIO_AF_2,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_11,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_PuPd  = GPIO_PuPd_UP,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_Speed = GPIO_Speed_50MHz,
			},
			.pin_source = GPIO_PinSource11,
		},
	},
	{
		.timer = TIM1,
		.timer_chan = TIM_Channel_3,
		.remap = GPIO_AF_2,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_10,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_PuPd  = GPIO_PuPd_UP,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_Speed = GPIO_Speed_50MHz,
			},
			.pin_source = GPIO_PinSource10,
		},
	},
	{
		.timer = TIM1,
		.timer_chan = TIM_Channel_2,
		.remap = GPIO_AF_2,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_9,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_PuPd  = GPIO_PuPd_UP,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_Speed = GPIO_Speed_50MHz,
			},
			.pin_source = GPIO_PinSource9,
		},
	},
	{
		.timer = TIM1,
		.timer_chan = TIM_Channel_1,
		.remap = GPIO_AF_2,
		.pin = {
			.gpio = GPIOA,
			.init = {
				.GPIO_Pin   = GPIO_Pin_8,
				.GPIO_Mode  = GPIO_Mode_AF,
				.GPIO_PuPd  = GPIO_PuPd_UP,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_Speed = GPIO_Speed_50MHz,
			},
			.pin_source = GPIO_PinSource8,
		},
	},
};

#if defined(PIOS_INCLUDE_USART)

#include "pios_usart_priv.h"

static const struct pios_usart_cfg pios_usart_rcvr_cfg = {
	.regs  = USART1,
	.irq = {
		.init    = {
			.NVIC_IRQChannel                   = USART1_IRQn,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
	.rx   = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_7,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_PuPd  = GPIO_PuPd_UP,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_Speed = GPIO_Speed_50MHz,
		},
	},
	.tx   = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_6,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_PuPd  = GPIO_PuPd_UP,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_Speed = GPIO_Speed_50MHz,
		},
	},
};

#if defined(PIOS_INCLUDE_DSM)
#include <pios_dsm_priv.h>

static const struct pios_dsm_cfg pios_dsm_rcvr_cfg = {
	.bind = {
		.gpio = GPIOB,
		.init = {
			.GPIO_Pin   = GPIO_Pin_7,
			.GPIO_Speed = GPIO_Speed_50MHz,
			.GPIO_PuPd  = GPIO_PuPd_UP,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_Speed = GPIO_Speed_50MHz,
		},
	},
};
#endif  /* PIOS_INCLUDE_DSM */

#endif  /* PIOS_INCLUDE_USART */

#if defined(PIOS_INCLUDE_COM)

#include "pios_com_priv.h"

#endif	/* PIOS_INCLUDE_COM */

#if defined(PIOS_INCLUDE_RTC)
// XXX XXX RTC.
/*
 * Realtime Clock (RTC)
 */
#include <pios_rtc_priv.h>

void PIOS_RTC_IRQ_Handler (void);
void RTC_WKUP_IRQHandler() __attribute__ ((alias ("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
	.clksrc = RCC_RTCCLKSource_HSE_Div32,
	.prescaler = 25 - 1, // 8MHz / 32 / 16 / 25 == 625Hz
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = RTC_IRQn,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

void PIOS_RTC_IRQ_Handler (void)
{
	PIOS_RTC_irq_handler ();
}

#endif

#if defined(PIOS_INCLUDE_SERVO) && defined(PIOS_INCLUDE_TIM)
/*
 * Servo outputs
 */
#include <pios_servo_priv.h>

const struct pios_servo_cfg pios_servo_cfg = {
	.tim_oc_init = {
		.TIM_OCMode = TIM_OCMode_PWM1,
		.TIM_OutputState = TIM_OutputState_Enable,
		.TIM_OutputNState = TIM_OutputNState_Disable,
		.TIM_Pulse = PIOS_SERVOS_INITIAL_POSITION,
		.TIM_OCPolarity = TIM_OCPolarity_High,
		.TIM_OCNPolarity = TIM_OCPolarity_High,
		.TIM_OCIdleState = TIM_OCIdleState_Reset,
		.TIM_OCNIdleState = TIM_OCNIdleState_Reset,
	},
	.channels = pios_tim_servoport_pins,
	.num_channels = NELEMENTS(pios_tim_servoport_pins),
};

#endif	/* PIOS_INCLUDE_SERVO && TIM */

/*
 * PPM Inputs
 */
#if defined(PIOS_INCLUDE_PPM)
#include <pios_ppm_priv.h>

const struct pios_ppm_cfg pios_ppm_cfg = {
	.tim_ic_init = {
		.TIM_ICPolarity = TIM_ICPolarity_Rising,
		.TIM_ICSelection = TIM_ICSelection_DirectTI,
		.TIM_ICPrescaler = TIM_ICPSC_DIV1,
		.TIM_ICFilter = 0x0,
	},
	/* Use only the first channel for ppm */
	.channels = &pios_tim_rcvrport_pin,
	.num_channels = 1,
};

#endif	/* PIOS_INCLUDE_PPM */

#if defined(PIOS_INCLUDE_RCVR)
#include "pios_rcvr_priv.h"
#endif /* PIOS_INCLUDE_RCVR */

const struct pios_sbus_cfg *get_sbus_cfg(enum board_revision board_rev)
{
	(void)board_rev;
	return NULL;
}

/**
 * @}
 * @}
 */
