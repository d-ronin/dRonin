#include <pios.h>
#include <pios_board.h>

#include "pios_board_info.h"

#if defined(FLIGHT_POSIX)
const struct pios_board_info pios_board_info_blob = {
#else
const struct pios_board_info __attribute__((__used__)) __attribute__((__section__(".boardinfo"))) pios_board_info_blob = {
#endif
  .magic      = PIOS_BOARD_INFO_BLOB_MAGIC,
  .board_type = BOARD_TYPE,
  .board_rev  = BOARD_REVISION,
#ifdef BOOTLOADER_VERSION
  .bl_rev     = BOOTLOADER_VERSION,
#endif
  .hw_type    = HW_TYPE,
#ifdef FW_BANK_SIZE
  .fw_base    = FW_BANK_BASE,
  .fw_size    = FW_BANK_SIZE - FW_DESC_SIZE,
  .desc_base  = FW_BANK_BASE + FW_BANK_SIZE - FW_DESC_SIZE,
  .desc_size  = FW_DESC_SIZE,
#endif
#ifdef EE_BANK_BASE
  .ee_base    = EE_BANK_BASE,
  .ee_size    = EE_BANK_SIZE,
#endif
};
