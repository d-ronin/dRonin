/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_VIDEO Code for OSD video generator
 * @brief Output video (black & white pixels) over SPI
 * @{
 *
 * @file       pios_video.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010-2014.
 * @brief      OSD gen module, handles OSD draw. Parts from CL-OSD and SUPEROSD projects
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

#include "pios_config.h"

#if defined(PIOS_INCLUDE_VIDEO_QUADSPI)

#if !defined(PIOS_VIDEO_QUADSPI_Y_OFFSET)
#define PIOS_VIDEO_QUADSPI_Y_OFFSET 0
#endif /* !defined(PIOS_VIDEO_QUADSPI_Y_OFFSET) */

#if defined(PIOS_INCLUDE_FREERTOS)
#include "FreeRTOS.h"
#endif /* defined(PIOS_INCLUDE_FREERTOS) */

#include "pios.h"
#include "pios_video.h"
#include "pios_semaphore.h"

// How many frames until we redraw
#define VSYNC_REDRAW_CNT 2

extern struct pios_semaphore * onScreenDisplaySemaphore;

#define GRPAHICS_RIGHT_NTSC 351
#define GRPAHICS_RIGHT_PAL  359

static const struct pios_video_type_boundary pios_video_type_boundary_ntsc = {
	.graphics_right  = GRPAHICS_RIGHT_NTSC, // must be: graphics_width_real - 1
	.graphics_bottom = 239,                 // must be: graphics_hight_real - 1
};

static const struct pios_video_type_boundary pios_video_type_boundary_pal = {
	.graphics_right  = GRPAHICS_RIGHT_PAL, // must be: graphics_width_real - 1
	.graphics_bottom = 265,                // must be: graphics_hight_real - 1
};

#define NTSC_BYTES (GRPAHICS_RIGHT_NTSC / (8 / PIOS_VIDEO_BITS_PER_PIXEL) + 1)
#define PAL_BYTES (GRPAHICS_RIGHT_PAL / (8 / PIOS_VIDEO_BITS_PER_PIXEL) + 1)

static const struct pios_video_type_cfg pios_video_type_cfg_ntsc = {
	.graphics_hight_real   = 240,   // Real visible lines
	.graphics_column_start = 260,   // First visible OSD column (after Hsync)
	.graphics_line_start   = 16,    // First visible OSD line
	.dma_buffer_length     = NTSC_BYTES + NTSC_BYTES % 4, // DMA buffer length in bytes (has to be multiple of 4)
};

static const struct pios_video_type_cfg pios_video_type_cfg_pal = {
	.graphics_hight_real   = 266,   // Real visible lines
	.graphics_column_start = 420,   // First visible OSD column (after Hsync)
	.graphics_line_start   = 28,    // First visible OSD line
	.dma_buffer_length     = PAL_BYTES + PAL_BYTES % 4, // DMA buffer length in bytes (has to be multiple of 4)
};

// Allocate buffers.
// Must be allocated in one block, so it is in a struct.
struct _buffers {
	uint8_t buffer0[BUFFER_HEIGHT * BUFFER_WIDTH];
	uint8_t buffer1[BUFFER_HEIGHT * BUFFER_WIDTH];
} buffers;

// Remove the struct definition (makes it easier to write for).
#define buffer0 (buffers.buffer0)
#define buffer1 (buffers.buffer1)

// Pointers to each of these buffers.
uint8_t *draw_buffer;
uint8_t *disp_buffer;


const struct pios_video_type_boundary *pios_video_type_boundary_act = &pios_video_type_boundary_pal;

int8_t video_type_act = VIDEO_TYPE_NONE;

// Private variables
static int16_t active_line = 0;
static uint32_t buffer_offset;
static int8_t y_offset = 0;
static const struct pios_video_cfg *dev_cfg = NULL;
static uint16_t num_video_lines = 0;
static int8_t video_type_tmp = VIDEO_TYPE_PAL;
static const struct pios_video_type_cfg *pios_video_type_cfg_act = &pios_video_type_cfg_pal;

// Private functions
static void swap_buffers();

/**
 * @brief Vsync interrupt service routine
 */
bool PIOS_Vsync_ISR()
{
	static bool woken = false;
	static uint16_t Vsync_update = 0;

	// discard spurious vsync pulses (due to improper grounding), so we don't overload the CPU
	if (active_line < pios_video_type_cfg_ntsc.graphics_hight_real - 10) {
		return false;
	}

	// Update the number of video lines
	num_video_lines = active_line + pios_video_type_cfg_act->graphics_line_start + y_offset;

	// check video type
	if (num_video_lines > VIDEO_TYPE_PAL_ROWS) {
		video_type_tmp = VIDEO_TYPE_PAL;
	}

	// if video type has changed set new active values
	if (video_type_act != video_type_tmp) {
		video_type_act = video_type_tmp;
		if (video_type_act == VIDEO_TYPE_NTSC) {
			pios_video_type_boundary_act = &pios_video_type_boundary_ntsc;
			pios_video_type_cfg_act = &pios_video_type_cfg_ntsc;
		} else {
			pios_video_type_boundary_act = &pios_video_type_boundary_pal;
			pios_video_type_cfg_act = &pios_video_type_cfg_pal;
		}
	}

	video_type_tmp = VIDEO_TYPE_NTSC;

	// Every VSYNC_REDRAW_CNT field: swap buffers and trigger redraw
	if (++Vsync_update >= VSYNC_REDRAW_CNT) {
		Vsync_update = 0;
		swap_buffers();
		PIOS_Semaphore_Give_FromISR(onScreenDisplaySemaphore, &woken);
	}

	// Get ready for the first line. We will start outputting data at line zero.
	active_line = 0 - (pios_video_type_cfg_act->graphics_line_start + y_offset);
	buffer_offset = 0;

#if defined(PIOS_INCLUDE_FREERTOS)
	/* Yield From ISR if needed */
	portEND_SWITCHING_ISR(woken == true ? pdTRUE : pdFALSE);
#endif
	return woken;
}

bool PIOS_Hsync_ISR()
{
	static bool woken = false;

	active_line++;

	if ((active_line >= 0) && (active_line < pios_video_type_cfg_act->graphics_hight_real)) {
		// Check if QUADSPI is busy
		if (QUADSPI->SR & 0x20)
			goto exit;

		// Disable DMA
		dev_cfg->dma.tx.channel->CR &= ~(uint32_t)DMA_SxCR_EN;

		// Clear the DMA interrupt flags
		dev_cfg->pixel_dma->HIFCR  |= DMA_FLAG_TCIF7 | DMA_FLAG_HTIF7 | DMA_FLAG_FEIF7 | DMA_FLAG_TEIF7 | DMA_FLAG_DMEIF7;

		// Load new line
		dev_cfg->dma.tx.channel->M0AR = (uint32_t)&disp_buffer[buffer_offset];

		// Set length
		dev_cfg->dma.tx.channel->NDTR = (uint16_t)pios_video_type_cfg_act->dma_buffer_length;
		QUADSPI->DLR = (uint32_t)pios_video_type_cfg_act->dma_buffer_length - 1;

		// Enable DMA
		dev_cfg->dma.tx.channel->CR |= (uint32_t)DMA_SxCR_EN;

		buffer_offset += BUFFER_WIDTH;
	}

exit:
#if defined(PIOS_INCLUDE_FREERTOS)
	/* Yield From ISR if needed */
	portEND_SWITCHING_ISR(woken == true ? pdTRUE : pdFALSE);
#endif
	return woken;
}

/**
 * swap_buffers: Swaps the two buffers. Contents in the display
 * buffer is seen on the output and the display buffer becomes
 * the new draw buffer.
 */
static void swap_buffers()
{
	// While we could use XOR swap this is more reliable and
	// dependable and it's only called a few times per second.
	// Many compilers should optimize these to EXCH instructions.
	uint8_t *tmp;

	SWAP_BUFFS(tmp, disp_buffer, draw_buffer);
}

/**
 * Init
 */
void PIOS_Video_Init(const struct pios_video_cfg *cfg)
{
	dev_cfg = cfg; // store config before enabling interrupt

	/* Enable QUADSPI clock */
	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_QSPI, ENABLE);

	/* Map pins to QUADSPI */
	GPIO_PinAFConfig(cfg->sclk.gpio, __builtin_ctz(cfg->sclk.init.GPIO_Pin), GPIO_AF9_QUADSPI);
	GPIO_PinAFConfig(cfg->bk1_io0.gpio, __builtin_ctz(cfg->bk1_io0.init.GPIO_Pin), GPIO_AF9_QUADSPI);
	GPIO_PinAFConfig(cfg->bk1_io1.gpio, __builtin_ctz(cfg->bk1_io1.init.GPIO_Pin), GPIO_AF9_QUADSPI);

	GPIO_Init(cfg->sclk.gpio, (GPIO_InitTypeDef *)&(cfg->sclk.init));
	GPIO_Init(cfg->bk1_io0.gpio, (GPIO_InitTypeDef *)&(cfg->bk1_io0.init));
	GPIO_Init(cfg->bk1_io1.gpio, (GPIO_InitTypeDef *)&(cfg->bk1_io1.init));

	/* Configure QUADSPI */
	QSPI_Init(&cfg->qspi_init);

	QSPI_ComConfig_InitTypeDef qspi_com_config;
	QSPI_ComConfig_StructInit(&qspi_com_config);

	qspi_com_config.QSPI_ComConfig_FMode       = QSPI_ComConfig_FMode_Indirect_Write;
	qspi_com_config.QSPI_ComConfig_DDRMode     = QSPI_ComConfig_DDRMode_Disable;
	qspi_com_config.QSPI_ComConfig_DHHC        = QSPI_ComConfig_DHHC_Disable;
	qspi_com_config.QSPI_ComConfig_SIOOMode    = QSPI_ComConfig_SIOOMode_Disable;
	qspi_com_config.QSPI_ComConfig_DMode       = QSPI_ComConfig_DMode_2Line;
	qspi_com_config.QSPI_ComConfig_DummyCycles = 0;
	qspi_com_config.QSPI_ComConfig_ABMode      = QSPI_ComConfig_ABMode_NoAlternateByte;
	qspi_com_config.QSPI_ComConfig_ADMode      = QSPI_ComConfig_ADMode_NoAddress;
	qspi_com_config.QSPI_ComConfig_IMode       = QSPI_ComConfig_IMode_NoInstruction;
	QSPI_ComConfig_Init(&qspi_com_config);

	QSPI_SetFIFOThreshold(3);

	/* Configure DMA */
	DMA_Init(cfg->dma.tx.channel, (DMA_InitTypeDef *)&(cfg->dma.tx.init));

	/* Configure and clear buffers */
	draw_buffer = buffer0;
	disp_buffer = buffer1;

	/* Enable TC interrupt */
	QSPI_ITConfig(QSPI_IT_TC, ENABLE);
	QSPI_ITConfig(QSPI_IT_FT, ENABLE);

	/* Enable DMA */
	QSPI_DMACmd(ENABLE);

	// Enable the QUADSPI
	QSPI_Cmd(ENABLE);

	// Enable interrupts
	PIOS_EXTI_Init(cfg->vsync);
	PIOS_EXTI_Init(cfg->hsync);
}

