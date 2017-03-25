__main_stack_size__     = 0x0400;
__process_stack_size__  = 0x0400;


MEMORY
{
    BL_FLASH (rx) : ORIGIN = 0x08000000,        LENGTH = 0x03000 - 0x00080
    bd_info (r)   : ORIGIN = 0x08003000 - 0x80, LENGTH =           0x00080
    flash (rx)    : ORIGIN = 0x08003000,        LENGTH = 0x20000 - 0x03000 - 0x04000
    EE_FLASH (rw) : ORIGIN = 0x0801C000,        LENGTH = 0x04000
    ram (rwx)    : ORIGIN = 0x20000000,        LENGTH = 0x05000
}
