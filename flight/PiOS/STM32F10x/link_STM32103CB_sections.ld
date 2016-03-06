/* This is the size of the stack for all FreeRTOS IRQs */
_irq_stack_size = 0x240;
/* This is the size of the stack for early init: life span is until scheduler starts */
_init_stack_size = 0x200;
/* This is the size of the pre-rtos heap */
_init_heap_size = 53 * 256;

/* Stub out these functions since we don't use them anyway */
PROVIDE (vPortSVCHandler = 0) ;
PROVIDE (xPortPendSVHandler = 0) ;
PROVIDE (xPortSysTickHandler = 0) ;

/* Section Definitions */
SECTIONS
{
    .text :
    {
        PROVIDE (pios_isr_vector_table_base = .);
        KEEP(*(.cpu_vectors))    /* CPU exception vectors */
        KEEP(*(.io_vectors))    /* I/O interrupt vectors */
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7t) *(.glue_7)
        *(.rodata .rodata* .gnu.linkonce.r.*)
    } > FLASH

    /* module sections */
    .initcallmodule.init :
    {
        . = ALIGN(4);
        __module_initcall_start = .;
        KEEP(*(.initcallmodule.init))
        . = ALIGN(4);
        __module_initcall_end   = .;
    } >FLASH

    /* 
     * _exti section
     */
    _exti :
    {
        . = ALIGN(4);
        __start__exti = .;
        KEEP(*(_exti))
        . = ALIGN(4);
        __stop__exti   = .;
    } >FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH

    .boardinfo :
    {
        PROVIDE(pios_board_info_blob = .);
        . = ALIGN(4);
        KEEP(*(.boardinfo))
        . = ALIGN(4);
    } > FLASH	

    /*
     * Markers for the end of the 'text' section and the in-flash start of 
     * non-constant data 
     */
    . = ALIGN(4);
    _etext = .;
    _sidata = .;

    /*
        * Place the IRQ/bootstrap stack at the bottom of SRAM so that an overflow
        * results in a hard fault.
        */
    .istack (NOLOAD) :
    {
       . = ALIGN(4);
        _irq_stack_end = . ;
        *(.irqstack)
        _irq_stack_top = . ;
    } > SRAM

    .data : AT (_etext)
    {
        _sdata = .;
        *(.data .data.*)
        . = ALIGN(4);
        _edata = . ;
    } > SRAM

    /*
     * Uninitialised data (BSS + commons).
     */
    .bss (NOLOAD) :
    {
        _sbss = . ;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        _ebss = . ;
        PROVIDE ( _end = _ebss ) ;
    } > SRAM

    /*
     * The heap consumes all of the SRAM up to init stack.
     */
    .heap (NOLOAD) :
    {
        . = ALIGN(4);
        _sheap = . ;
        
        /*
         * This allows us to declare an object or objects up to the minimum acceptable
         * heap size and receive a linker error if the space available for the heap is
         * not sufficient.
         */
        *(.heap)
        
        /* extend the heap up to the bottom of init stack */
        . = ORIGIN(SRAM) + LENGTH(SRAM) - (ABSOLUTE(_init_stack_top) - ABSOLUTE(_init_stack_end));
        _eheap = .;
    } > SRAM

    /*
        * Place the Init stack at the bottom of SRAM so that an overflow
        * results in a hard fault.
        */
    .inistack (NOLOAD) :
    {
       . = ALIGN(4);
        _init_stack_end = . ;
        *(.initstack)
        _init_stack_top = . ;
    } > SRAM

    _eram = ORIGIN(SRAM) + LENGTH(SRAM) ;
 
    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}
