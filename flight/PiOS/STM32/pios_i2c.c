/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_I2C I2C Functions
 * @brief STM32F4xx Hardware dependent I2C functionality
 * @{
 *
 * @file       pios_i2c.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
 * @author     dRonin, http://dronin.org, Copyright (C) 2016-2017
 * @brief      I2C Enable/Disable routines
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

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_I2C)

#include <pios_i2c_priv.h>

struct pios_i2c_adapter {
	enum pios_i2c_adapter_magic         magic;
	const struct pios_i2c_adapter_cfg * cfg;

	struct pios_mutex *lock;
	struct pios_semaphore *sem_ready;

	bool bus_error;
	bool nack;

	volatile enum i2c_adapter_state state;
	const struct pios_i2c_txn *active_txn;
	const struct pios_i2c_txn *last_txn;

	uint8_t *active_byte;
	uint8_t *last_byte;

#if defined(PIOS_I2C_DIAGNOSTICS)
	volatile struct pios_i2c_fault_history i2c_adapter_fault_history;

	volatile uint32_t i2c_evirq_history[I2C_LOG_DEPTH];
	volatile uint8_t i2c_evirq_history_pointer;

	volatile uint32_t i2c_erirq_history[I2C_LOG_DEPTH];
	volatile uint8_t i2c_erirq_history_pointer;

	volatile enum i2c_adapter_state i2c_state_history[I2C_LOG_DEPTH];
	volatile uint8_t i2c_state_history_pointer;

	volatile enum i2c_adapter_event i2c_state_event_history[I2C_LOG_DEPTH];
	volatile uint8_t i2c_state_event_history_pointer;

	volatile uint32_t i2c_fsm_fault_count;
	volatile uint32_t i2c_bad_event_counter;
	volatile uint32_t i2c_error_interrupt_counter;
	volatile uint32_t i2c_nack_counter;
	volatile uint32_t i2c_timeout_counter;
#endif
};

#if defined(PIOS_I2C_DIAGNOSTICS)

#define I2C_DIAG(adapter, field, val) \
	do { \
		(adapter)->i2c_##field##_history[(adapter)->i2c_##field##_history_pointer] = val; \
		(adapter)->i2c_##field##_history_pointer = ((adapter)->i2c_##field##_history_pointer + 1) % I2C_LOG_DEPTH; \
	} while (0)

#define I2C_DIAG_INCR(adapter, field) do { (adapter)->field++; } while (0)
#else

#define I2C_DIAG(adapter, field, val) do { (void) (adapter);  (void) (val); } while (0)

#define I2C_DIAG_INCR(adapter, field) do { (void) (adapter); } while (0)

#define i2c_adapter_log_fault(adapter, val) do { (void) (adapter);  (void) (val); } while (0)
#endif

static void i2c_adapter_inject_event(struct pios_i2c_adapter *i2c_adapter, enum i2c_adapter_event event, bool *woken);

static bool PIOS_I2C_validate(struct pios_i2c_adapter *i2c_adapter)
{
	return i2c_adapter->magic == PIOS_I2C_DEV_MAGIC;
}

static struct pios_i2c_adapter *PIOS_I2C_alloc(void)
{
	struct pios_i2c_adapter *i2c_adapter;

	i2c_adapter =  PIOS_malloc(sizeof(struct pios_i2c_adapter));

	if (i2c_adapter == NULL)
		return NULL;

	// init all to zero
	memset(i2c_adapter, 0, sizeof(*i2c_adapter));

	// set magic
	i2c_adapter->magic = PIOS_I2C_DEV_MAGIC;

	return i2c_adapter;
}

