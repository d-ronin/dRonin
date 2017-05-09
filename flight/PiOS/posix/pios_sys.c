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
 * with this program; if not, see <http://www.gnu.org/licenses/>
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
#include <ctype.h>

#if !(defined(_WIN32) || defined(WIN32) || defined(__MINGW32__))
#ifndef __APPLE__
#include <sys/mman.h>
#include <sched.h>
#endif
#endif

#include "pios.h"

#include "pios_fileout_priv.h"
#include "pios_com_priv.h"
#include "pios_serial_priv.h"
#include "pios_tcp_priv.h"
#include "pios_flightgear.h"
#include "pios_thread.h"

#include "pios_hal.h"
#include "pios_adc_priv.h"
#include "pios_rcvr_priv.h"
#ifdef PIOS_INCLUDE_RTC
#include "pios_rtc_priv.h"
#endif

#include "manualcontrolsettings.h"

#include "sha1.h"

#if defined(PIOS_INCLUDE_SYS)
static bool debug_fpe=false;

#define MAX_SPI_BUSES 16

bool are_realtime = false;

#ifdef PIOS_INCLUDE_SPI
#include <pios_dio.h>

int num_spi = 0;
pios_spi_t spi_devs[16];

dio_tag_t gyro_int = NULL;

#include "pios_spi_posix_priv.h"
#include "pios_ms5611_priv.h"
#include "pios_bmm150_priv.h"
#include "pios_bmx055_priv.h"
#include "pios_flyingpio.h"
#endif

#ifdef PIOS_INCLUDE_I2C
char mag_orientation = 255;

int num_i2c = 0;
pios_i2c_t i2c_devs[16];

pios_i2c_t external_i2c_adapter_id;

#include "pios_px4flow_priv.h"
#include "pios_hmc5883_priv.h"
#endif

int orig_stdout;

static void Usage(char *cmdName) {
	printf( "usage: %s [-f] [-r] [-g num] [-m orientation] [-s spibase] [-d drvname:bus:id]\n"
		"\t\t[-l logfile] [-I i2cdev] [-i drvname:bus] [-g port]"
		"\n"
		"\t-f\tEnables floating point exception trapping mode\n"
		"\t-r\tGoes realtime-class and pins all memory (requires root)\n"
		"\t-l log\tWrites simulation data to a log\n"
		"\t-g port\tStarts FlightGear driver on port\n"
#ifdef PIOS_INCLUDE_SIMSENSORS_YASIM
		"\t-y\tUse an external simulator (drhil yasim)\n"
#endif
		"\t-x time\tExit after time seconds\n"
#ifdef PIOS_INCLUDE_SERIAL
		"\t-S drvname:serialpath\tStarts a serial driver on serialpath\n"
		"\t\t\tAvailable drivers: gps msp lighttelemetry telemetry omnip\n"
#endif
#ifdef PIOS_INCLUDE_SPI
		"\t-g gpionum\tSpecifies the gyro interrupt is on gpionum\n"
		"\t-s spibase\tConfigures a SPI interface on the base path\n"
		"\t-d drvname:bus:id\tStarts driver drvname on bus/id\n"
		"\t\t\tAvailable drivers: bmm150 bmx055 flyingpio ms5611\n"
#endif
#ifdef PIOS_INCLUDE_I2C
		"\t-m orientation\tSets the orientation of an external mag\n"
		"\t-I i2cdev\tConfigures an I2C interface on i2cdev\n"
		"\t-i drvname:bus\tStarts a driver instance on bus\n"
		"\t\t\tAvailable drivers: px4flow hmc5883 hmc5983 bmp280 ms5611\n"
#endif
		"",
		cmdName);

	exit(1);
}

#ifdef PIOS_INCLUDE_SERIAL

#ifdef PIOS_INCLUDE_OMNIP
#include <pios_omnip.h>
#endif

