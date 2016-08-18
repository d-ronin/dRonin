#include <pios_spislave.h>

#ifdef PIOS_INCLUDE_SPISLAVE

struct pios_spislave_dev {
	const struct pios_spislave_cfg *cfg;
	bool last_ssel;
};

static void setup_spi_dma(spislave_t slave_dev, uint32_t tx_len)
{
	DMA_Cmd(slave_dev->cfg->rx_dma.channel, DISABLE);
	DMA_Cmd(slave_dev->cfg->tx_dma.channel, DISABLE);

	SPI_Cmd(slave_dev->cfg->regs, DISABLE);

	SPI_I2S_DMACmd(slave_dev->cfg->regs, SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_I2S_DMACmd(slave_dev->cfg->regs, SPI_I2S_DMAReq_Rx, DISABLE);

	DMA_SetCurrDataCounter(slave_dev->cfg->tx_dma.channel, tx_len);
	DMA_SetCurrDataCounter(slave_dev->cfg->rx_dma.channel,
		slave_dev->cfg->max_rx_len);

	SPI_I2S_DMACmd(slave_dev->cfg->regs, SPI_I2S_DMAReq_Rx, ENABLE);
	DMA_Cmd(slave_dev->cfg->rx_dma.channel, ENABLE);
	DMA_Cmd(slave_dev->cfg->tx_dma.channel, ENABLE);
	SPI_I2S_DMACmd(slave_dev->cfg->regs, SPI_I2S_DMAReq_Tx, ENABLE);

	SPI_Cmd(slave_dev->cfg->regs, ENABLE);
}

int32_t PIOS_SPISLAVE_PollSS(spislave_t slave_dev)
{
	if (DMA_GetCurrDataCounter(slave_dev->cfg->rx_dma.channel) ==
			slave_dev->cfg->max_rx_len) {
		// Nothing clocked.  So even if the CS blipped, we don't
		// care-- ignore it.

		return 0;
	}

	bool pin_status = GPIO_ReadInputDataBit(slave_dev->cfg->ssel.gpio,
		slave_dev->cfg->ssel.init.GPIO_Pin) == Bit_SET;

	if (slave_dev->last_ssel != pin_status) {
		slave_dev->last_ssel = pin_status;

		/* This was a rising edge that we spotted. */
		if (pin_status) {
			int resp_len;

			/* XXX figure out count, wait as appropriate, meh */
			slave_dev->cfg->process_message(
				slave_dev->cfg->ctx, 0, &resp_len);

			setup_spi_dma(slave_dev, resp_len);
		}
	}

	return 0;
}

int32_t PIOS_SPISLAVE_Init(spislave_t *spislave_id,
		const struct pios_spislave_cfg *cfg,
		int initial_tx_size)
{
	PIOS_Assert(spislave_id);
	PIOS_Assert(cfg);

	spislave_t slave_dev = PIOS_malloc(sizeof(*slave_dev));
	if (!slave_dev) goto out_fail;

	slave_dev->cfg = cfg;
	slave_dev->last_ssel = true;

#ifndef STM32F10X_MD
	/* Initialize the GPIO pins */
	/* note __builtin_ctz() due to the difference between GPIO_PinX and GPIO_PinSourceX */
	if (slave_dev->cfg->remap) {
		GPIO_PinAFConfig(slave_dev->cfg->sclk.gpio,
				__builtin_ctz(slave_dev->cfg->sclk.init.GPIO_Pin),
				slave_dev->cfg->remap);
		GPIO_PinAFConfig(slave_dev->cfg->mosi.gpio,
				__builtin_ctz(slave_dev->cfg->mosi.init.GPIO_Pin),
				slave_dev->cfg->remap);
		GPIO_PinAFConfig(slave_dev->cfg->miso.gpio,
				__builtin_ctz(slave_dev->cfg->miso.init.GPIO_Pin),
				slave_dev->cfg->remap);
		GPIO_PinAFConfig(slave_dev->cfg->ssel.gpio,
				__builtin_ctz(slave_dev->cfg->ssel.init.GPIO_Pin),
				slave_dev->cfg->remap);
	}
#endif

	GPIO_Init(slave_dev->cfg->sclk.gpio,
			(GPIO_InitTypeDef *) & (slave_dev->cfg->sclk.init));
	GPIO_Init(slave_dev->cfg->mosi.gpio,
			(GPIO_InitTypeDef *) & (slave_dev->cfg->mosi.init));
	GPIO_Init(slave_dev->cfg->miso.gpio,
			(GPIO_InitTypeDef *) & (slave_dev->cfg->miso.init));
	GPIO_Init(slave_dev->cfg->ssel.gpio,
			(GPIO_InitTypeDef *) & (slave_dev->cfg->miso.init));

	SPI_I2S_DeInit(slave_dev->cfg->regs);
	SPI_Init(slave_dev->cfg->regs,
			(SPI_InitTypeDef *) & (slave_dev->cfg->init));
	SPI_CalculateCRC(slave_dev->cfg->regs, DISABLE);
	SPI_RxFIFOThresholdConfig(slave_dev->cfg->regs,
			SPI_RxFIFOThreshold_QF);

	DMA_DeInit(slave_dev->cfg->rx_dma.channel);
	DMA_DeInit(slave_dev->cfg->tx_dma.channel);

	DMA_Init(slave_dev->cfg->rx_dma.channel,
			(DMA_InitTypeDef *) &slave_dev->cfg->rx_dma.init);
	DMA_Init(slave_dev->cfg->tx_dma.channel,
			(DMA_InitTypeDef *) &slave_dev->cfg->tx_dma.init);

	setup_spi_dma(slave_dev, initial_tx_size);

	*spislave_id = slave_dev;

	return 0;

out_fail:
	return -1;
}

#endif