static void i2c_adapter_reset_bus(struct pios_i2c_adapter *i2c_adapter)
{
	uint8_t retry_count = 0;
	uint8_t retry_count_clk = 0;
	static const uint8_t MAX_I2C_RETRY_COUNT = 10;

	/* Reset the I2C block */
	I2C_DeInit(i2c_adapter->cfg->regs);

	/* Make sure the bus is free by clocking it until any slaves release the bus. */
	GPIO_InitTypeDef scl_gpio_init;
	scl_gpio_init = i2c_adapter->cfg->scl.init;
	scl_gpio_init.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_SetBits(i2c_adapter->cfg->scl.gpio, i2c_adapter->cfg->scl.init.GPIO_Pin);
	GPIO_Init(i2c_adapter->cfg->scl.gpio, &scl_gpio_init);

	GPIO_InitTypeDef sda_gpio_init;
	sda_gpio_init = i2c_adapter->cfg->sda.init;
	sda_gpio_init.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_SetBits(i2c_adapter->cfg->sda.gpio, i2c_adapter->cfg->sda.init.GPIO_Pin);
	GPIO_Init(i2c_adapter->cfg->sda.gpio, &sda_gpio_init);

	/* Check SDA line to determine if slave is asserting bus and clock out if so, this may  */
	/* have to be repeated (due to further bus errors) but better than clocking 0xFF into an */
	/* ESC */

	retry_count_clk = 0;
	while (GPIO_ReadInputDataBit(i2c_adapter->cfg->sda.gpio, i2c_adapter->cfg->sda.init.GPIO_Pin) == Bit_RESET && (retry_count_clk++ < MAX_I2C_RETRY_COUNT)) {
		retry_count = 0;
		/* Set clock high and wait for any clock stretching to finish. */
		GPIO_SetBits(i2c_adapter->cfg->scl.gpio, i2c_adapter->cfg->scl.init.GPIO_Pin);
		while (GPIO_ReadInputDataBit(i2c_adapter->cfg->scl.gpio, i2c_adapter->cfg->scl.init.GPIO_Pin) == Bit_RESET && (retry_count++ < MAX_I2C_RETRY_COUNT))
			PIOS_DELAY_WaituS(1);

		PIOS_DELAY_WaituS(2);

		/* Set clock low */
		GPIO_ResetBits(i2c_adapter->cfg->scl.gpio, i2c_adapter->cfg->scl.init.GPIO_Pin);
		PIOS_DELAY_WaituS(2);

		/* Clock high again */
		GPIO_SetBits(i2c_adapter->cfg->scl.gpio, i2c_adapter->cfg->scl.init.GPIO_Pin);
		PIOS_DELAY_WaituS(2);
	}

	/* Generate a start then stop condition */
	GPIO_SetBits(i2c_adapter->cfg->scl.gpio, i2c_adapter->cfg->scl.init.GPIO_Pin);
	PIOS_DELAY_WaituS(2);
	GPIO_ResetBits(i2c_adapter->cfg->sda.gpio, i2c_adapter->cfg->sda.init.GPIO_Pin);
	PIOS_DELAY_WaituS(2);
	GPIO_ResetBits(i2c_adapter->cfg->sda.gpio, i2c_adapter->cfg->sda.init.GPIO_Pin);
	PIOS_DELAY_WaituS(2);

	/* Set data and clock high and wait for any clock stretching to finish. */
	GPIO_SetBits(i2c_adapter->cfg->sda.gpio, i2c_adapter->cfg->sda.init.GPIO_Pin);
	GPIO_SetBits(i2c_adapter->cfg->scl.gpio, i2c_adapter->cfg->scl.init.GPIO_Pin);

	retry_count = 0;
	while (GPIO_ReadInputDataBit(i2c_adapter->cfg->scl.gpio, i2c_adapter->cfg->scl.init.GPIO_Pin) == Bit_RESET && (retry_count++ < MAX_I2C_RETRY_COUNT))
		PIOS_DELAY_WaituS(1);

	/* Wait for data to be high */
	retry_count = 0;
	while (GPIO_ReadInputDataBit(i2c_adapter->cfg->sda.gpio, i2c_adapter->cfg->sda.init.GPIO_Pin) != Bit_SET && (retry_count++ < MAX_I2C_RETRY_COUNT))
		PIOS_DELAY_WaituS(1);

	/* Bus signals are guaranteed to be high (ie. free) after this point */
	/* Initialize the GPIO pins to the peripheral function */
	if (i2c_adapter->cfg->remap) {
		GPIO_PinAFConfig(i2c_adapter->cfg->scl.gpio,
				__builtin_ctz(i2c_adapter->cfg->scl.init.GPIO_Pin),
				i2c_adapter->cfg->remap);
		GPIO_PinAFConfig(i2c_adapter->cfg->sda.gpio,
				__builtin_ctz(i2c_adapter->cfg->sda.init.GPIO_Pin),
				i2c_adapter->cfg->remap);
	}
	GPIO_Init(i2c_adapter->cfg->scl.gpio, (GPIO_InitTypeDef *) & (i2c_adapter->cfg->scl.init)); // Struct is const, function signature not
	GPIO_Init(i2c_adapter->cfg->sda.gpio, (GPIO_InitTypeDef *) & (i2c_adapter->cfg->sda.init));

	/* Reset the I2C block */
	I2C_DeInit(i2c_adapter->cfg->regs);

	/* Initialize the I2C block */
	I2C_Init(i2c_adapter->cfg->regs, (I2C_InitTypeDef *) & (i2c_adapter->cfg->init));

	/* Enable the I2C block */
	I2C_Cmd(i2c_adapter->cfg->regs, ENABLE);

#if defined(STM32F30X)
	if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_BUSY))
		I2C_SoftwareResetCmd(i2c_adapter->cfg->regs);
#elif defined(STM32F4XX)
	if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_BUSY))
		I2C_SoftwareResetCmd(i2c_adapter->cfg->regs, ENABLE);
#endif
}

/**
 * @brief Check the I2C bus is clear and in a properly reset state
 * @returns  0 Bus is clear
 * @returns -1 Bus is in use
 * @returns -2 Bus not clear
 */
int32_t PIOS_I2C_CheckClear(pios_i2c_t i2c_adapter)
{
	bool valid = PIOS_I2C_validate(i2c_adapter);
	PIOS_Assert(valid);

	if (PIOS_Mutex_Lock(i2c_adapter->lock, 0) == false)
		return -1;

	if (i2c_adapter->state != I2C_STATE_STOPPED) {
		PIOS_Mutex_Unlock(i2c_adapter->lock);
		return -2;
	}

	if (GPIO_ReadInputDataBit(i2c_adapter->cfg->sda.gpio, i2c_adapter->cfg->sda.init.GPIO_Pin) == Bit_RESET ||
		GPIO_ReadInputDataBit(i2c_adapter->cfg->scl.gpio, i2c_adapter->cfg->scl.init.GPIO_Pin) == Bit_RESET) {
		PIOS_Mutex_Unlock(i2c_adapter->lock);
		return -3;
	}

	PIOS_Mutex_Unlock(i2c_adapter->lock);

	return 0;
}

static void i2c_adapter_fsm_init(struct pios_i2c_adapter *i2c_adapter)
{
	i2c_adapter_reset_bus(i2c_adapter);
	i2c_adapter->state = I2C_STATE_STOPPED;
}

