#include <pios.h>
#include <pios_thread.h>

#include "pios_semaphore.h"
#include "pios_quickdma.h"

struct quickdma_transfer {

	/* Original configuration, to not duplicate static stuff like stream and channel stuff. */
	const struct quickdma_config *c;

	/* ISR and IFCR flags are all the same on F4. Use one set for both checking
	   interrupt status and clearing them. */
	uint32_t tcflag;						/* Transfer complete flag. */

	uint32_t memaddr;
	uint32_t phaddr;
	uint16_t transfer_length;

	struct pios_semaphore *irq_sema;		/* Semaphore for yielding. */
	/*
		To maintain a linked list of yielding enabled DMA streams/channels.
	 */
	struct quickdma_transfer *next;
};

/* Use a sufficienty large timeout, because if there's multiple SPI slaves, and
   RTOS/PIOS jitter makes the semaphore timeout, everything goes sideways.
   Seems especially dramatic on some targets, notably Seppuku. */
#define IRQ_SEMA_TIMEOUT		5

#define HIFCR_FLAG				0x80000000	/* Bit to store whether a flag belongs to LIFCR or HIFCR. */

/* Root of linked-list of semaphore enabled transfers. */
quickdma_transfer_t root;

/**
 * @brief Generates interrupt flag clearing masks for (de)initialization.
 */
uint32_t quickdma_ifcflag_mask(DMA_Stream_TypeDef *stream)
{
	/* Ugly as hell.
	   Probably should do the sketchy bitshifting stuff like in StdPeriph. */
	switch ((uint32_t)stream) {
	case (uint32_t)DMA1_Stream0:
	case (uint32_t)DMA2_Stream0:
		return DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;

	case (uint32_t)DMA1_Stream1:
	case (uint32_t)DMA2_Stream1:
		return DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;

	case (uint32_t)DMA1_Stream2:
	case (uint32_t)DMA2_Stream2:
		return DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTEIF2 | DMA_LIFCR_CDMEIF2 | DMA_LIFCR_CFEIF2;

	case (uint32_t)DMA1_Stream3:
	case (uint32_t)DMA2_Stream3:
		return DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CFEIF3;

	case (uint32_t)DMA1_Stream4:
	case (uint32_t)DMA2_Stream4:
		return DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4;

	case (uint32_t)DMA1_Stream5:
	case (uint32_t)DMA2_Stream5:
		return DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5;

	case (uint32_t)DMA1_Stream6:
	case (uint32_t)DMA2_Stream6:
		return DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;

	case (uint32_t)DMA1_Stream7:
	case (uint32_t)DMA2_Stream7:
		return DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7 | DMA_HIFCR_CFEIF7;

	default:
		// Whoops. Don't.
		PIOS_Assert(0);
	}
}

/**
 * @brief Returns the transfer complete flag for a stream.
 */
uint32_t quickdma_tcflag_map(DMA_Stream_TypeDef *stream)
{
	/* Ugly as sin. */
	switch ((uint32_t)stream) {
	case (uint32_t)DMA1_Stream0:
	case (uint32_t)DMA2_Stream0:
		return DMA_LIFCR_CTCIF0;
	case (uint32_t)DMA1_Stream1:
	case (uint32_t)DMA2_Stream1:
		return DMA_LIFCR_CTCIF1;
	case (uint32_t)DMA1_Stream2:
	case (uint32_t)DMA2_Stream2:
		return DMA_LIFCR_CTCIF2;
	case (uint32_t)DMA1_Stream3:
	case (uint32_t)DMA2_Stream3:
		return DMA_LIFCR_CTCIF3;
	case (uint32_t)DMA1_Stream4:
	case (uint32_t)DMA2_Stream4:
		return DMA_HIFCR_CTCIF4 | HIFCR_FLAG;
	case (uint32_t)DMA1_Stream5:
	case (uint32_t)DMA2_Stream5:
		return DMA_HIFCR_CTCIF5 | HIFCR_FLAG;
	case (uint32_t)DMA1_Stream6:
	case (uint32_t)DMA2_Stream6:
		return DMA_HIFCR_CTCIF6 | HIFCR_FLAG;
	case (uint32_t)DMA1_Stream7:
	case (uint32_t)DMA2_Stream7:
		return DMA_HIFCR_CTCIF7 | HIFCR_FLAG;
	default:
		// Whoops. Don't.
		PIOS_Assert(0);
	}
}