#define SERIAL_BUF_LEN 384
static int handle_serial_device(const char *optarg) {
	char arg_copy[128];

	strncpy(arg_copy, optarg, sizeof(arg_copy));
	arg_copy[sizeof(arg_copy)-1] = 0;

	char *saveptr;

	char *drv_name = strtok_r(arg_copy, ":", &saveptr);
	if (drv_name == NULL) goto fail;

	char *ser_path = strtok_r(NULL, ":", &saveptr);
	if (ser_path == NULL) goto fail;

	// If ser_path is entirely a number.. let's treat this as TCP
	// instead!

	long port;
	char *endptr;

	port = strtol(ser_path, &endptr, 10);

	uintptr_t com_id;
	uintptr_t lower_id;

	const struct pios_com_driver *com_driver;

	if ((port > 0) && (port < 65536) && !(*endptr)) {
		struct pios_tcp_cfg *virtserial_cfg;

		virtserial_cfg = PIOS_malloc(sizeof(*virtserial_cfg));

		virtserial_cfg->ip = "0.0.0.0";
		virtserial_cfg->port = port;

		if (PIOS_TCP_Init(&lower_id, virtserial_cfg)) {
			printf("Can't init PIOS_TCP\n");
			goto fail;
		}

		com_driver = &pios_tcp_com_driver;

	} else if (!strcmp(ser_path, "stdio")) {
		if (PIOS_SERIAL_InitFromFd(&lower_id, STDIN_FILENO,
					orig_stdout, true)) {
			printf("Can't init stdio-serial\n");
			goto fail;
		}

		com_driver = &pios_serial_com_driver;
	} else {
		if (PIOS_SERIAL_Init(&lower_id, ser_path)) {
			printf("Can't init serial\n");
			goto fail;
		}

		com_driver = &pios_serial_com_driver;
	}

	if (PIOS_COM_Init(&com_id, com_driver, lower_id,
				SERIAL_BUF_LEN, SERIAL_BUF_LEN)) {
		printf("Can't init PIOS_COM\n");
		goto fail;
	}

	if (!strcmp(drv_name, "gps")) {
		PIOS_Modules_Enable(PIOS_MODULE_GPS);
		PIOS_COM_GPS = com_id;
	} else if (!strcmp(drv_name, "msp")) {
		PIOS_Modules_Enable(PIOS_MODULE_UAVOMSPBRIDGE);
		PIOS_COM_MSP = com_id;
	} else if (!strcmp(drv_name, "lighttelemetry")) {
		PIOS_Modules_Enable(PIOS_MODULE_UAVOLIGHTTELEMETRYBRIDGE);
		PIOS_COM_LIGHTTELEMETRY = com_id;
	} else if (!strcmp(drv_name, "telemetry")) {
		PIOS_COM_TELEM_RF = com_id;
#ifdef PIOS_INCLUDE_OMNIP
	} else if (!strcmp(drv_name, "omnip")) {
		omnip_dev_t dontcare;

		if (PIOS_OMNIP_Init(&dontcare, com_driver, lower_id)) {
			printf("Can't init OmniP radar\n");
			goto fail;
		}
#endif
	} else {
		printf("Unknown serial driver %s\n", drv_name);
		goto fail;
	}

	return 0;
fail:
	return -1;
}
#endif

