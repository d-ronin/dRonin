/**
 ******************************************************************************
 * @file       loadable.c
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup Modules Modules
 * @{
 * @brief Implement simple interface for loadable modules.
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */


#include "openpilot.h"
#include <stdbool.h>
#include "pios_thread.h"

#include "loadable_extension.h"

#include "modulesettings.h"

#define MOD_RAM_BEGIN    0x10000000
#define MOD_RODATA_BEGIN 0x20000000
#define MOD_CALLS_BEGIN  0xdf000000

#define CALL_DO_DUMB_TEST_DELAY 0xdf000001
#define CALL_PIOS_THREAD_CREATE 0xdf000003
#define CALL_PIOS_THREAD_SLEEP  0xdf000005
#define CALL_DO_DUMB_REGTASK    0xdf000007

static bool module_enabled;

static int32_t loadable_initialize(void)
{
#ifdef MODULE_Loadable_BUILTIN
	module_enabled = true;
#else
	uint8_t module_state[MODULESETTINGS_ADMINSTATE_NUMELEM];
	ModuleSettingsAdminStateGet(module_state);
	if (module_state[MODULESETTINGS_ADMINSTATE_LOADABLE] == MODULESETTINGS_ADMINSTATE_ENABLED) {
		module_enabled = true;
	} else {
		module_enabled = false;
	}
#endif

	return 0;
}

static uint32_t construct_trampoline(struct loadable_extension *ext,
		void *data_seg, uint32_t got_addr)
{
	/* FLASH relative offset */

	uint32_t tramp_len;
	void *tramp_template;

	/* This wraps the trampoline code in a shim that gives us the
	 * length of the code and the address of it.
	 */
	asm volatile(
		".align 2\n\t"
		"ldr %0, tramplen\n\t"
		"adr %1, trampbegin\n\t"
		"b trampend\n\t"
		"tramplen: .word trampend-trampbegin\n\t"
		/* Next-- the actual trampoline code */
		"trampbegin:\n\t"
		"push { r9, lr }\n\t"      // Preserve ret addr, and r9 just in case
		"ldr ip, trampdest\n\t"    // Put the trampoline dest into r12
		"ldr r9, trampdata\n\t"    // Set up data-seg-pointer
		"blx ip\n\t"               // Branch via IP, with linkage
		"pop { r9, pc }\n\t"       // Restore R9 and return via saved lr
		".align 2\n\t"
		"trampdest: .word 123\n\t" /* Fill in tramp destination here */
		"trampdata: .word 456\n\t" /* Fill in tramp data seg here */
		"trampend:\n\t"
		: "=r" (tramp_len), "=r" (tramp_template));

	uint32_t *tramp_alloc = PIOS_malloc(tramp_len);

	if (!tramp_alloc) {
		return 0;
	}

	memcpy(tramp_alloc, tramp_template, tramp_len);

	tramp_alloc[(tramp_len / 4) - 2] = ((uintptr_t) ext) + got_addr;
	tramp_alloc[(tramp_len / 4) - 1] = (uintptr_t) data_seg;

	return ((uint32_t) tramp_alloc) + 1;
}

static void *layout_extension_ram(struct loadable_extension *ext)
{
	char *new_seg;

	/* Allocate a data segment, and copy the things that matter
	 * from it.
	 */
	new_seg = PIOS_malloc(ext->ram_seg_len);
	if (!new_seg) {
		return NULL;
	}

	memcpy(new_seg,
			(void *) (((uintptr_t) ext) + ext->ram_seg_copyoff),
			ext->ram_seg_copylen);

	/* Zero the rest of things */
	memset(new_seg + ext->ram_seg_copylen, 0,
			ext->ram_seg_len - ext->ram_seg_copylen);

	/* process relocations */
	uint32_t *got = (uint32_t *) new_seg;

	for (int i = 0; i < (ext->ram_seg_gotlen / sizeof(uint32_t)); i++) {
		if (got[i] >= MOD_CALLS_BEGIN) {
			/* df... These shouldn't really show up in
			 * GOT, but ignore them if they do
			 */
		} else if (got[i] >= MOD_RODATA_BEGIN) {
			/* RODATA.  Convert to a flash memory address */

			got[i] = ((uint32_t) ext) + got[i] - MOD_RODATA_BEGIN;
		} else if (got[i] >= MOD_RAM_BEGIN) {
			/* RAM-resident relative offset */
			got[i] = (uint32_t) (new_seg + got[i] - MOD_RAM_BEGIN - ext->ram_seg_copyoff);
		} else if (got[i]) {
			/* Code address, presumably used in callback.  It
			 * needs a trampoline that ensures that R9 is set.
			 */
			got[i] = construct_trampoline(ext, new_seg, got[i]);

			if (!got[i]) {
				return NULL;
			}
		}
	}

	return new_seg;
}