/**
 * @brief Maps quickdma memory size parameter to a DMA flag.
 */
inline uint32_t quickdma_trsize_mem(uint8_t x)
{
	switch (x) {
	case QUICKDMA_SIZE_BYTE:
		return 0;
	case QUICKDMA_SIZE_HALFWORD:
		return DMA_SxCR_MSIZE_0;
	case QUICKDMA_SIZE_WORD:
		return DMA_SxCR_MSIZE_1;
	default:
		PIOS_Assert(0);
	}
}

/**
 * @brief Maps quickdma peripheral size parameter to a DMA flag.
 */
inline uint32_t quickdma_trsize_ph(uint8_t x)
{
	switch (x) {
	case QUICKDMA_SIZE_BYTE:
		return 0;
	case QUICKDMA_SIZE_HALFWORD:
		return DMA_SxCR_PSIZE_0;
	case QUICKDMA_SIZE_WORD:
		return DMA_SxCR_PSIZE_1;
	default:
		PIOS_Assert(0);
	}
}

/**
 * @brief Maps quickdma priority parameter to a DMA flag.
 */
inline uint32_t quickdma_priority(uint8_t x)
{
	switch (x) {
	case QUICKDMA_PRIORITY_LOW:
		return 0;
	case QUICKDMA_PRIORITY_MEDIUM:
		return DMA_SxCR_PL_0;
	case QUICKDMA_PRIORITY_HIGH:
		return DMA_SxCR_PL_1;
	case QUICKDMA_PRIORITY_VERYHIGH:
		return DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
	default:
		PIOS_Assert(0);
	}
}

/**
 * @brief Runs a deinitialization of a DMA stream.
 */
void quickdma_deinit(DMA_Stream_TypeDef *s)
{
	s->CR &= ~DMA_SxCR_EN;
	while (s->CR & DMA_SxCR_EN) ;

	s->CR = 0;
	s->NDTR = 0;
	s->PAR = 0;
	s->M0AR = 0;
	s->M1AR = 0,
	s->FCR = 0x00000021; /* Stdlib does that. */

	DMA_TypeDef *dma = s < DMA2_Stream0 ? DMA1 : DMA2;
	volatile uint32_t *IFCR = (quickdma_tcflag_map(s) & HIFCR_FLAG) ? &dma->HIFCR : &dma->LIFCR;

	/* Clear all flags. */
	*IFCR |= quickdma_ifcflag_mask(s);
}

/**
 * @brief Calculates NVIC interrupt priority from PIOS priority.
 */
uint8_t quickdma_calc_priority(uint8_t pios_prio)
{
	uint8_t prio = (uint8_t)((0x700 - ((SCB->AIRCR) & 0x700)) >> 0x8);
	uint8_t pre = 0x4 - prio;

	prio = pios_prio << pre;
	prio <<= 0x4;

	return prio;
}

/**
 * @brief		Initializes and configures a DMA stream/channel for transfers.
 *
 * @param[in] stream 			DMA stream in question.
 * @param[in] channel			DMA channel on the stream.
 * @param[in] fifo 				Whether to allow auto-FIFO configuration or not.
 *
 * @returns Configuration struct for further use.
 */
quickdma_transfer_t quickdma_initialize(const struct quickdma_config *cfg)
{
	quickdma_transfer_t tr = PIOS_malloc_no_dma(sizeof(*tr));
	PIOS_Assert(tr);
	memset(tr, 0, sizeof(*tr));

	tr->c = cfg;
	tr->tcflag = quickdma_tcflag_map(cfg->stream);

	/* Enable interrupts, if desired. */
	if (cfg->irq) {
		uint8_t irq = cfg->irq;

		NVIC->IP[irq] = quickdma_calc_priority(PIOS_IRQ_PRIO_HIGH);
		NVIC->ISER[irq >> 0x5] = 1 << (irq & 0x1f);
	}

	return tr;
}