/**
* Initializes IIC driver
* \param[in] mode currently only mode 0 supported
* \return < 0 if initialisation failed
*/
int32_t PIOS_I2C_Init(pios_i2c_t *i2c_id, const struct pios_i2c_adapter_cfg *cfg)
{
	PIOS_DEBUG_Assert(i2c_id);
	PIOS_DEBUG_Assert(cfg);

	struct pios_i2c_adapter *i2c_adapter;

	i2c_adapter = (struct pios_i2c_adapter *) PIOS_I2C_alloc();
	if (!i2c_adapter)
		goto out_fail;

	/* Bind the configuration to the device instance */
	i2c_adapter->cfg = cfg;

	i2c_adapter->sem_ready = PIOS_Semaphore_Create();
	i2c_adapter->lock = PIOS_Mutex_Create();

	/* Initialize the state machine */
	i2c_adapter_fsm_init(i2c_adapter);

	*i2c_id = i2c_adapter;

	/* Configure and enable I2C interrupts */
	NVIC_Init((NVIC_InitTypeDef *) & (i2c_adapter->cfg->event.init));
	NVIC_Init((NVIC_InitTypeDef *) & (i2c_adapter->cfg->error.init));

	/* No error */
	return 0;

out_fail:
	return -1;
}

int32_t PIOS_I2C_Transfer(pios_i2c_t i2c_adapter, const struct pios_i2c_txn txn_list[], uint32_t num_txns)
{
	bool valid = PIOS_I2C_validate(i2c_adapter);
	PIOS_Assert(valid);

	PIOS_DEBUG_Assert(txn_list);
	PIOS_DEBUG_Assert(num_txns);

	if (PIOS_Mutex_Lock(i2c_adapter->lock, i2c_adapter->cfg->transfer_timeout_ms) == false)
		return -2;

	PIOS_DEBUG_Assert(i2c_adapter->state == I2C_STATE_STOPPED);

	i2c_adapter->last_txn = &txn_list[num_txns - 1];
	i2c_adapter->active_txn = &txn_list[0];
	i2c_adapter->bus_error = false;
	i2c_adapter->nack = false;

	/* Make sure the done/ready semaphore is consumed before we start */
	PIOS_Semaphore_Take(i2c_adapter->sem_ready, 0);

	bool dummy = false;
	i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_START, &dummy);

	/* Wait for the transfer to complete */
	bool semaphore_success = (PIOS_Semaphore_Take(i2c_adapter->sem_ready, i2c_adapter->cfg->transfer_timeout_ms) == true);

	if (!semaphore_success)
		I2C_DIAG_INCR(i2c_adapter, i2c_timeout_counter);

	int32_t result = !semaphore_success ? -2 :
			i2c_adapter->bus_error ? -1 :
			i2c_adapter->nack ? -3 :
			0;

	PIOS_Mutex_Unlock(i2c_adapter->lock);

	return result;
}

#if defined(PIOS_I2C_DIAGNOSTICS)
/**
 * Logs the last N state transitions and N IRQ events due to
 * an error condition
 * \param[in] i2c the adapter number to log an event for
 */
static void i2c_adapter_log_fault(struct pios_i2c_adapter *i2c_adapter, enum pios_i2c_error_type type)
{
	i2c_adapter->i2c_adapter_fault_history.type = type;
	for (uint8_t i = 0; i < I2C_LOG_DEPTH; i++) {
		i2c_adapter->i2c_adapter_fault_history.evirq[i] =
				i2c_adapter->i2c_evirq_history[(I2C_LOG_DEPTH + i2c_adapter->i2c_evirq_history_pointer - 1 - i) % I2C_LOG_DEPTH];
		i2c_adapter->i2c_adapter_fault_history.erirq[i] =
				i2c_adapter->i2c_erirq_history[(I2C_LOG_DEPTH + i2c_adapter->i2c_erirq_history_pointer - 1 - i) % I2C_LOG_DEPTH];
		i2c_adapter->i2c_adapter_fault_history.event[i] =
				i2c_adapter->i2c_state_event_history[(I2C_LOG_DEPTH + i2c_adapter->i2c_state_event_history_pointer - 1 - i) % I2C_LOG_DEPTH];
		i2c_adapter->i2c_adapter_fault_history.state[i] =
				i2c_adapter->i2c_state_history[(I2C_LOG_DEPTH + i2c_adapter->i2c_state_history_pointer - 1 - i) % I2C_LOG_DEPTH];
	}
	switch (type) {
	case PIOS_I2C_ERROR_EVENT:
		i2c_adapter->i2c_bad_event_counter++;
		break;
	case PIOS_I2C_ERROR_FSM:
		i2c_adapter->i2c_fsm_fault_count++;
		break;
	case PIOS_I2C_ERROR_INTERRUPT:
		i2c_adapter->i2c_error_interrupt_counter++;
		break;
	}
}
#endif