/**
 *
 */
uint16_t PIOS_Video_GetLines(void)
{
	return num_video_lines;
}

/**
 *
 */
uint16_t PIOS_Video_GetType(void)
{
	return video_type_act;
}

/**
*  Set the black and white levels
*/
void PIOS_Video_SetLevels(uint8_t black, uint8_t white)
{
	if (dev_cfg->set_bw_levels) {
		dev_cfg->set_bw_levels(black, white);
	}
}

/**
*  Set the offset in x direction
*/
void PIOS_Video_SetXOffset(int8_t x_offset_in)
{
	if (x_offset_in > 20)
		x_offset_in = 20;
	if (x_offset_in < -20)
		x_offset_in = -20;

	if (dev_cfg->set_x_offset) {
		dev_cfg->set_x_offset(x_offset_in);
	}
}

/**
*  Set the offset in y direction
*/
void PIOS_Video_SetYOffset(int8_t y_offset_in)
{
	if (y_offset_in > 20)
		y_offset_in = 20;
	if (y_offset_in < -20)
		y_offset_in = -20;
	y_offset = y_offset_in + PIOS_VIDEO_QUADSPI_Y_OFFSET;
}

/**
*  Set the x scale
*/
void PIOS_Video_SetXScale(uint8_t x_scale)
{
	if (dev_cfg->set_x_scale) {
		dev_cfg->set_x_scale(x_scale);
	}
}

/**
*  Set the 3D mode configuration
*/
void PIOS_Video_Set3DConfig(enum pios_video_3d_mode mode, uint8_t right_eye_x_shift)
{
	if (dev_cfg->set_3d_config){
		dev_cfg->set_3d_config(mode, right_eye_x_shift);
	}
}
#endif /* PIOS_INCLUDE_VIDEO_SINGLESPI */
