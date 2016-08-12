/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SYS System Functions
 * @brief PIOS System Initialization code
 * @{
 *
 * @file       pios_sys.c  
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * 	        Parts by Thorsten Klose (tk@midibox.org) (tk@midibox.org)
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @brief      Sets up basic STM32 system hardware, functions are called from Main.
 * @see        The GNU Public License (GPL) Version 3
 *
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
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */



/* Project Includes */
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif /* !defined(_GNU_SOURCE) */

#include <unistd.h>

#if !(defined(_WIN32) || defined(WIN32) || defined(__MINGW32__))
#ifndef __APPLE__
#include <sys/mman.h>
#include <sched.h>
#endif
#endif

#include "pios.h"

#include "pios_fileout_priv.h"
#include "pios_com_priv.h"

#include "pios_spi_posix_priv.h"
#include "pios_ms5611_priv.h"
#include "pios_bmx055_priv.h"


#if defined(PIOS_INCLUDE_SYS)
static bool debug_fpe=false;

#define MAX_SPI_BUSES 16
int num_spi = 0;
uintptr_t spi_devs[16];

static void Usage(char *cmdName) {
	printf( "usage: %s [-f] [-r] [-l logfile] [-s spibase] [-d drvname:bus:id]\n"
		"\n"
		"\t-f\tEnables floating point exception trapping mode\n"
		"\t-r\tGoes realtime-class and pins all memory (requires root)\n"
		"\t-l log\tWrites simulation data to a log\n"
#ifdef PIOS_INCLUDE_SPI
		"\t-s spibase\tConfigures a SPI interface on the base path\n"
		"\t-d drvname:bus:id\tStarts driver drvname on bus/id\n"
		"\t\t\tAvailable drivers: ms5611_spi\n"
#endif
		"",
		cmdName);

	exit(1);
}

#ifdef PIOS_INCLUDE_SPI
static int handle_device(const char *optarg) {
	char arg_copy[128];

	strncpy(arg_copy, optarg, sizeof(arg_copy));
	arg_copy[sizeof(arg_copy)-1] = 0;

	char *saveptr;

	char *drv_name = strtok_r(arg_copy, ":", &saveptr);
	if (drv_name == NULL) goto fail;

	char *bus_num_str = strtok_r(NULL, ":", &saveptr);
	if (bus_num_str == NULL) goto fail;

	char *dev_num_str = strtok_r(NULL, ":", &saveptr);
	if (dev_num_str == NULL) goto fail;

	int bus_num = atoi(bus_num_str);
	if ((bus_num < 0) || (bus_num >= num_spi)) {
		goto fail;
	}

	int dev_num = atoi(dev_num_str);
	if (dev_num < 0) {
		goto fail;
	}

	if (!strcmp(drv_name, "ms5611_spi")) {
		struct pios_ms5611_cfg *ms5611_cfg;

		ms5611_cfg = PIOS_malloc(sizeof(*ms5611_cfg));

		ms5611_cfg->oversampling = MS5611_OSR_512;
		ms5611_cfg->temperature_interleaving = 1;

		int ret = PIOS_MS5611_SPI_Init(spi_devs[bus_num], dev_num, ms5611_cfg);

		if (ret) goto fail;
	} else if (!strcmp(drv_name, "bmx055")) {
		struct pios_bmx055_cfg *bmx055_cfg;
		pios_bmx055_dev_t dev;

		bmx055_cfg = PIOS_malloc(sizeof(*bmx055_cfg));

		int ret = PIOS_BMX055_SPI_Init(&dev, spi_devs[bus_num], dev_num, dev_num+1, dev_num+2, bmx055_cfg);

		if (ret) goto fail;
	}

	return 0;
fail:
	return -1;
}
#endif

static void go_realtime() {
#ifdef __linux__
	/* First, pin all our memory.  We don't want stuff we need
	 * to get faulted out. */
	int rc = mlockall(MCL_CURRENT | MCL_FUTURE);

	if (rc) {
		perror("mlockall");
		exit(1);
	}

	/* We always run on the same processor-- why migrate when
	 * you never yield?
	 */

	cpu_set_t allowable_cpus;

	CPU_ZERO(&allowable_cpus);

	CPU_SET(0, &allowable_cpus);

	rc = sched_setaffinity(0, CPU_SETSIZE, &allowable_cpus);

	if (rc) {
		perror("sched_setaffinity");
		exit(1);
	}

	/* Next, let's go hard realtime. */

	struct sched_param sch_p = {
		.sched_priority = 50
	};

	rc = sched_setscheduler(0, SCHED_RR, &sch_p);

	if (rc) {
		perror("sched_setscheduler");
		exit(1);
	}
#else
	printf("Only can do realtime stuff on Linux\n");
	exit(1);
#endif
}

void PIOS_SYS_Args(int argc, char *argv[]) {
	int opt;

	while ((opt = getopt(argc, argv, "frl:s:d:")) != -1) {
		switch (opt) {
			case 'f':
				debug_fpe = true;
				break;
			case 'r':
				go_realtime();
				break;
			case 'l':
			{
				uintptr_t tmp;
				if (PIOS_FILEOUT_Init(&tmp,
							optarg, "w")) {
					printf("Couldn't open logfile %s\n",
							optarg);
					exit(1);
				}

				if (PIOS_COM_Init(&PIOS_COM_OPENLOG,
							&pios_fileout_com_driver,
							tmp,
							0,
							PIOS_FILEOUT_TX_BUFFER_SIZE)) {
					printf("Couldn't init fileout com layer\n");
					exit(1);
				}
				break;
			}
#ifdef PIOS_INCLUDE_SPI
			case 'd':
				if (handle_device(optarg)) {
					printf("Couldn't init device\n");
					exit(1);
				}
				break;
			case 's':
			{
				struct pios_spi_cfg *spi_cfg;

				spi_cfg = PIOS_malloc(sizeof(*spi_cfg));

				strncpy(spi_cfg->base_path, optarg,
					sizeof(spi_cfg->base_path));

				spi_cfg->base_path[sizeof(spi_cfg->base_path)-1] = 0;

				int ret = PIOS_SPI_Init(spi_devs + num_spi,
					spi_cfg);

				if (ret < 0) {
					printf("Couldn't init SPI\n");
					exit(1);
				}

				num_spi++;
				break;
			}
#endif
				
			default:
				Usage(argv[0]);
				break;
		}
	}

	if (optind < argc) {
		Usage(argv[0]);
	}
}

/**
* Initialises all system peripherals
*/
#include <assert.h>		/* assert */
#include <stdlib.h>		/* printf */
#include <signal.h>		/* sigaction */
#include <fenv.h>		/* PE_* */

#if !(defined(_WIN32) || defined(WIN32) || defined(__MINGW32__))
static void sigint_handler(int signum, siginfo_t *siginfo, void *ucontext)
{
	printf("\nSIGINT received.  Shutting down\n");
	exit(0);
}

static void sigfpe_handler(int signum, siginfo_t *siginfo, void *ucontext)
{
	printf("\nSIGFPE received.  OMG!  Math Bug!  Run again with gdb to find your mistake.\n");
	exit(0);
}
#endif

void PIOS_SYS_Init(void)
{
#if !(defined(_WIN32) || defined(WIN32) || defined(__MINGW32__))
	struct sigaction sa_int = {
		.sa_sigaction = sigint_handler,
		.sa_flags = SA_SIGINFO,
	};

	int rc = sigaction(SIGINT, &sa_int, NULL);
	assert(rc == 0);

	if (debug_fpe) {
		struct sigaction sa_fpe = {
			.sa_sigaction = sigfpe_handler,
			.sa_flags = SA_SIGINFO,
		};

		rc = sigaction(SIGFPE, &sa_fpe, NULL);
		assert(rc == 0);

		// Underflow is fairly harmless, do we even care in debug
		// mode?
#ifndef __APPLE__
		feenableexcept(FE_DIVBYZERO | FE_UNDERFLOW | FE_OVERFLOW |
			FE_INVALID);
#else
		// XXX need the right magic
		printf("UNABLE TO DBEUG FPE ON OSX!\n");
		exit(1);
#endif
	}
#endif
}

/**
* Shutdown PIOS and reset the microcontroller:<BR>
* <UL>
*   <LI>Disable all RTOS tasks
*   <LI>Disable all interrupts
*   <LI>Turn off all board LEDs
*   <LI>Reset STM32
* </UL>
* \return < 0 if reset failed
*/
int32_t PIOS_SYS_Reset(void)
{
	/* We will never reach this point */
	return -1;
}

/**
* Returns the serial number as a string
* param[out] uint8_t pointer to a string which can store at least 12 bytes
* (12 bytes returned for STM32)
* return < 0 if feature not supported
*/
int32_t PIOS_SYS_SerialNumberGetBinary(uint8_t *array)
{
	/* Stored in the so called "electronic signature" */
	for (int i = 0; i < PIOS_SYS_SERIAL_NUM_BINARY_LEN; ++i) {
		array[i] = 0xff;
	}

	/* No error */
	return 0;
}

/**
* Returns the serial number as a string
* param[out] str pointer to a string which can store at least 32 digits + zero terminator!
* (24 digits returned for STM32)
* return < 0 if feature not supported
*/
int32_t PIOS_SYS_SerialNumberGet(char *str)
{
	/* Stored in the so called "electronic signature" */
	int i;
	for (i = 0; i < PIOS_SYS_SERIAL_NUM_ASCII_LEN; ++i) {
		str[i] = 'F';
	}
	str[i] = '\0';

	/* No error */
	return 0;
}
#endif

/**
  * @}
  * @}
  */
