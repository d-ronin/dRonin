// Startup code and interrupt vector table.
//
// Copyright (c) 2016, dRonin
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <string.h>
#include <strings.h>
#include <stm32f0xx.h>

extern char _sbss, _ebss, _sidata, _sdata, _edata, _stack_top /*, _sfast, _efast */;

typedef const void (vector)(void);

extern int main(void);

void _start(void) __attribute__((noreturn, naked, no_instrument_function));

void _start(void)
{
	/* Copy data segment */
	memcpy(&_sdata, &_sidata, &_edata - &_sdata);

	/* Zero BSS segment */
	bzero(&_sbss, &_ebss - &_sbss);

	/* Invoke main. */
	asm ("bl	main");
	__builtin_unreachable();
}

/* This ends one early, so other code can provide systick handler */
const void *_vectors[] __attribute((section(".vectors"))) = {
	&_stack_top,
	_start,
	0,	/* XXX: NMI_Handler */
	0,	/* XXX: HardFault_Handler */
	0,	/* XXX: MemManage_Handler */
	0,	/* XXX: BusFault_Handler */
	0,	/* XXX: UsageFault_Handler */
	0,	/* 4 reserved */
	0,
	0,
	0,
	0,	/* SVCall */
	0,	/* Reserved for debug */
	0,	/* Reserved */
	0,	/* PendSV */
};
