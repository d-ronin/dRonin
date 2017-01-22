/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_VIDEO Code for OSD video generator
 * @brief Output video (black & white pixels) over SPI
 * @{
 *
 * @file       pios_video.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010-2014.
 * @brief      OSD gen module, handles OSD draw. Parts from CL-OSD and SUPEROSD projects
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************
 */
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

#ifndef PIOS_VIDEO_H
#define PIOS_VIDEO_H

#include <pios_stm32.h>
#include <pios_spi_priv.h>

// PAL/NTSC specific boundary values
struct pios_video_type_boundary {
	uint16_t graphics_right;
	uint16_t graphics_bottom;
};

// 3D Mode
enum pios_video_3d_mode {
	PIOS_VIDEO_3D_DISABLED,
	PIOS_VIDEO_3D_SBS3D,
};

enum pios_video_system {
	PIOS_VIDEO_SYSTEM_NONE,
	PIOS_VIDEO_SYSTEM_PAL,
	PIOS_VIDEO_SYSTEM_NTSC,
};


#if defined(PIOS_INCLUDE_VIDEO_QUADSPI)
#include <stm32f4xx_qspi.h>

// PAL/NTSC specific config values
struct pios_video_type_cfg {
	uint16_t graphics_height_real;
	uint16_t graphics_column_start;
	uint8_t  graphics_line_start;
	uint8_t  dma_buffer_length;
};

struct pios_video_cfg {
	QSPI_InitTypeDef qspi_init;
	DMA_TypeDef *pixel_dma;
	struct stm32_dma dma;
	struct stm32_gpio sclk;
	struct stm32_gpio bk1_io0;
	struct stm32_gpio bk1_io1;
	struct stm32_gpio bk1_io2;
	struct stm32_gpio bk1_io3;
	const struct pios_exti_cfg *hsync;
	const struct pios_exti_cfg *vsync;
	void (* set_bw_levels)(uint8_t, uint8_t);
	void (* set_x_offset)(int8_t);
	void (* set_x_scale)(uint8_t);
	void (* set_3d_config)(enum pios_video_3d_mode, uint8_t);
};
#else
// PAL/NTSC specific config values
struct pios_video_type_cfg {
	uint16_t graphics_height_real;
	uint16_t graphics_column_start;
	uint8_t  graphics_line_start;
	uint8_t  dma_buffer_length;
	uint8_t  period;
	uint8_t  dc;
};

struct pios_video_cfg {
	DMA_TypeDef *mask_dma;
	const struct pios_spi_cfg mask;
	DMA_TypeDef *level_dma;
	const struct pios_spi_cfg  level;
	struct pios_tim_channel    hsync_capture;
	struct pios_tim_channel    pixel_timer;
	TIM_TypeDef *              line_counter;
	TIM_OCInitTypeDef tim_oc_init;
	const struct pios_exti_cfg *vsync;
	void (* set_bw_levels)(uint8_t, uint8_t);
	void (* set_x_scale)(uint8_t);
	void (* set_3d_config)(enum pios_video_3d_mode, uint8_t);
};
#endif /* defined(PIOS_VIDEO_QUADSPI) */

extern const struct pios_video_cfg pios_video_cfg;

extern bool PIOS_Vsync_ISR();
extern bool PIOS_Hsync_ISR();

extern void PIOS_Video_Init(const struct pios_video_cfg *cfg);
extern void PIOS_Pixel_Init(void);
extern void PIOS_Video_SetLevels(uint8_t, uint8_t);
extern void PIOS_Video_SetXOffset(int8_t);
extern void PIOS_Video_SetYOffset(int8_t);
extern void PIOS_Video_SetXScale(uint8_t x_scale);
extern void PIOS_Video_Set3DConfig(enum pios_video_3d_mode mode, uint8_t right_eye_x_shift);

uint16_t PIOS_Video_GetLines(void);
enum pios_video_system PIOS_Video_GetSystem(void);

// video boundary values
extern const struct pios_video_type_boundary *pios_video_type_boundary_act;
#define GRAPHICS_LEFT        0
#define GRAPHICS_TOP         0
#define GRAPHICS_RIGHT       pios_video_type_boundary_act->graphics_right
#define GRAPHICS_BOTTOM      pios_video_type_boundary_act->graphics_bottom

#define GRAPHICS_X_MIDDLE	((GRAPHICS_RIGHT + 1) / 2)
#define GRAPHICS_Y_MIDDLE	((GRAPHICS_BOTTOM + 1) / 2)

// for autodetect
#define VIDEO_TYPE_PAL_ROWS  300

// draw area buffer values, for memory allocation, access and calculations we suppose the larger values for PAL, this also works for NTSC
#define GRAPHICS_WIDTH_REAL  376                            // max columns
#define GRAPHICS_HEIGHT_REAL 266                            // max lines
#if defined(PIOS_VIDEO_SPLITBUFFER)
#define BUFFER_WIDTH         (GRAPHICS_WIDTH_REAL / 8  + 1)  // Bytes plus one byte for SPI, needs to be multiple of 4 for alignment
#define BUFFER_HEIGHT        (GRAPHICS_HEIGHT_REAL)
#else
#define BUFFER_WIDTH_TMP     (GRAPHICS_WIDTH_REAL / (8 / PIOS_VIDEO_BITS_PER_PIXEL))
#define BUFFER_WIDTH (BUFFER_WIDTH_TMP + BUFFER_WIDTH_TMP % 4)
#define BUFFER_HEIGHT        (GRAPHICS_HEIGHT_REAL)
#endif /* PIOS_INCLUDE_VIDEO_SPLITBUFFER */

// Macro to swap buffers given a temporary pointer.
#define SWAP_BUFFS(tmp, a, b) { tmp = a; a = b; b = tmp; }

#endif /* PIOS_VIDEO_H */