static void invoke_one_loadable(struct loadable_extension *ext)
{
	/* XXX check payload CRC. */

	register void *data_seg asm("r9") = layout_extension_ram(ext);

	if (!data_seg) {
		return;
	}

	void (*entry)() = (void *) (((uintptr_t) ext) + ext->entry_offset);

	asm volatile(
		"blx %0\n\t"
		: /* No output operands */
		: "r"(entry), "r"(data_seg) // Expects data seg in r9
		: "memory",		    // callback may clobber memory,
		"cc",			    // condition codes,
		"r0", "r1", "r2", "r3", "r12",
		// And call-clobbered floating point registers
		"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		"s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15",
		"lr"  		            // we clobber lr
		);
}

static void invoke_loadables()
{
	uintptr_t part_id;

	if (PIOS_FLASH_find_partition_id(FLASH_PARTITION_LABEL_LOADABLE_EXTENSION,
				&part_id)) {
		/* No loadable partition */
		return;
	}

	uint32_t part_len;
	uint8_t *part_beginning;

	part_beginning = PIOS_FLASH_get_address(part_id, &part_len);

	if (!part_beginning) {
		return;
	}

	uint32_t part_pos = 0;

	while ((part_pos + sizeof(struct loadable_extension)) < part_len) {
		struct loadable_extension *ext =
			(void *) (part_beginning + part_pos);

		/* Validate header before trying to go further */

		if (ext->magic != LOADABLE_EXTENSION_MAGIC) {
			return;
		}

		if (ext->length < sizeof(struct loadable_extension)) {
			return;
		}

		if (ext->require_version == LOADABLE_REQUIRE_VERSION_INVALID) {
			return;
		}

		if (ext->entry_offset >= ext->length) {
			return;
		}

		if (ext->entry_offset < sizeof(struct loadable_extension)) {
			return;
		}

		/* XXX test the data seg related things for sanity */

		/* XXX alignment requirements */

		/* XXX check CRC */

		invoke_one_loadable(ext);

		part_pos += ext->length;
	}
}

static int32_t loadable_start(void)
{
	if (!module_enabled) {
		return 0;
	}

	invoke_loadables();

	return 0;
}


MODULE_INITCALL(loadable_initialize, loadable_start)

static void do_dumb_regtask(struct pios_thread *h)
{
        TaskMonitorAdd(TASKINFO_RUNNING_LOADABLE, h);
}

static void do_dumb_test_delay(int cycles) {
	for (volatile int i = 0; i < cycles; i++);
}

#define EXCEPTION_STACK_PC_OFFSET 6

/* This would be better as an enum, but C standard specifies enum type is
 * signed...  Also all of these should be odd, to specify thumbiness.
 */

void MemManageHandler_C(uint32_t *exception_stack) {
	uintptr_t pc = exception_stack[EXCEPTION_STACK_PC_OFFSET];

	switch (pc) {
		case CALL_DO_DUMB_TEST_DELAY-1:
		case CALL_DO_DUMB_TEST_DELAY:
			pc = (uint32_t) do_dumb_test_delay;
			break;
		case CALL_PIOS_THREAD_CREATE-1:
		case CALL_PIOS_THREAD_CREATE:
			pc = (uint32_t) PIOS_Thread_Create;
			break;
		case CALL_PIOS_THREAD_SLEEP-1:
		case CALL_PIOS_THREAD_SLEEP:
			pc = (uint32_t) PIOS_Thread_Sleep;
			break;
		case CALL_DO_DUMB_REGTASK-1:
		case CALL_DO_DUMB_REGTASK:
			pc = (uint32_t) do_dumb_regtask;
			break;
		default:
			while (1);
	}

	exception_stack[EXCEPTION_STACK_PC_OFFSET] = pc;
}

void MemManageVector(void) __attribute__((naked));

void MemManageVector(void)
{
	/* Stores the exception stack pointer into an argument, and invokes
	 * the real handler (without linkage-- the BX LR at completion of
	 * MemManageHandler will in turn return from the exception.
	 */

	asm volatile(
		/* Inspect the LSB of the link register, to determine whether
		 * we are using PSP or MSP.
		 */
		"ldr r1, =stack_sel_mask\n\t"
		"tst r1, lr\n\t"
		"bne using_psp\n\t"	/* if bit unset */
		"mrs r0, MSP\n\t"
		"b MemManageHandler_C\n\t"
		"using_psp: mrs r0, PSP\n\t"
		"b MemManageHandler_C\n\t"
		"stack_sel_mask: .word 0x00000001\n\t"
		: );
}

/** 
  * @}
  */