static void go_fsm_fault(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void go_bus_error(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void go_stopped(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void go_starting(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void go_nack(struct pios_i2c_adapter *i2c_adapter, bool *woken);

//#define I2C_HALT_ON_ERRORS

static void receive_byte(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	// read received byte from buffer
	*i2c_adapter->active_byte = I2C_ReceiveData(i2c_adapter->cfg->regs);

	/* Move to the next byte */
	i2c_adapter->active_byte++;
}

static void send_byte(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	// write byte to buffer
	I2C_SendData(i2c_adapter->cfg->regs, *i2c_adapter->active_byte);

	/* Move to the next byte */
	i2c_adapter->active_byte++;
}


struct i2c_adapter_transition {
	void (*entry_fn)(struct pios_i2c_adapter *i2c_adapter, bool *woken);
	enum i2c_adapter_state next_state[I2C_EVENT_NUM_EVENTS];
};

#if defined(STM32F30X)
static void go_transfer_complete(struct pios_i2c_adapter *i2c_adapter, bool *woken);

const static struct i2c_adapter_transition i2c_adapter_transitions[I2C_STATE_NUM_STATES] = {
	[I2C_STATE_FSM_FAULT] = {
		.entry_fn = go_fsm_fault,
		.next_state = {
			[I2C_EVENT_AUTO] = I2C_STATE_STOPPED,
		},
	},
	[I2C_STATE_BUS_ERROR] = {
		.entry_fn = go_bus_error,
		.next_state = {
			[I2C_EVENT_AUTO] = I2C_STATE_STOPPED,
		},
	},

	[I2C_STATE_STOPPED] = {
		.entry_fn = go_stopped,
		.next_state = {
			[I2C_EVENT_START] = I2C_STATE_STARTING,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},

	[I2C_STATE_STARTING] = {
		.entry_fn = go_starting,
		.next_state = {
			[I2C_EVENT_TRANSMIT_BUFFER_EMPTY] = I2C_STATE_WRITE_BYTE,
			[I2C_EVENT_RECEIVER_BUFFER_NOT_EMPTY] = I2C_STATE_READ_BYTE,
			[I2C_EVENT_NACK] = I2C_STATE_NACK,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},

	[I2C_STATE_WRITE_BYTE] = {
		.entry_fn = send_byte,
		.next_state = {
			[I2C_EVENT_TRANSMIT_BUFFER_EMPTY] = I2C_STATE_WRITE_BYTE,
			[I2C_EVENT_RECEIVER_BUFFER_NOT_EMPTY] = I2C_STATE_READ_BYTE,
			[I2C_EVENT_TRANSFER_COMPLETE] = I2C_STATE_TRANSFER_COMPLETE,
			[I2C_EVENT_STOP] = I2C_STATE_STOPPED,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},

	[I2C_STATE_READ_BYTE] = {
		.entry_fn = receive_byte,
		.next_state = {
			[I2C_EVENT_TRANSMIT_BUFFER_EMPTY] = I2C_STATE_WRITE_BYTE,
			[I2C_EVENT_RECEIVER_BUFFER_NOT_EMPTY] = I2C_STATE_READ_BYTE,
			[I2C_EVENT_TRANSFER_COMPLETE] = I2C_STATE_TRANSFER_COMPLETE,
			[I2C_EVENT_STOP] = I2C_STATE_STOPPED,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},

	[I2C_STATE_TRANSFER_COMPLETE] = {
		.entry_fn = go_transfer_complete,
		.next_state = {
			[I2C_EVENT_AUTO] = I2C_STATE_STARTING,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},

	[I2C_STATE_NACK] = {
		.entry_fn = go_nack,
		.next_state = {
			[I2C_EVENT_AUTO] = I2C_STATE_STOPPED,
		},
	},
};
#elif defined(STM32F4XX)

static void prepare_nack(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void prepare_ack(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void receive_byte(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void send_byte(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void send_address_and_enable_BUF_intr(struct pios_i2c_adapter *i2c_adapter,
		bool *woken);
static void send_w_address(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void receive_and_setup_nack(struct pios_i2c_adapter *i2c_adapter, bool *woken);
static void advance_txn_and_start(struct pios_i2c_adapter *i2c_adapter, bool *woken);

static void go_r_last_txn_post_last(struct pios_i2c_adapter *i2c_adapter, bool *woken);

static void go_w_last_txn_post_last(struct pios_i2c_adapter *i2c_adapter, bool *woken);

static const struct i2c_adapter_transition i2c_adapter_transitions[I2C_STATE_NUM_STATES] = {
	[I2C_STATE_FSM_FAULT] = {
		.entry_fn = go_fsm_fault,
		.next_state = {
			[I2C_EVENT_AUTO] = I2C_STATE_STOPPED,
		},
	},
	[I2C_STATE_BUS_ERROR] = {
		.entry_fn = go_bus_error,
		.next_state = {
			[I2C_EVENT_AUTO] = I2C_STATE_STOPPED,
		},
	},
	[I2C_STATE_NACK] = {
		.entry_fn = go_nack,
		.next_state = {
			[I2C_EVENT_AUTO] = I2C_STATE_STOPPED,
		},
	},
	[I2C_STATE_STOPPED] = {
		.entry_fn = go_stopped,
		.next_state = {
			[I2C_EVENT_START] = I2C_STATE_STARTING,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_STARTING] = {
		.entry_fn = go_starting,
		.next_state = {
			[I2C_EVENT_R_MORE_TXN_STARTED] = I2C_STATE_R_MORE_TXN_ADDR,
			[I2C_EVENT_W_MORE_TXN_STARTED] = I2C_STATE_W_MORE_TXN_ADDR,
			[I2C_EVENT_R_LAST_TXN_STARTED] = I2C_STATE_R_LAST_TXN_ADDR,
			[I2C_EVENT_W_LAST_TXN_STARTED] = I2C_STATE_W_LAST_TXN_ADDR,
			[I2C_EVENT_NACK] = I2C_STATE_NACK,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},

	/*
	 * Read with restart
	 */
	[I2C_STATE_R_MORE_TXN_ADDR] = {
		.entry_fn = send_address_and_enable_BUF_intr,
		.next_state = {
			[I2C_EVENT_ADDR_SENT_LEN_EQ_1] = I2C_STATE_R_MORE_TXN_PRE_ONE,
			[I2C_EVENT_ADDR_SENT_LEN_EQ_2] = I2C_STATE_R_MORE_TXN_PRE_FIRST,
			[I2C_EVENT_ADDR_SENT_LEN_GT_2] = I2C_STATE_R_MORE_TXN_PRE_FIRST,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_R_MORE_TXN_PRE_ONE] = {
		.entry_fn = prepare_nack,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_1] = I2C_STATE_MORE_TXN_POST_LAST,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_R_MORE_TXN_PRE_FIRST] = {
		.entry_fn = prepare_ack,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_2] = I2C_STATE_R_MORE_TXN_PRE_LAST,
			[I2C_EVENT_TRANSFER_DONE_LEN_GT_2] = I2C_STATE_R_MORE_TXN_PRE_MIDDLE,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_R_MORE_TXN_PRE_MIDDLE] = {
		.entry_fn = receive_byte,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_2] = I2C_STATE_R_MORE_TXN_PRE_LAST,
			[I2C_EVENT_TRANSFER_DONE_LEN_GT_2] = I2C_STATE_R_MORE_TXN_PRE_MIDDLE,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_R_MORE_TXN_PRE_LAST] = {
		.entry_fn = receive_and_setup_nack,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_1] = I2C_STATE_MORE_TXN_POST_LAST,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},

	/*
	 * Read with stop
	 */
	[I2C_STATE_R_LAST_TXN_ADDR] = {
		.entry_fn = send_address_and_enable_BUF_intr,
		.next_state = {
			[I2C_EVENT_ADDR_SENT_LEN_EQ_1] = I2C_STATE_R_LAST_TXN_PRE_ONE,
			[I2C_EVENT_ADDR_SENT_LEN_EQ_2] = I2C_STATE_R_LAST_TXN_PRE_FIRST,
			[I2C_EVENT_ADDR_SENT_LEN_GT_2] = I2C_STATE_R_LAST_TXN_PRE_FIRST,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_R_LAST_TXN_PRE_ONE] = {
		.entry_fn = prepare_nack,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_1] = I2C_STATE_R_LAST_TXN_POST_LAST,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_R_LAST_TXN_PRE_FIRST] = {
		.entry_fn = prepare_ack,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_2] = I2C_STATE_R_LAST_TXN_PRE_LAST,
			[I2C_EVENT_TRANSFER_DONE_LEN_GT_2] = I2C_STATE_R_LAST_TXN_PRE_MIDDLE,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_R_LAST_TXN_PRE_MIDDLE] = {
		.entry_fn = receive_byte,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_2] = I2C_STATE_R_LAST_TXN_PRE_LAST,
			[I2C_EVENT_TRANSFER_DONE_LEN_GT_2] = I2C_STATE_R_LAST_TXN_PRE_MIDDLE,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_R_LAST_TXN_PRE_LAST] = {
		.entry_fn = receive_and_setup_nack,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_1] = I2C_STATE_R_LAST_TXN_POST_LAST,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_R_LAST_TXN_POST_LAST] = {
		.entry_fn = go_r_last_txn_post_last,
		.next_state = {
			[I2C_EVENT_AUTO] = I2C_STATE_STOPPED,
		},
	},

	/*
	 * Write with restart
	 */
	[I2C_STATE_W_MORE_TXN_ADDR] = {
		.entry_fn = send_w_address,
		.next_state = {
			[I2C_EVENT_ADDR_SENT_LEN_EQ_1] = I2C_STATE_W_MORE_TXN_PRE,
			[I2C_EVENT_ADDR_SENT_LEN_EQ_2] = I2C_STATE_W_MORE_TXN_PRE,
			[I2C_EVENT_ADDR_SENT_LEN_GT_2] = I2C_STATE_W_MORE_TXN_PRE,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_W_MORE_TXN_PRE] = {
		.entry_fn = send_byte,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_0] = I2C_STATE_MORE_TXN_POST_LAST,
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_1] = I2C_STATE_W_MORE_TXN_PRE,
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_2] = I2C_STATE_W_MORE_TXN_PRE,
			[I2C_EVENT_TRANSFER_DONE_LEN_GT_2] = I2C_STATE_W_MORE_TXN_PRE,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},

	/* Translation finishup before end of sequence */
	[I2C_STATE_MORE_TXN_POST_LAST] = {
		.entry_fn = advance_txn_and_start,
		.next_state = {
			[I2C_EVENT_R_MORE_TXN_STARTED] = I2C_STATE_R_MORE_TXN_ADDR,
			[I2C_EVENT_W_MORE_TXN_STARTED] = I2C_STATE_W_MORE_TXN_ADDR,
			[I2C_EVENT_R_LAST_TXN_STARTED] = I2C_STATE_R_LAST_TXN_ADDR,
			[I2C_EVENT_W_LAST_TXN_STARTED] = I2C_STATE_W_LAST_TXN_ADDR,
			[I2C_EVENT_NACK] = I2C_STATE_NACK,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},

	/*
	 * Write with stop
	 */
	[I2C_STATE_W_LAST_TXN_ADDR] = {
		.entry_fn = send_w_address,
		.next_state = {
			[I2C_EVENT_ADDR_SENT_LEN_EQ_1] = I2C_STATE_W_LAST_TXN_PRE,
			[I2C_EVENT_ADDR_SENT_LEN_EQ_2] = I2C_STATE_W_LAST_TXN_PRE,
			[I2C_EVENT_ADDR_SENT_LEN_GT_2] = I2C_STATE_W_LAST_TXN_PRE,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_W_LAST_TXN_PRE] = {
		.entry_fn = send_byte,
		.next_state = {
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_0] = I2C_STATE_W_LAST_TXN_POST_LAST,
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_1] = I2C_STATE_W_LAST_TXN_PRE,
			[I2C_EVENT_TRANSFER_DONE_LEN_EQ_2] = I2C_STATE_W_LAST_TXN_PRE,
			[I2C_EVENT_TRANSFER_DONE_LEN_GT_2] = I2C_STATE_W_LAST_TXN_PRE,
			[I2C_EVENT_BUS_ERROR] = I2C_STATE_BUS_ERROR,
		},
	},
	[I2C_STATE_W_LAST_TXN_POST_LAST] = {
		.entry_fn = go_w_last_txn_post_last,
		.next_state = {
			[I2C_EVENT_AUTO] = I2C_STATE_STOPPED,
		},
	},
};

#endif /* STM32F4XX */

static void i2c_adapter_process_auto(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	enum i2c_adapter_state prev_state = i2c_adapter->state;
	if (prev_state);

	while (i2c_adapter_transitions[i2c_adapter->state].next_state[I2C_EVENT_AUTO]) {
		i2c_adapter->state = i2c_adapter_transitions[i2c_adapter->state].next_state[I2C_EVENT_AUTO];

		/* Call the entry function (if any) for the next state. */
		if (i2c_adapter_transitions[i2c_adapter->state].entry_fn) {
			i2c_adapter_transitions[i2c_adapter->state].entry_fn(i2c_adapter, woken);
		}
	}
}

static void i2c_adapter_inject_event(struct pios_i2c_adapter *i2c_adapter, enum i2c_adapter_event event, bool *woken)
{
	I2C_DIAG(i2c_adapter, state_event, event);
	I2C_DIAG(i2c_adapter, state, i2c_adapter->state);

	if (i2c_adapter_transitions[i2c_adapter->state].next_state[event] == I2C_STATE_FSM_FAULT)
		i2c_adapter_log_fault(i2c_adapter, PIOS_I2C_ERROR_FSM);

	/*
	 * Move to the next state
	 *
	 * This is done prior to calling the new state's entry function to
	 * guarantee that the entry function never depends on the previous
	 * state.  This way, it cannot ever know what the previous state was.
	 */
	enum i2c_adapter_state prev_state = i2c_adapter->state;
	if (prev_state);

	i2c_adapter->state = i2c_adapter_transitions[i2c_adapter->state].next_state[event];

	/* Call the entry function (if any) for the next state. */
	if (i2c_adapter_transitions[i2c_adapter->state].entry_fn) {
		i2c_adapter_transitions[i2c_adapter->state].entry_fn(i2c_adapter, woken);
	}

	/* Process any AUTO transitions in the FSM */
	i2c_adapter_process_auto(i2c_adapter, woken);
}

static void go_fsm_fault(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
#if defined(I2C_HALT_ON_ERRORS)
	PIOS_DEBUG_Assert(0);
#endif
	/* Note that this transfer has hit a bus error */
	i2c_adapter->bus_error = true;

	i2c_adapter_reset_bus(i2c_adapter);
}

static void go_bus_error(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	/* Note that this transfer has hit a bus error */
	i2c_adapter->bus_error = true;

	i2c_adapter_reset_bus(i2c_adapter);
}

static void set_i2c_irqs(struct pios_i2c_adapter *i2c_adapter, FunctionalState new_state)
{
#if defined(STM32F30X)
	I2C_ITConfig(i2c_adapter->cfg->regs, I2C_IT_ERRI | I2C_IT_TCI | I2C_IT_NACKI | I2C_IT_RXI | I2C_IT_STOPI | I2C_IT_TXI, new_state);
#elif defined(STM32F4XX)
	I2C_ITConfig(i2c_adapter->cfg->regs, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, new_state);
#endif
}

static void go_nack(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	i2c_adapter->nack = true;
	set_i2c_irqs(i2c_adapter, DISABLE);	// XXX this one used to only happen on F3

	I2C_AcknowledgeConfig(i2c_adapter->cfg->regs, DISABLE);
	I2C_GenerateSTART(i2c_adapter->cfg->regs, DISABLE);
	I2C_GenerateSTOP(i2c_adapter->cfg->regs, ENABLE);
}

static void go_stopped(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	set_i2c_irqs(i2c_adapter, DISABLE);

	/* wake up blocked PIOS_I2C_Transfer() */
	if (PIOS_Semaphore_Give_FromISR(i2c_adapter->sem_ready, woken) == false) {
#if defined(I2C_HALT_ON_ERRORS)
		PIOS_DEBUG_Assert(0);
#endif
	}
}

static void setup_txn_ptrs(struct pios_i2c_adapter *i2c_adapter)
{
	// set up current txn byte pointers
	i2c_adapter->active_byte = &(i2c_adapter->active_txn->buf[0]);
	i2c_adapter->last_byte = &(i2c_adapter->active_txn->buf[i2c_adapter->active_txn->len - 1]);
}

#if defined(STM32F30X)
static void go_starting(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	PIOS_DEBUG_Assert(i2c_adapter->active_txn);
	PIOS_DEBUG_Assert(i2c_adapter->active_txn <= i2c_adapter->last_txn);
	PIOS_DEBUG_Assert(i2c_adapter->active_txn->len <= 255);	//FIXME: implement this using TCR

	setup_txn_ptrs(i2c_adapter);

	set_i2c_irqs(i2c_adapter, ENABLE);

	I2C_TransferHandling(
		i2c_adapter->cfg->regs,
		(i2c_adapter->active_txn->addr << 1),
		i2c_adapter->active_txn->len,
		i2c_adapter->active_txn == i2c_adapter->last_txn ? I2C_AutoEnd_Mode : I2C_SoftEnd_Mode,
		i2c_adapter->active_txn->rw == PIOS_I2C_TXN_WRITE ? I2C_Generate_Start_Write : I2C_Generate_Start_Read);
}

static void go_transfer_complete(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	/* Move to the next transaction */
	i2c_adapter->active_txn++;
	PIOS_DEBUG_Assert(i2c_adapter->active_txn <= i2c_adapter->last_txn);
}

void PIOS_I2C_EV_IRQ_Handler(pios_i2c_t i2c_adapter)
{
	PIOS_IRQ_Prologue();

	bool valid = PIOS_I2C_validate(i2c_adapter);
	PIOS_Assert(valid);

	bool woken = false;

	if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_RXNE)) {
		//flag will be cleared by event
		I2C_DIAG(i2c_adapter, evirq, I2C_FLAG_RXNE);
		i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_RECEIVER_BUFFER_NOT_EMPTY, &woken);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_TXIS)) {
		//flag will be cleared by event
		I2C_DIAG(i2c_adapter, evirq, I2C_FLAG_TXIS);
		i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_TRANSMIT_BUFFER_EMPTY, &woken);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_NACKF)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_NACKF);
		I2C_DIAG(i2c_adapter, evirq, I2C_FLAG_NACKF);
		I2C_DIAG_INCR(i2c_adapter, i2c_nack_counter);
		i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_NACK, &woken);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_TC)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_TC);
		I2C_DIAG(i2c_adapter, evirq, I2C_FLAG_TC);
		i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_TRANSFER_COMPLETE, &woken);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_STOPF)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_STOPF);
		I2C_DIAG(i2c_adapter, evirq, I2C_FLAG_STOPF);
		i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_STOP, &woken);
	}

	PIOS_IRQ_Epilogue();
}


