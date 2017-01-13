/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 *
 * @file       pios.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      Main PiOS header to include all the compiled in PiOS options
 *
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#ifndef PIOS_H
#define PIOS_H

#ifdef SIM_POSIX
#include <pios_posix.h>
#endif

/* PIOS Feature Selection */
#include "pios_config.h"

#if defined(PIOS_INCLUDE_CHIBIOS)
/* @note    This is required because of difference in chip define between ChibiOS and ST libs.
 *          It is also used to force inclusion of chibios_transition defines. */
#include "hal.h"
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

/* C Lib Includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#ifndef SIM_POSIX

/* STM32 Std Periph Lib */
#if defined(STM32F4XX)
# include <stm32f4xx.h>
# include <stm32f4xx_rcc.h>
#elif defined(STM32F30X)
#include <stm32f30x.h>
#include <stm32f30x_rcc.h>
#elif defined(STM32F2XX)
#include <stm32f2xx.h>
#include <stm32f2xx_syscfg.h>
#elif defined(STM32F0XX)
#include <stm32f0xx.h>
#else
#include <stm32f10x.h>
#endif

#endif //SIM_POSIX

/* PIOS Board Specific Device Configuration */
#include "pios_board.h"

/* Generic initcall infrastructure */
#if defined(PIOS_INCLUDE_INITCALL)
#include "pios_initcall.h"
#endif

/* PIOS Hardware Includes */
#include <pios_sys.h>
#include <pios_delay.h>
#include <pios_annunc.h>
#include <pios_irq.h>
#include <pios_adc.h>
#include <pios_internal_adc.h>
#include <pios_servo.h>
#include <pios_rtc.h>
#include <pios_i2c.h>
#include <pios_can.h>
#include <pios_spi.h>
#include <pios_ppm.h>
#include <pios_pwm.h>
#include <pios_rcvr.h>
#include <pios_reset.h>
#if defined(PIOS_INCLUDE_DMA_CB_SUBSCRIBING_FUNCTION)
#include <pios_dma.h>
#endif
#if defined(PIOS_INCLUDE_FREERTOS) || defined(PIOS_INCLUDE_CHIBIOS)
#include <pios_sensors.h>
#endif
#include <pios_wdg.h>

#ifndef SIM_POSIX
#include <pios_gpio.h>
#include <pios_exti.h>
#include <pios_usart.h>
#include <pios_srxl.h>
#endif  // SIM_POSIX

/* PIOS Hardware Includes (Common) */
#include <pios_debug.h>
#include <pios_heap.h>
#include <pios_com.h>
#if defined(PIOS_INCLUDE_MPXV7002)
#include <pios_mpxv7002.h>
#endif
#if defined(PIOS_INCLUDE_MPXV5004)
#include <pios_mpxv5004.h>
#endif
#if defined(PIOS_INCLUDE_ETASV3)
#include <pios_etasv3.h>
#endif
#if defined(PIOS_INCLUDE_BMP085)
#include <pios_bmp085.h>
#endif
#if defined(PIOS_INCLUDE_BMP280)
#include <pios_bmp280.h>
#endif
#if defined(PIOS_INCLUDE_HCSR04)
#include <pios_hcsr04.h>
#endif
#if defined(PIOS_INCLUDE_HMC5843)
#include <pios_hmc5843.h>
#endif
#if defined(PIOS_INCLUDE_HMC5983)
#include <pios_hmc5983.h>
#endif
#if defined(PIOS_INCLUDE_I2C_ESC)
#include <pios_i2c_esc.h>
#endif
#if defined(PIOS_INCLUDE_IMU3000)
#include <pios_imu3000.h>
#endif
#if defined(PIOS_INCLUDE_MPU6050)
#include <pios_mpu6050.h>
#endif
#if defined(PIOS_INCLUDE_MPU9150)
#include <pios_mpu9150.h>
#endif
#if defined(PIOS_INCLUDE_MPU9250_BRAIN)
#include <pios_mpu9250_brain.h>
#endif
#if defined(PIOS_INCLUDE_MPU6000)
#include <pios_mpu6000.h>
#endif
#if defined(PIOS_INCLUDE_MPU)
#include <pios_mpu.h>
#endif
#if defined(PIOS_INCLUDE_L3GD20)
#include <pios_l3gd20.h>
#endif
#if defined(PIOS_INCLUDE_LSM303)
#include <pios_lsm303.h>
#endif
#if defined(PIOS_INCLUDE_MS5611)
#include <pios_ms5611.h>
#endif
#if defined(PIOS_INCLUDE_MS5611_SPI)
#include <pios_ms5611_spi.h>
#endif
#if defined(PIOS_INCLUDE_IAP)
#include <pios_iap.h>
#endif
#if defined(PIOS_INCLUDE_VIDEO)
#include <pios_video.h>
#endif

#if defined(PIOS_INCLUDE_FLASH)
#include <pios_flash.h>
#include <pios_flashfs.h>
#endif

#if defined(PIOS_INCLUDE_BL_HELPER)
#include <pios_bl_helper.h>
#endif

#if defined(PIOS_INCLUDE_USB)
#include <pios_usb.h>
#endif

#if defined(PIOS_INCLUDE_RFM22B)
#include <pios_rfm22b.h>
#ifdef PIOS_INCLUDE_RFM22B_COM
#include <pios_rfm22b_com.h>
#endif
#endif

#if defined(PIOS_INCLUDE_IBUS)
#include <pios_ibus.h>
#endif

#if defined(PIOS_INCLUDE_MAX7456)
#include <pios_max7456.h>
#endif

#include <pios_modules.h>

#include <pios_crc.h>

#define NELEMENTS(x) (sizeof(x) / sizeof(*(x)))
#define DONT_BUILD_IF(COND,MSG) typedef char static_assertion_##MSG[(COND)?-1:1]

#endif /* PIOS_H */

/**
 * @}
 */