/**
 * @brief		Sets up a DMA stream/channel to memory-to-peripheral direction.
 *
 * @param[in] tr 				DMA configuration.
 * @param[in] memaddr 			Address of source data.
 * @param[in] phaddr 			Address of peripheral to send to.
 * @param[in] len 				Length of the transfer.
 * @param[in] datasize 			Transfer chunk size of the data.
 */
void quickdma_mem_to_peripheral(quickdma_transfer_t tr, uint32_t memaddr, uint32_t phaddr, uint16_t len, uint8_t datasize)
{
	PIOS_Assert(tr);
	PIOS_Assert(tr->c);
	PIOS_Assert(tr->c->stream);

	DMA_Stream_TypeDef *s = tr->c->stream;

	s->CR &= ~DMA_SxCR_EN;
	while (s->CR & DMA_SxCR_EN) ;

	quickdma_deinit(s);

	s->CR = tr->c->channel |
	        DMA_SxCR_DIR_0 |					/* Mem to peripheral. */
	        DMA_SxCR_MINC |						/* Memory pointer increase. */
	        quickdma_trsize_ph(datasize) |		/* Periph. data size */
	        quickdma_trsize_mem(datasize);		/* Mem. data size */

	if (tr->c->fifo && (memaddr % 16) == 0) {
		/* TODO: Make more sophisticated, dealing with non-16 byte alignments. */
		/* Aligned to 16 bytes, do burst. */
		s->FCR |= DMA_SxFCR_FTH_0 | DMA_SxFCR_FTH_1 | DMA_SxFCR_DMDIS;	/* Full threshold */
		switch (datasize) {
		case QUICKDMA_SIZE_BYTE:
			s->CR |= DMA_SxCR_MBURST_0 | DMA_SxCR_MBURST_1;	/* INC16 */
			break;
		case QUICKDMA_SIZE_HALFWORD:
			s->CR |= DMA_SxCR_MBURST_1;						/* INC8 */
			break;
		case QUICKDMA_SIZE_WORD:
			s->CR |= DMA_SxCR_MBURST_0;						/* INC4 */
			break;
		/* Doesn't need default case, if the value is invalid, it'll crash
		   via quickdma_trsize_mem. */
		}
	} else {
		/* Clear FIFO flags. */
		s->FCR &= ~(DMA_SxFCR_FTH | DMA_SxFCR_DMDIS);
		s->CR &= ~DMA_SxCR_MBURST;
	}

	tr->memaddr = memaddr;
	tr->phaddr = phaddr;
	tr->transfer_length = len;
}

/**
 * @brief		Sets up a DMA stream/channel to peripheral-to-memory direction.
 *
 * @param[in] tr 				DMA configuration.
 * @param[in] phaddr 			Address of peripheral to send to.
 * @param[in] memaddr 			Address of source data.
 * @param[in] len 				Length of the transfer.
 * @param[in] datasize 			Transfer chunk size of the data.
 */
void quickdma_peripheral_to_mem(quickdma_transfer_t tr, uint32_t phaddr, uint32_t memaddr, uint16_t len, uint8_t datasize)
{
	PIOS_Assert(tr);
	PIOS_Assert(tr->c);
	PIOS_Assert(tr->c->stream);

	DMA_Stream_TypeDef *s = tr->c->stream;

	s->CR &= ~DMA_SxCR_EN;
	while (s->CR & DMA_SxCR_EN) ;

	quickdma_deinit(s);

	s->CR = tr->c->channel |
	        									/* Peripheral to mem. */
	        DMA_SxCR_MINC |						/* Memory pointer increase. */
	        quickdma_trsize_ph(datasize) |		/* Periph. data size */
	        quickdma_trsize_mem(datasize);		/* Mem. data size */

	/* Don't do FIFO in this direction yet. */
	s->FCR &= ~(DMA_SxFCR_FTH | DMA_SxFCR_DMDIS);
	s->CR &= ~DMA_SxCR_MBURST;

	tr->memaddr = memaddr;
	tr->phaddr = phaddr;
	tr->transfer_length = len;
}