void PIOS_I2C_ER_IRQ_Handler(pios_i2c_t i2c_adapter)
{
	PIOS_IRQ_Prologue();

	bool valid = PIOS_I2C_validate(i2c_adapter);
	PIOS_Assert(valid);

	if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_BERR)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_BERR);
		I2C_DIAG(i2c_adapter, erirq, I2C_FLAG_BERR);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_ARLO)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_ARLO);
		I2C_DIAG(i2c_adapter, erirq, I2C_FLAG_ARLO);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_OVR)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_OVR);
		I2C_DIAG(i2c_adapter, erirq, I2C_FLAG_OVR);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_PECERR)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_PECERR);
		I2C_DIAG(i2c_adapter, erirq, I2C_FLAG_PECERR);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_TIMEOUT)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_TIMEOUT);
		I2C_DIAG(i2c_adapter, erirq, I2C_FLAG_TIMEOUT);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_ALERT)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_ALERT);
		I2C_DIAG(i2c_adapter, erirq, I2C_FLAG_ALERT);
	} else if (I2C_GetFlagStatus(i2c_adapter->cfg->regs, I2C_FLAG_BUSY)) {
		I2C_ClearFlag(i2c_adapter->cfg->regs, I2C_FLAG_BUSY);
		I2C_DIAG(i2c_adapter, erirq, I2C_FLAG_BUSY);
	}

	i2c_adapter_log_fault(i2c_adapter, PIOS_I2C_ERROR_INTERRUPT);

	/* Fail hard on any errors for now */
	bool woken = false;
	i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_BUS_ERROR, &woken);

	PIOS_IRQ_Epilogue();
}

