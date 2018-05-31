#include <stdlib.h>		/* abort */
#include <stdio.h>		/* fopen/fread/fwrite/fseek */
#include <assert.h>		/* assert */
#include <string.h>		/* memset */

#include <stdbool.h>
#include "pios_heap.h"
#include "pios_flash_posix_priv.h"

#include <pios_semaphore.h>

enum flash_posix_magic {
	FLASH_POSIX_MAGIC = 0x321dabc1,
};

struct flash_posix_dev {
	enum flash_posix_magic magic;
	const struct pios_flash_posix_cfg * cfg;
	FILE * flash_file;

	struct pios_semaphore *transaction_lock;
};

static struct flash_posix_dev * PIOS_Flash_Posix_Alloc(void)
{
	struct flash_posix_dev * flash_dev = PIOS_malloc(sizeof(struct flash_posix_dev));

	flash_dev->magic = FLASH_POSIX_MAGIC;

	return flash_dev;
}

static const char *pios_flash_file_name = "theflash.bin";

static const char *PIOS_Flash_Posix_GetFName()
{
	return pios_flash_file_name;
}

void PIOS_Flash_Posix_SetFName(const char *name)
{
	pios_flash_file_name = name;
}

int32_t PIOS_Flash_Posix_Init(uintptr_t * chip_id,
		const struct pios_flash_posix_cfg * cfg,
		bool force_recreate)
{
	/* Check inputs */
	assert(chip_id);
	assert(cfg);
	assert(cfg->size_of_flash);
	assert(cfg->size_of_sector);
	assert((cfg->size_of_flash % cfg->size_of_sector) == 0);

	struct flash_posix_dev * flash_dev = PIOS_Flash_Posix_Alloc();
	assert(flash_dev);

	flash_dev->cfg = cfg;

	if (!force_recreate) {
		flash_dev->flash_file = fopen(PIOS_Flash_Posix_GetFName(), "r+");
	} else {
		flash_dev->flash_file = NULL;
	}

	if (flash_dev->flash_file == NULL) {
		flash_dev->flash_file = fopen(PIOS_Flash_Posix_GetFName(), "w+");
		if (!flash_dev->flash_file) {
			perror("fopen flash");
			return -1;
		}

		uint8_t sector[cfg->size_of_sector];
		memset(sector, 0xFF, sizeof(sector));
		for (uint32_t i = 0;
				i < cfg->size_of_flash / cfg->size_of_sector;
				i++) {
			fwrite(sector, sizeof(sector), 1, flash_dev->flash_file);
		}
	}

	fseek(flash_dev->flash_file, 0, SEEK_END);
	if (ftell(flash_dev->flash_file) != flash_dev->cfg->size_of_flash) {
		fclose(flash_dev->flash_file);
		return -2;
	}

	flash_dev->transaction_lock = PIOS_Semaphore_Create();

	*chip_id = (uintptr_t)flash_dev;

	return 0;
}

void PIOS_Flash_Posix_Destroy(uintptr_t chip_id)
{
	struct flash_posix_dev * flash_dev = (struct flash_posix_dev *)chip_id;

	fclose(flash_dev->flash_file);

	PIOS_free(flash_dev);
}

/**********************************
 *
 * Provide a PIOS flash driver API
 *
 *********************************/
#include "pios_flash_priv.h"

static int32_t PIOS_Flash_Posix_StartTransaction(uintptr_t chip_id)
{
	struct flash_posix_dev * flash_dev = (struct flash_posix_dev *)chip_id;

	if (PIOS_Semaphore_Take(flash_dev->transaction_lock,
				PIOS_SEMAPHORE_TIMEOUT_MAX) != true) {
		return -2;
	}

	return 0;
}

static int32_t PIOS_Flash_Posix_EndTransaction(uintptr_t chip_id)
{
	struct flash_posix_dev * flash_dev = (struct flash_posix_dev *)chip_id;

	if (PIOS_Semaphore_Give(flash_dev->transaction_lock) != true) {
		assert(false);
	}

	return 0;
}

static int32_t PIOS_Flash_Posix_EraseSector(uintptr_t chip_id, uint32_t chip_sector, uint32_t chip_offset)
{
	struct flash_posix_dev * flash_dev = (struct flash_posix_dev *)chip_id;

	/* assert(flash_dev->transaction_in_progress); */

	if (fseek (flash_dev->flash_file, chip_offset, SEEK_SET) != 0) {
		assert(0);
	}

	unsigned char buf[flash_dev->cfg->size_of_sector];

	memset((void *)buf, 0xFF, flash_dev->cfg->size_of_sector);

	size_t s;
	s = fwrite (buf, 1, flash_dev->cfg->size_of_sector, flash_dev->flash_file);

	assert (s == flash_dev->cfg->size_of_sector);

	fflush(flash_dev->flash_file);

	return 0;
}

static int32_t PIOS_Flash_Posix_WriteData(uintptr_t chip_id, uint32_t chip_offset, const uint8_t * data, uint16_t len)
{
	/* Check inputs */
	assert(data);

	struct flash_posix_dev * flash_dev = (struct flash_posix_dev *)chip_id;

	/* assert(flash_dev->transaction_in_progress); */

	if (fseek (flash_dev->flash_file, chip_offset, SEEK_SET) != 0) {
		assert(0);
	}

	size_t s;
	s = fwrite (data, 1, len, flash_dev->flash_file);

	assert (s == len);

	fflush(flash_dev->flash_file);

	return 0;
}

static int32_t PIOS_Flash_Posix_ReadData(uintptr_t chip_id, uint32_t chip_offset, uint8_t * data, uint16_t len)
{
	/* Check inputs */
	assert(data);

	struct flash_posix_dev * flash_dev = (struct flash_posix_dev *)chip_id;

	/* assert(flash_dev->transaction_in_progress); */

	if (fseek (flash_dev->flash_file, chip_offset, SEEK_SET) != 0) {
		assert(0);
	}

	size_t s;
	s = fread (data, 1, len, flash_dev->flash_file);

	assert (s == len);

	return 0;
}

/* Provide a flash driver to external drivers */
const struct pios_flash_driver pios_posix_flash_driver = {
	.start_transaction = PIOS_Flash_Posix_StartTransaction,
	.end_transaction   = PIOS_Flash_Posix_EndTransaction,
	.erase_sector      = PIOS_Flash_Posix_EraseSector,
	.write_data        = PIOS_Flash_Posix_WriteData,
	.read_data         = PIOS_Flash_Posix_ReadData,
};