/**
 * @brief		Sets the priority of the DMA transfer.
 *
 * @param[in] tr 				DMA configuration.
 * @param[in] priority 			The transfer priority, in case of contention.
 *
 * @note Default priority is low.
 */
void quickdma_set_priority(quickdma_transfer_t tr, uint8_t priority)
{
	PIOS_Assert(tr);
	PIOS_Assert(tr->c);
	PIOS_Assert(tr->c->stream);
	PIOS_Assert(priority <= QUICKDMA_PRIORITY_VERYHIGH);

	DMA_Stream_TypeDef *s = tr->c->stream;

	s->CR &= ~DMA_SxCR_PL;
	s->CR |= quickdma_priority(priority);
}

/**
 * @brief		Stops an ongoing transfer.
 *
 * @param[in] tr 				DMA configuration.
 */
void quickdma_stop_transfer(quickdma_transfer_t tr)
{
	PIOS_Assert(tr);
	PIOS_Assert(tr->c);
	PIOS_Assert(tr->c->stream);

	DMA_Stream_TypeDef *s = tr->c->stream;
	DMA_TypeDef *dma = s < DMA2_Stream0 ? DMA1 : DMA2;
	volatile uint32_t *IFCR = (tr->tcflag & HIFCR_FLAG) ? &dma->HIFCR : &dma->LIFCR;

	s->CR &= ~DMA_SxCR_EN;
	while (s->CR & DMA_SxCR_EN) ;

	*IFCR |= quickdma_ifcflag_mask(s);
}

/**
 * @brief		Starts a DMA transfer.
 *
 * @param[in] tr 				DMA configuration.
 *
 * @returns true if the DMA transfer started successfully, false if it didn't 
 *          (likely caused by an already ongoing transfer).
 */
bool quickdma_start_transfer(quickdma_transfer_t tr)
{
	PIOS_Assert(tr);
	PIOS_Assert(tr->c);
	PIOS_Assert(tr->c->stream);

	DMA_Stream_TypeDef *s = tr->c->stream;
	DMA_TypeDef *dma = s < DMA2_Stream0 ? DMA1 : DMA2;
	volatile uint32_t *IFCR = (tr->tcflag & HIFCR_FLAG) ? &dma->HIFCR : &dma->LIFCR;

	/* Wait for DMA to disable. */
	s->CR &= ~DMA_SxCR_EN;
	while (s->CR & DMA_SxCR_EN) ;

	s->M0AR = tr->memaddr;
	s->PAR = tr->phaddr;
	s->NDTR = tr->transfer_length;

	*IFCR |= quickdma_ifcflag_mask(s);

	if (tr->irq_sema) {
		PIOS_Semaphore_Take(tr->irq_sema, 0);
		s->CR |= DMA_SxCR_TCIE;
	}

	s->CR |= DMA_SxCR_EN;

	return true;
}

/**
 * @brief		Waits for a DMA transfer to finish.
 *
 * @param[in] tr 				DMA configuration.
 *
 * @returns False if the transfer timed out.
 */
void quickdma_wait_for_transfer(quickdma_transfer_t tr)
{
	PIOS_Assert(tr);
	PIOS_Assert(tr->c);
	PIOS_Assert(tr->c->stream);

	DMA_Stream_TypeDef *s = tr->c->stream;
	DMA_TypeDef *dma = s < DMA2_Stream0 ? DMA1 : DMA2;
	volatile uint32_t *ISR = (tr->tcflag & HIFCR_FLAG) ? &dma->HISR : &dma->LISR;

	if (*ISR & (tr->tcflag & ~HIFCR_FLAG)) {
		return;
	}

	if (tr->irq_sema) {
		PIOS_Semaphore_Take(tr->irq_sema, IRQ_SEMA_TIMEOUT);
	} else {
		while (!(*ISR & (tr->tcflag & ~HIFCR_FLAG))) ;
	}
}

