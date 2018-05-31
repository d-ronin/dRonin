#include <stdint.h>

struct pios_flash_posix_cfg {
	uint32_t size_of_flash;
	uint32_t size_of_sector;
};

int32_t PIOS_Flash_Posix_Init(uintptr_t * chip_id,
		const struct pios_flash_posix_cfg * cfg,
		bool force_recreate);
void PIOS_Flash_Posix_Destroy(uintptr_t chip_id);
void PIOS_Flash_Posix_SetFName(const char *name);

extern const struct pios_flash_driver pios_posix_flash_driver;
