#ifndef PIOS_SPI_POSIX_PRIV_H
#define PIOS_SPI_POSIX_PRIV_H

#include <pios.h>
#include "pios_semaphore.h"

#include <limits.h>

#define SPI_MAX_SUBDEV 8

struct pios_spi_cfg {
	char base_path[PATH_MAX];
};

#endif /* PIOS_SPI_POSIX_PRIV_H */
