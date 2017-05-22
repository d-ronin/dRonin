#ifndef PIOS_QUICKDMA_H
#define PIOS_QUICKDMA_H

#include <pios.h>

typedef struct quickdma_transfer* quickdma_transfer_t;

#define QUICKDMA_SIZE_BYTE				1
#define QUICKDMA_SIZE_HALFWORD			2
#define QUICKDMA_SIZE_WORD				4

#define QUICKDMA_PRIORITY_LOW			0
#define QUICKDMA_PRIORITY_MEDIUM		1
#define QUICKDMA_PRIORITY_HIGH			2
#define QUICKDMA_PRIORITY_VERYHIGH		3

#if defined(STM32F4XX)

#include "stm32f4xx.h"

struct quickdma_config {
	DMA_Stream_TypeDef *stream;
	uint32_t channel;

#elif defined(STM32F30X)
#error MCU not supported.

#include "stm32f30x.h"

struct quickdma_config {
	DMA_Channel_TypeDef *channel;

#else
#error MCU not supported.
#endif

	/* Common settings */
	uint8_t fifo;
};

quickdma_transfer_t quickdma_initialize(const struct quickdma_config *cfg);

void quickdma_mem_to_peripheral(quickdma_transfer_t tr, uint32_t memaddr, uint32_t phaddr, uint16_t len, uint8_t datasize);
void quickdma_peripheral_to_mem(quickdma_transfer_t tr, uint32_t phaddr, uint32_t memaddr, uint16_t len, uint8_t datasize);

void quickdma_set_meminc(quickdma_transfer_t tr, bool enabled);

void quickdma_set_priority(quickdma_transfer_t tr, uint8_t priority);

bool quickdma_start_transfer(quickdma_transfer_t tr);
void quickdma_wait_for_transfer(quickdma_transfer_t tr);
bool quickdma_is_transferring(quickdma_transfer_t tr);
void quickdma_stop_transfer(quickdma_transfer_t tr);

#endif // PIOS_QUICKDMA_H