#elif defined(STM32F4XX)

static void go_starting(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	setup_txn_ptrs(i2c_adapter);

	// enable interrupts
	I2C_ITConfig(i2c_adapter->cfg->regs, I2C_IT_EVT | I2C_IT_ERR, ENABLE);

	// generate a start condition
	I2C_GenerateSTART(i2c_adapter->cfg->regs, ENABLE);
}

static void send_address_and_enable_BUF_intr(struct pios_i2c_adapter *i2c_adapter,
		bool *woken)
{
	// enable buffer RxNE
	I2C_ITConfig(i2c_adapter->cfg->regs, I2C_IT_BUF, ENABLE);

	I2C_Send7bitAddress(i2c_adapter->cfg->regs, (i2c_adapter->active_txn->addr) << 1, I2C_Direction_Receiver);
}

static void prepare_nack(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	I2C_AcknowledgeConfig(i2c_adapter->cfg->regs, DISABLE);
}

static void prepare_ack(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	I2C_AcknowledgeConfig(i2c_adapter->cfg->regs, ENABLE);
}

static void advance_txn_and_start(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	// the last byte of this txn has been transmitted, so disable the buffer TxE
	I2C_ITConfig(i2c_adapter->cfg->regs, I2C_IT_BUF, DISABLE);

	if (i2c_adapter->active_txn->rw == PIOS_I2C_TXN_WRITE) {
		// do a dummy read to clear buf
		(void)I2C_ReceiveData(i2c_adapter->cfg->regs);
	} else {
		receive_byte(i2c_adapter, woken);
	}

	/* Move to the next transaction */
	i2c_adapter->active_txn++;

	setup_txn_ptrs(i2c_adapter);

	// generate restart
	I2C_GenerateSTART(i2c_adapter->cfg->regs, ENABLE);
}

