/* Section Definitions */
SECTIONS
{
    .header : AT(0)
    {
	*(.extension_header)
    }

    /*
     * Vectors, code and constant data.
     */
    .text :
    {
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7t) *(.glue_7)
    }

    _etext = .;

    _sidata = .;
    . = . + 0x10000000;

    _got_begin = .;

    .got : AT(ADDR(.got) & 0xffffff)
    {
        . = ALIGN(4);
	*(.igot.plt)
	*(.got .got.*)
        . = ALIGN(4);
    }

    _egot = .;

    _gotlen = _egot - _got_begin;

    /*
     * Non-const initialised data.
     */
    .data : AT(ADDR(.data) & 0xffffff)
    {
        . = ALIGN(4);
        *(.data .data.*)
        . = ALIGN(4);
    }

    _copylen = _gotlen + SIZEOF(.data);

    /*
     * Uninitialised data (BSS + commons).
     */
    .bss (NOLOAD) :
    {
        _sbss = . ;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        _ebss = . ;
    }

    _ramlen = _copylen + SIZEOF(.bss);

    . = ADDR(.data) + SIZEOF(.data);

    . += 0x10000000;

    .rodata : AT(ADDR(.rodata) & 0xffffff)
    {
        . = ALIGN(4);
        *(.rodata .rodata* .gnu.linkonce.r.*)
	. = ALIGN(4);
    }

    _eidata = . & 0xffffff;

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

    dumb_test_delay              = 0xdf000001;
    PIOS_Thread_Create           = 0xdf000003;
    PIOS_Thread_Sleep            = 0xdf000005;
    dumb_regtask                 = 0xdf000007;
    system_annunc_custom_string  = 0xdf000009;
}
