__main_stack_size__     = 0x0400;
__process_stack_size__  = 0x0400;

MEMORY
{
bd_info (r) : org = 0x08008000 - 0x80, len = 0x000080
flash : org = 0x08000000 + 128k, len = 512k
ram : org = 0x20000000, len = 128k
}