static void send_w_address(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	I2C_Send7bitAddress(i2c_adapter->cfg->regs, (i2c_adapter->active_txn->addr) << 1, I2C_Direction_Transmitter);
}

static void receive_and_setup_nack(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	receive_byte(i2c_adapter, woken);

	// send a nack on the last byte
	I2C_AcknowledgeConfig(i2c_adapter->cfg->regs, DISABLE);
}

/*
 * Read with stop
 */
static void go_r_last_txn_post_last(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	// the last byte of this txn has been received, so disable the buffer RxNE
	I2C_ITConfig(i2c_adapter->cfg->regs, I2C_IT_BUF, DISABLE);

	receive_byte(i2c_adapter, woken);

	// generate a stop condition
	I2C_GenerateSTOP(i2c_adapter->cfg->regs, ENABLE);

	/* Move to the next transaction */
	i2c_adapter->active_txn++;
}


/*
 * Write with stop
 */
static void go_w_last_txn_post_last(struct pios_i2c_adapter *i2c_adapter, bool *woken)
{
	// the last byte of this txn has been transmitted, so disable the buffer TxE
	I2C_ITConfig(i2c_adapter->cfg->regs, I2C_IT_BUF, DISABLE);

	// setup handling of this byte
	I2C_GenerateSTOP(i2c_adapter->cfg->regs, ENABLE);

	/* Move to the next transaction */
	i2c_adapter->active_txn++;
}