/**
 * @brief		Checks whether there's a DMA transfer going.
 *
 * @param[in] tr 				DMA configuration.
 *
 * @returns true if a transfer is ongoing, false is it's not.
 */
bool quickdma_is_transferring(quickdma_transfer_t tr)
{
	PIOS_Assert(tr);
	PIOS_Assert(tr->c);
	PIOS_Assert(tr->c->stream);

	DMA_Stream_TypeDef *s = tr->c->stream;
	DMA_TypeDef *dma = s < DMA2_Stream0 ? DMA1 : DMA2;
	volatile uint32_t *ISR = (tr->tcflag & HIFCR_FLAG) ? &dma->HISR : &dma->LISR;

	if (!(s->CR & DMA_SxCR_EN)) {
		/* DMA not going. */
		return false;
	}

	if (*ISR & (tr->tcflag & ~HIFCR_FLAG)) {
		/* DMA has finished. */
		return false;
	}

	return true;
}

/**
 * @brief		Overrides memory pointer increase policy.
 *
 * @param[in] tr 				DMA configuration.
 * @param[in] enabled 			Whether to increase the memory pointer during transfers.
 *
 * @note Use this if you want to send dummy data from or to a peripheral, without the need
 *       to allocate a lot of memory.
 */
void quickdma_set_meminc(quickdma_transfer_t tr, bool enabled)
{
	PIOS_Assert(tr);
	PIOS_Assert(tr->c);
	PIOS_Assert(tr->c->stream);

	DMA_Stream_TypeDef *s = tr->c->stream;

	if (enabled) {
		s->CR |= DMA_SxCR_MINC;
	} else {
		s->CR &= ~DMA_SxCR_MINC;
	}
}

/**
 * @brief		Enables yielding while waiting on DMA transfers.
 *
 * @param[in]	DMA configuration.
 */
void quickdma_enable_yielding(quickdma_transfer_t tr)
{
	PIOS_Assert(tr);

	if (!tr->c->irq) {
		/* No point in setting everything up if there is no interrupt configured. */
		return;
	}

	if (!tr->irq_sema) {
		tr->irq_sema = PIOS_Semaphore_Create();
	}

	if (!root) {
		root = tr;
		return;
	}

	quickdma_transfer_t x = root;
	while (x->next) {
		x = x->next;
	}
	x->next = tr;
}

/**
 * @brief		Call from DMA IRQ handler to release a semaphore on an interrupt.
 *
 * @param[in]	Configuration struct as reference.
 */
void quickdma_irq_trigger(const struct quickdma_config *cfg)
{
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_PROLOGUE();
#endif /* PIOS_INCLUDE_CHIBIOS */

	/* Uh, don't pass NULL please? */
	PIOS_Assert(cfg);

	quickdma_transfer_t tr = root;

	while (tr) {
		if (tr->c == cfg) {
			/* Found related config. */

			DMA_Stream_TypeDef *s = tr->c->stream;
			DMA_TypeDef *dma = s < DMA2_Stream0 ? DMA1 : DMA2;
			volatile uint32_t *IFCR = (tr->tcflag & HIFCR_FLAG) ? &dma->HIFCR : &dma->LIFCR;

			/* Clear transfer complete flag */
			*IFCR |= tr->tcflag & ~HIFCR_FLAG;

			/* Trigger semaphore. */
			if (tr->irq_sema) {
				bool woken;
				PIOS_Semaphore_Give_FromISR(tr->irq_sema, &woken);
			}

			/* Disable DMA and interrupts. */
			//s->CR &= ~(DMA_SxCR_EN | DMA_SxCR_TCIE);

			break;
		}

		tr = tr->next;
	}
#if defined(PIOS_INCLUDE_CHIBIOS)
	CH_IRQ_EPILOGUE();
#endif /* PIOS_INCLUDE_CHIBIOS */
}