#ifdef PIOS_INCLUDE_I2C
static int handle_i2c_device(const char *optarg) {
	char arg_copy[128];

	strncpy(arg_copy, optarg, sizeof(arg_copy));
	arg_copy[sizeof(arg_copy) - 1] = 0;

	char *saveptr;

	char *drv_name = strtok_r(arg_copy, ":", &saveptr);
	if (drv_name == NULL) {
		goto fail;
	}

	char *bus_num_str = strtok_r(NULL, ":", &saveptr);
	if (bus_num_str == NULL) {
		goto fail;
	}

	int bus_num = atoi(bus_num_str);
	if ((bus_num < 0) || (bus_num >= num_i2c)) {
		goto fail;
	}

	if (!strcmp(drv_name, "px4flow")) {
		struct pios_px4flow_cfg *px4_cfg;

		px4_cfg = PIOS_malloc(sizeof(*px4_cfg));

		external_i2c_adapter_id = i2c_devs[bus_num];
	} else if (!strcmp(drv_name, "hmc5883")) {
		if (PIOS_HAL_ConfigureExternalMag(HWSHARED_MAG_EXTERNALHMC5883,
					mag_orientation,
					i2c_devs + bus_num,
					NULL)) {
			goto fail;
		}
	} else if (!strcmp(drv_name, "hmc5983")) {
		if (PIOS_HAL_ConfigureExternalMag(HWSHARED_MAG_EXTERNALHMC5983,
					mag_orientation, i2c_devs + bus_num,
					NULL)) {
			goto fail;
		}
	} else if (!strcmp(drv_name, "bmp280")) {
		if (PIOS_HAL_ConfigureExternalBaro(HWSHARED_EXTBARO_BMP280,
					i2c_devs + bus_num, NULL)) {
			goto fail;
		}
	} else if (!strcmp(drv_name, "ms5611")) {
		if (PIOS_HAL_ConfigureExternalBaro(HWSHARED_EXTBARO_MS5611,
					i2c_devs + bus_num, NULL)) {
			goto fail;
		}
	} else {
		goto fail;
	}

	return 0;
fail:
	return -1;
}
#endif

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

	if (!strcmp(drv_name, "ms5611")) {
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
		bzero(bmx055_cfg, sizeof(*bmx055_cfg));

		bmx055_cfg->int_pin = gyro_int;

		int ret = PIOS_BMX055_SPI_Init(&dev, spi_devs[bus_num], dev_num, dev_num+1, bmx055_cfg);

		if (ret) goto fail;
	} else if (!strcmp(drv_name, "bmm150")) {
		struct pios_bmm150_cfg *bmm150_cfg;
		pios_bmm150_dev_t dev;

		bmm150_cfg = PIOS_malloc(sizeof(*bmm150_cfg));
		bzero(bmm150_cfg, sizeof(*bmm150_cfg));

		if (mag_orientation == 255) {
			bmm150_cfg->orientation = PIOS_BMM_TOP_90DEG;
		} else {
			/* XXX figure out orientation properly */
			goto fail;
		}

		int ret = PIOS_BMM150_SPI_Init(&dev, spi_devs[bus_num], dev_num, bmm150_cfg);

		if (ret) goto fail;
	} else if (!strcmp(drv_name, "flyingpio")) {
		pios_flyingpio_dev_t dev;

		int ret = PIOS_FLYINGPIO_SPI_Init(&dev, spi_devs[bus_num], dev_num);

		if (ret) goto fail;

		uintptr_t rcvr_id;

		if (PIOS_RCVR_Init(&rcvr_id, &pios_flyingpio_rcvr_driver,
				(uintptr_t) dev)) {
			PIOS_Assert(0);
		}

		/* Pretend what we get is PWM */
		PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM,
			rcvr_id);

		uintptr_t adc_id;

		if (PIOS_ADC_Init(&adc_id, &pios_flyingpio_adc_driver,
				(uintptr_t) dev)) {
			PIOS_Assert(0);
		}
	} else {
		goto fail;
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

	rc = sched_setaffinity(0, sizeof(allowable_cpus), &allowable_cpus);

	if (rc) {
		perror("sched_setaffinity");
		exit(1);
	}

	/* Next, let's go hard realtime. */

	struct sched_param sch_p = {
		.sched_priority = 10
	};

	rc = sched_setscheduler(0, SCHED_RR, &sch_p);

	if (rc) {
		perror("sched_setscheduler");
		exit(1);
	}

	are_realtime = true;
#else
	printf("Only can do realtime stuff on Linux\n");
	exit(1);
#endif
}

static int saved_argc;
static char **saved_argv;

#ifdef PIOS_INCLUDE_SIMSENSORS_YASIM
bool use_yasim;
#endif

