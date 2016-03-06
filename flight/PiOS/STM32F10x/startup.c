/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 *
 * @file       startup.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @brief      C based startup of F4
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */


#include <string.h>
#include <stm32f10x.h>

/* prototype for main() that tells us not to worry about it possibly returning */
extern int		main(void) __attribute__((noreturn));

/* prototype our _main() to avoid prolog/epilog insertion and other related junk */
void 			_main(void) __attribute__((noreturn, naked, no_instrument_function));

/* stack_change() to reclaim init stack for heap */
void 			stack_change(void) __attribute__((noreturn, naked, no_instrument_function));

/** default handler for CPU exceptions */
static void		default_cpu_handler(void) __attribute__((noreturn, naked, no_instrument_function));

extern void 	PIOS_heap_increase_size(size_t bytes);

/** BSS symbols XXX should have a header that defines all of these */
extern char		_sbss, _ebss;

/** DATA symbols XXX should have a header that defines all of these */
extern char		_sidata, _sdata, _edata;

/** The IRQ and init stacks */
char			init_stack[INIT_STACK_SIZE] __attribute__((section(".initstack")));
char			irq_stack[IRQ_STACK_SIZE] __attribute__((section(".irqstack")));

/** exception handler */
typedef const void	(vector)(void);

/** CortexM3 CPU vectors */
struct cm3_vectors {
	void	*initial_stack;
	vector	*entry;
	vector	*vectors[14];
};

/**
 * Initial startup code.
 */
void
_main(void)
{
	// load the stack base for the current stack before we attempt to branch to any function
	// that might bounds-check the stack
	asm volatile ("mov r10, %0" : : "r" (&irq_stack[0]) : );

	/* enable usage, bus and memory faults */
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;

	/* copy initialised data from flash to RAM */
	memcpy(&_sdata, &_sidata, &_edata - &_sdata);

	/* zero the BSS */
	memset(&_sbss, 0, &_ebss - &_sbss);

	// Jump to ST bootloader if BL is requested on Naze32
#ifdef USE_STM32103CB_Naze32
	// enable power and backup clocks
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP | RCC_APB1Periph_PWR, ENABLE);
	// enable backup register access
	PWR_BackupAccessCmd(ENABLE);
	// get and clear magic word 1
	uint16_t mw1 = BKP_ReadBackupRegister(BKP_DR1);
	BKP_WriteBackupRegister(BKP_DR1, 0);
	// get and clear magic word 2
	uint16_t mw2 = BKP_ReadBackupRegister(BKP_DR2);
	BKP_WriteBackupRegister(BKP_DR2, 0);
	// reset periphs
	PWR_BackupAccessCmd(DISABLE);
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_BKP | RCC_APB1Periph_PWR, ENABLE);
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_BKP | RCC_APB1Periph_PWR, DISABLE);
	// check magic words
	if (mw1 == 0x1122 && mw2 == 0xAA55) {
		// we're going to bootloader, enable LEDs. don't reset these clocks or the LEDs go off
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);
		// remap JTAG pins to GPIO
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
		// set LEDs as low outputs (on)
		GPIOB->CRL = (uint32_t)(0x77 << 12);
		GPIOB->BRR = (uint16_t)(GPIO_Pin_3 | GPIO_Pin_4);
		// jump to ST bootloader*/
		__set_MSP(0x1FFFF000ul);
		// jump an extra one byte for thumb mode
		((void (*)(void))(0x1FFFF005ul))();
	}
#endif // USE_STM32103CB_Naze32

	/* fill most of the IRQ stack with a watermark pattern so we can measure how much is used */
	for (int i = 0; i < ((sizeof(irq_stack) - 64) / 4); i++)
		((uint32_t *)irq_stack)[i] = 0x0000a5a5;

	/* call main */
	(void)main();
}

/**
 * Change from Init stack to IRQ stack, reclaim Init stack into heap
 */
void stack_change(void)
{
	register long lr asm("lr");
	uint32_t ret = lr;
	__set_MSP((uint32_t)&irq_stack[sizeof(irq_stack)]);
	PIOS_heap_increase_size(sizeof(init_stack));
	((void (*)(void))ret)();
}

/**
 * Default handler for CPU exceptions.
 */
void **HARDFAULT_PSP;
register void *stack_pointer asm("sp");


static void
default_cpu_handler(void)
{
    // Hijack the process stack pointer to make backtrace work
    asm("mrs %0, psp" : "=r"(HARDFAULT_PSP) : :);
    stack_pointer = HARDFAULT_PSP;
    while(1);
}

/** Prototype for optional exception vector handlers */
#define HANDLER(_name)	extern vector _name __attribute__((weak, alias("default_cpu_handler")))

/* standard CMSIS vector names */
HANDLER(NMI_Handler);
HANDLER(HardFault_Handler);
HANDLER(MemManage_Handler);
HANDLER(BusFault_Handler);
HANDLER(UsageFault_Handler);
HANDLER(DebugMon_Handler);

/* these vectors point directly to the relevant FreeRTOS functions if they are defined */
HANDLER(vPortSVCHandler);
HANDLER(xPortPendSVHandler);
HANDLER(xPortSysTickHandler);

/** CortexM3 vector table */
struct cm3_vectors cpu_vectors __attribute((section(".cpu_vectors"))) = {
		.initial_stack = &init_stack[sizeof(init_stack)],
		.entry = (vector *)_main,
		.vectors = {
				NMI_Handler,
				HardFault_Handler,
				MemManage_Handler,
				BusFault_Handler,
				UsageFault_Handler,
				0,
				0,
				0,
				0,
				vPortSVCHandler,
				DebugMon_Handler,
				0,
				xPortPendSVHandler,
				xPortSysTickHandler,
		}
};

/**
 * @}
 */