void PIOS_I2C_EV_IRQ_Handler(pios_i2c_t i2c_adapter)
{
	PIOS_IRQ_Prologue();

	PIOS_Assert(PIOS_I2C_validate(i2c_adapter) == true);

	bool woken = false;

	// reading the event clears the interrupt flags
	uint32_t event = I2C_GetLastEvent(i2c_adapter->cfg->regs);

	I2C_DIAG(i2c_adapter, evirq, event);

	switch (event) {
	case I2C_EVENT_MASTER_MODE_SELECT:	/* EV5 */
		switch (i2c_adapter->active_txn->rw) {
		case PIOS_I2C_TXN_READ:
			if (i2c_adapter->active_txn == i2c_adapter->last_txn) {
				/* Final transaction */
				i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_R_LAST_TXN_STARTED, &woken);
			} else {
				/* More transactions follow */
				i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_R_MORE_TXN_STARTED, &woken);
			}
			break;
		case PIOS_I2C_TXN_WRITE:
			if (i2c_adapter->active_txn == i2c_adapter->last_txn) {
				/* Final transaction */
				i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_W_LAST_TXN_STARTED, &woken);
			} else {
				/* More transactions follow */
				i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_W_MORE_TXN_STARTED, &woken);
			}
			break;
		}
		break;
	case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:	/* EV6 */
	case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED:	/* EV6 */
		switch (i2c_adapter->last_byte - i2c_adapter->active_byte + 1) {
		case 0:
			i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_ADDR_SENT_LEN_EQ_0, &woken);
			break;
		case 1:
			i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_ADDR_SENT_LEN_EQ_1, &woken);
			break;
		case 2:
			i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_ADDR_SENT_LEN_EQ_2, &woken);
			break;
		default:
			i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_ADDR_SENT_LEN_GT_2, &woken);
			break;
		}
		break;
	case I2C_EVENT_MASTER_BYTE_RECEIVED:	/* EV7 */
	case I2C_EVENT_MASTER_BYTE_TRANSMITTED:	/* EV8_2 */
		switch (i2c_adapter->last_byte - i2c_adapter->active_byte + 1) {
		case 0:
			i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_TRANSFER_DONE_LEN_EQ_0, &woken);
			break;
		case 1:
			i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_TRANSFER_DONE_LEN_EQ_1, &woken);
			break;
		case 2:
			i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_TRANSFER_DONE_LEN_EQ_2, &woken);
			break;
		default:
			i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_TRANSFER_DONE_LEN_GT_2, &woken);
			break;
		}
		break;
	case I2C_EVENT_MASTER_BYTE_TRANSMITTING:	/* EV8 */
		// This event is being ignored. It may be used to speed up transfer by reloading data buffer earlier.
		break;
	default:
		i2c_adapter_log_fault(i2c_adapter, PIOS_I2C_ERROR_EVENT);

		i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_BUS_ERROR, &woken);
		break;
	}

	PIOS_IRQ_Epilogue();
}

void PIOS_I2C_ER_IRQ_Handler(pios_i2c_t i2c_adapter)
{
	PIOS_IRQ_Prologue();

	bool valid = PIOS_I2C_validate(i2c_adapter);
	PIOS_Assert(valid);

	bool woken = false;

	uint32_t event = I2C_GetLastEvent(i2c_adapter->cfg->regs);

	// clear all flags
	I2C_ClearFlag(i2c_adapter->cfg->regs,
		I2C_FLAG_SMBALERT |
		I2C_FLAG_TIMEOUT |
		I2C_FLAG_PECERR |
		I2C_FLAG_OVR |
		I2C_FLAG_AF |
		I2C_FLAG_ARLO |
		I2C_FLAG_BERR);

	I2C_DIAG(i2c_adapter, erirq, event);

	// explicitly handle NACK
	if (event & I2C_FLAG_AF) {
		I2C_DIAG_INCR(i2c_adapter, i2c_nack_counter);
		i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_NACK, &woken);
	} else { /* Mostly bus errors here */
		i2c_adapter_log_fault(i2c_adapter, PIOS_I2C_ERROR_INTERRUPT);

		/* Fail hard on any errors for now */
		i2c_adapter_inject_event(i2c_adapter, I2C_EVENT_BUS_ERROR, &woken);
	}

	PIOS_IRQ_Epilogue();
}

#else /* STM32F4xx */
#error NoImplForArch
#endif

#endif

/**
  * @}
  * @}
  */