void PIOS_SYS_Args(int argc, char *argv[]) {
	saved_argc = argc;
	saved_argv = argv;

	int opt;

	bool first_arg = true;

	while ((opt = getopt(argc, argv, "yfrx:g:l:s:d:S:I:i:m:")) != -1) {
		switch (opt) {
#ifdef PIOS_INCLUDE_SIMSENSORS_YASIM
			case 'y':
				use_yasim = true;
				break;
#endif
			case 'f':
				debug_fpe = true;
				break;
			case 'r':
				if (!first_arg) {
					printf("Realtime must be before hw\n");
					exit(1);
				}

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
				first_arg = false;
				break;
			}
#ifdef PIOS_INCLUDE_SERIAL
			case 'S':
				if (handle_serial_device(optarg)) {
					printf("Couldn't init device\n");
					exit(1);
				}
				first_arg = false;
				break;
#endif
#ifdef PIOS_INCLUDE_I2C
			case 'm':
			{
				char *endptr;

				if (!first_arg) {
					printf("Mag orientation must be before hw\n");
					exit(1);
				}

				mag_orientation = strtol(optarg, &endptr, 10);

				if (!endptr || (*endptr != '\0')) {
					printf("Invalid mag orientation\n");
					exit(1);
				}
				break;
			}
			case 'I':
			{
				int ret = PIOS_I2C_Init(i2c_devs + num_i2c,
					optarg);

				if (ret < 0) {
					printf("Couldn't init I2C\n");
					exit(1);
				}

				num_i2c++;
				break;
			}
			case 'i':
				if (handle_i2c_device(optarg)) {
					printf("Couldn't init i2c device\n");
					exit(1);
				}
				first_arg = false;
				break;
#endif
#ifdef PIOS_INCLUDE_SPI
			case 'g':
			{
				if (!first_arg) {
					printf("Gyro int must be before hw\n");
					exit(1);
				}

				char *endptr;

				int num;

				num = strtol(optarg, &endptr, 10);

				if (!endptr || (*endptr != '\0')) {
					printf("Invalid gyro int pin\n");
					exit(1);
				}

				gyro_int = DIO_MAKE_TAG(num);

				break;
			}

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

				first_arg = false;
				break;
			}
			case 'g':
			{
				uint16_t port = atoi(optarg);

				flightgear_dev_t dontcare;

				if (PIOS_FLIGHTGEAR_Init(&dontcare, port)) {
					printf("Couldn't init FGear\n");
					exit(1);
				}

				first_arg = false;
				break;
			}
			case 'x':
			{
				int timeout = atoi(optarg);

				alarm(timeout);
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
	printf("\nSIGFPE received.  OMG!\n");
	exit(0);
}
#endif

void PIOS_SYS_Init(void)
{
	/* Save the old stdout in case we want to do something with it in
	 * the future, but ensure that all output goes to stderr. */
	orig_stdout = dup(STDOUT_FILENO);

	if (orig_stdout < 0) {
		perror("dup");
		exit(1);
	}

	if (dup2(STDERR_FILENO, STDOUT_FILENO) < 0) {
		perror("dup2");
		exit(1);
	}

	char ser_text[PIOS_SYS_SERIAL_NUM_ASCII_LEN + 1];

	int ret = PIOS_SYS_SerialNumberGet(ser_text);

	if (ret == 0) {
		printf("HW serial number-- hex: %s\n", ser_text);
	}

#ifdef PIOS_INCLUDE_RTC
	PIOS_RTC_Init();
	printf("Pseudo-RTC started\n");
#endif

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
	char *argv[128];

	/* Sigh, ensure NULL termination of array like execvpe(3) expects*/
	int num = saved_argc;
	int i = 0;

	if (num > 128) {
		num = 127;
	}

	for (i = 0; i < num; i++) {
		argv[i] = saved_argv[i];
	}

	argv[i] = 0;

	// Close out all fds-- open or not.
	// We don't want our existing listening socket leaking, etc.
	// Note that we should really be using FD_CLOEXEC, but it's really
	// tricky.. and even if we do, we should leave this to be safe.
	for (i = STDERR_FILENO+1; i < 1024; i++) {
		close(i);
	}

	execvp(argv[0], argv);

	abort();

	return -1;
}

static inline uint64_t find_ser_in_buf(const char *buf)
{
	char *serial = strstr(buf, "Serial");

	if (!serial) {
		return 0;
	}

	serial += 6;

	while ((*serial) && (isblank(*serial))) {
		serial++;
	}

	if (*serial != ':') {
		return 0;
	}

	serial++;

	while (isblank(*serial)) {
		serial++;
	}

	char *endptr;

	uint64_t ret = strtoull(serial, &endptr, 16);

	if (ret == 0) {
		return 0;
	}

	if (*endptr != '\n' && (!*endptr)) {
		return 0;
	}

	return ret;
}

DONT_BUILD_IF(PIOS_SYS_SERIAL_NUM_BINARY_LEN < 12, cpuSerLen);

/**
* Returns the serial number as a string
* param[out] uint8_t pointer to a buf which can store at least
* PIOS_SYS_SERIAL_NUM_BINARY_LEN
* return < 0 if feature not supported
*/
int32_t PIOS_SYS_SerialNumberGetBinary(uint8_t *array)
{
	uint8_t buf[8192];

	int fd;
	int len;

	uint64_t ret = 0;

	fd = open("/proc/cpuinfo", O_RDONLY);

	if (fd >= 0) {
		len = read(fd, buf, sizeof(buf) - 1);

		if (len >= 0) {
			buf[len] = 0;

			ret = find_ser_in_buf((const char *) buf);
		}

		close(fd);

		if (ret) {
			array[0] = 'C';
			array[1] = 'P';
			array[2] = 'U';
			array[3] = ':';

			for (int i = 4; i < PIOS_SYS_SERIAL_NUM_BINARY_LEN; i++) {
				array[i] = ret >> 56;

				ret <<= 8;
			}

			return 0;
		}
	}

	fd = open("/sys/class/dmi/id/board_serial", O_RDONLY);

	if (fd >= 0) {
		len = read(fd, buf, sizeof(buf) - 1);

		if (len > 0) {
			/* Chomp newline */
			if (buf[len-1] == '\n') {
				len--;
			}

			if (len < PIOS_SYS_SERIAL_NUM_BINARY_LEN) {
				memcpy(array, buf, len);

				for (int i = len; i < PIOS_SYS_SERIAL_NUM_BINARY_LEN;
						i++) {
					array[i] = 0;
				}
			} else {
				/* Too long to all fit in.  So put the prefix of the
				 * serial number, and then hash the rest.
				 */
				memcpy(array, buf, PIOS_SYS_SERIAL_NUM_BINARY_LEN - 4);

				SHA1_CTX ctx;
				sha1_init(&ctx);

				sha1_update(&ctx,
					buf + PIOS_SYS_SERIAL_NUM_BINARY_LEN - 4,
					len - PIOS_SYS_SERIAL_NUM_BINARY_LEN + 4);

				uint8_t hash[SHA1_BLOCK_SIZE];
				sha1_final(&ctx, hash);

				array[PIOS_SYS_SERIAL_NUM_BINARY_LEN - 4] = hash[0];
				array[PIOS_SYS_SERIAL_NUM_BINARY_LEN - 3] = hash[1];
				array[PIOS_SYS_SERIAL_NUM_BINARY_LEN - 2] = hash[2];
				array[PIOS_SYS_SERIAL_NUM_BINARY_LEN - 1] = hash[3];
			}

			return 0;
		}
	}

	/* Last resort: all zeroes */
	for (int i = 0; i < PIOS_SYS_SERIAL_NUM_BINARY_LEN; i++) {
		array[i] = 0;
	}

	return 0;
}

static inline char nibble_to_hex(uint8_t c)
{
	if (c < 10) {
		return c + '0';
	}

	return c + 'A' - 10;
}

DONT_BUILD_IF(PIOS_SYS_SERIAL_NUM_BINARY_LEN * 2 != PIOS_SYS_SERIAL_NUM_ASCII_LEN,
		serLenMismatch);


/**
* Returns the serial number as a string
* param[out] str pointer to a string which can store PIOS_SYS_SERIAL_NUM_ASCII_LEN+1
* (24 digits returned for STM32)
* return < 0 if feature not supported
*/
int32_t PIOS_SYS_SerialNumberGet(char *str)
{
	uint8_t array[PIOS_SYS_SERIAL_NUM_BINARY_LEN];

	int ret = PIOS_SYS_SerialNumberGetBinary(array);

	if (ret) {
		return ret;
	}

	for (int i = 0; i < PIOS_SYS_SERIAL_NUM_BINARY_LEN; i++) {
		str[i * 2]     = nibble_to_hex(array[i] >> 4);
		str[i * 2 + 1] = nibble_to_hex(array[i] & 0xf);
	}

	str[PIOS_SYS_SERIAL_NUM_ASCII_LEN] = 0;

	/* No error */
	return 0;
}

size_t PIOS_SYS_IrqStackUnused(void)
{
	return 0;
}

size_t PIOS_SYS_OsStackUnused(void)
{
	return 0;
}

#endif

/**
  * @}
  * @}
  */
