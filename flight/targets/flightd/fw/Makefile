#####
# Makefile for host flight target (flightd)
#
# The OpenPilot Team, http://www.openpilot.org, Copyright (C) 2009.
# Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
# dRonin, http://dronin.org Copyright (C) 2015-2017
#
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, see <http://www.gnu.org/licenses/>
#
# Additional note on redistribution: The copyright and license notices above
# must be maintained in each individual source file that is a derivative work
# of this source file; otherwise redistribution is prohibited.
#####

# Default target.
.PHONY: all
all: elf

override THUMB :=

TCHAIN_PREFIX =

include $(MAKE_INC_DIR)/firmware-defs.mk

ifeq ($(CROSS_FLIGHTD),pi)

TCHAIN_PREFIX = arm-linux-gnueabihf-
LDFLAGS += -static

# Prefer GCC 6
ARMGCC:=$(shell command -v arm-linux-gnueabihf-gcc-6 2>/dev/null)

ifneq ($(ARMGCC),)
CC = $(CCACHE) $(ARMGCC)
BARECC = $(ARMGCC)
endif

endif

# Set developer code and compile options
# Set to YES to compile for debugging
# DEBUG ?= YES

CFLAGS += -DSTACK_DIAGNOSTICS
CFLAGS += -DRATEDESIRED_DIAGNOSTICS
CFLAGS += -DWDG_STATS_DIAGNOSTICS
CFLAGS += -DDIAG_TASKS

# Since we are running all this firmware the code needs to know what the BL would
# normally contain
BLONLY_CDEFS += -DBOARD_TYPE=$(BOARD_TYPE)
BLONLY_CDEFS += -DBOARD_REVISION=$(BOARD_REVISION)
BLONLY_CDEFS += -DHW_TYPE=$(HW_TYPE)

# Since we are simulating all this firmware the code needs to know what the BL would
# normally contain
CFLAGS += $(BLONLY_CDEFS)
CFLAGS += -D_XOPEN_SOURCE

# List of modules to include
MODULES += Actuator
MODULES += Attitude
MODULES += FirmwareIAP
MODULES += Logging
MODULES += ManualControl
# On "sim", the sensor stabilization module instead invokes the fake one when
# gyros are not present.
MODULES += Stabilization/simulated
MODULES += Stabilization
MODULES += Telemetry

OPTMODULES += Airspeed
OPTMODULES += Autotune
OPTMODULES += Battery
OPTMODULES += FlightStats
OPTMODULES += Geofence
OPTMODULES += PathPlanner
OPTMODULES += TxPID

OPTMODULES += GPS
OPTMODULES += UAVOLighttelemetryBridge
OPTMODULES += UAVOMSPBridge

# Paths
OPUAVOBJINC = $(OPUAVOBJ)/inc
PIOSINC = $(PIOS)/inc
FLIGHTLIBINC = $(FLIGHTLIB)/inc
MATHLIB = $(FLIGHTLIB)/math
MATHLIBINC = $(MATHLIB)
CRYPTOLIB = $(FLIGHTLIB)/crypto
CRYPTOLIBINC = $(CRYPTOLIB)
PIOSPOSIX = $(PIOS)/posix
PIOSCOMMON = $(PIOS)/Common
PIOSCOMMONLIB = $(PIOSCOMMON)/Libraries
PIOSBOARDS = $(PIOS)/Boards
PIOSPOSIXLIB = $(PIOSPOSIX)/Libraries
APPLIBDIR = $(PIOSPOSIX)/Libraries
RTOSDIR = $(APPLIBDIR)/FreeRTOS
RTOSSRCDIR = $(RTOSDIR)/Source
RTOSINCDIR = $(RTOSSRCDIR)/include
DOXYGENDIR = ../Doc/Doxygen

SRC = 

# List C source files here. (C dependencies are automatically generated.)
# use file-extension c for "c-only"-files

## OPENPILOT CORE:
SRC += ${OPMODULEDIR}/System/systemmod.c
SRC += ${OPMODULEDIR}/System/rgbleds.c
SRC += main.c
SRC += pios_board.c
SRC += $(wildcard $(FLIGHTLIB)/*.c)
SRC += $(OPUAVOBJ)/uavobjectmanager.c

## Libraries for flight calculations
SRC += taskmonitor.c

SRC += $(MATHLIB)/atmospheric_math.c
SRC += $(MATHLIB)/coordinate_conversions.c
SRC += $(MATHLIB)/misc_math.c
SRC += $(MATHLIB)/pid.c
SRC += $(MATHLIB)/lpfilter.c
SRC += $(MATHLIB)/smoothcontrol.c
SRC += $(CRYPTOLIB)/sha1.c

include $(PIOS)/posix/library.mk

SRC += pios_com.c
SRC += pios_crc.c
SRC += pios_flash.c
SRC += pios_flashfs_logfs.c
SRC += pios_rcvr.c
SRC += pios_sensors.c
SRC += pios_board_info.c
SRC += pios_semaphore.c
SRC += pios_mutex.c
SRC += pios_thread.c
SRC += pios_queue.c
SRC += pios_streamfs.c

SRC += pios_modules.c

SRC += pios_adc.c
SRC += pios_annunc.c
SRC += pios_bl_helper.c
SRC += pios_bmm150.c
SRC += pios_bmp280.c
SRC += pios_bmx055.c
SRC += pios_debug.c
SRC += pios_delay.c
SRC += pios_fileout.c
SRC += pios_flightgear.c
SRC += pios_flash_posix.c
SRC += pios_flyingpio.c
SRC += pios_uavtalkrcvr.c
SRC += pios_hal.c
SRC += pios_heap.c
SRC += pios_hmc5883.c
SRC += pios_hmc5983.c
SRC += pios_iap.c
SRC += pios_i2c.c
SRC += pios_irq.c
SRC += pios_ms5611.c
SRC += pios_ms5611_spi.c
SRC += pios_px4flow.c
SRC += pios_omnip.c
SRC += pios_reset.c
SRC += pios_rtc.c
SRC += pios_serial.c
SRC += pios_servo.c
SRC += pios_spi.c
SRC += pios_sys.c
SRC += pios_tcp.c
SRC += pios_wdg.c

# List C source files here which must be compiled in ARM-Mode (no -mthumb).
# use file-extension c for "c-only"-files
## just for testing, timer.c could be compiled in thumb-mode too
SRCARM =

# List C++ source files here.
# use file-extension .cpp for C++-files (not .C)
CPPSRC =

# List C++ source files here which must be compiled in ARM-Mode.
# use file-extension .cpp for C++-files (not .C)
#CPPSRCARM = $(TARGET).cpp
CPPSRCARM =


# List any extra directories to look for include files here.
#    Each directory must be seperated by a space.
EXTRAINCDIRS  += $(ROOT_DIR)/flight/PiOS.posix/inc
EXTRAINCDIRS  += $(SHAREDAPIDIR)
EXTRAINCDIRS  += .
EXTRAINCDIRS  += $(PIOS)
EXTRAINCDIRS  += $(PIOSINC)
EXTRAINCDIRS += $(PIOSCOMMON)/inc
EXTRAINCDIRS  += $(ROOT_DIR)/flight/PiOS/inc
EXTRAINCDIRS  += $(OPUAVOBJ)
EXTRAINCDIRS  += $(OPUAVOBJINC)
EXTRAINCDIRS  += $(OPUAVSYNTHDIR)
EXTRAINCDIRS  += $(FLIGHTLIBINC)
EXTRAINCDIRS  += $(MATHLIBINC)
EXTRAINCDIRS  += $(CRYPTOLIBINC)

EXTRAINCDIRS  += $(PIOSCOMMON)

EXTRAINCDIRS  += $(CMSISDIR)
EXTRAINCDIRS  += $(BOOTINC)

EXTRAINCDIRS  += $(PIOSPOSIX)
EXTRAINCDIRS  += $(RTOSINCDIR)
#EXTRAINCDIRS  += $(APPLIBDIR)
EXTRAINCDIRS  += $(RTOSSRCDIR)/portable/GCC/Posix

EXTRAINCDIRS += ${foreach MOD, ${OPTMODULES} ${MODULES}, $(OPMODULEDIR)/${MOD}/inc} ${OPMODULEDIR}/System/inc

# Optimization level, can be [0, 1, 2, 3, s].
# 0 = turn off optimization. s = optimize for size.
# (Note: 3 is not always the best optimization level. See avr-libc FAQ.)

ifeq ($(DEBUG),YES)
CFLAGS += -O0 -g
CFLAGS += -DGENERAL_COV
#CFLAGS += -finstrument-functions -ffixed-r10

# Turn on gcov support
CFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS += -fprofile-arcs

else
CFLAGS += -O2 -g3
endif

# common architecture-specific flags from the device-specific library makefile
CFLAGS += $(ARCHFLAGS)
CFLAGS += $(UAVOBJDEFINE)

# configure CMSIS DSP Library
CDEFS += -DARM_MATH_FLIGHT
CDEFS += -DARM_MATH_MATRIX_CHECK
CDEFS += -DARM_MATH_ROUNDING

# This is not the best place for these.  Really should abstract out
# to the board file or something
CFLAGS += -DMEM_SIZE=1024000000

# Debugging format.
DEBUGF = dwarf-2

# Place project-specific -D (define) and/or
# -U options for C here.
CDEFS += -DHSE_VALUE=$(OSCILLATOR_FREQ)
CDEFS += -DSYSCLK_FREQ=$(SYSCLK_FREQ)
CDEFS += -DUSE_STDPERIPH_DRIVER
CDEFS += -DUSE_$(BOARD)

# Make sure the build knows we're building a sim version
CDEFS += -DFLIGHT_POSIX

# Declare all non-optional modules as built-in to force inclusion
get_mod_name = $(shell echo $(1) | sed "s/\/[^\/]*$///")
BUILTIN_DEFS := ${foreach MOD, ${MODULES}, -DMODULE_$(call get_mod_name, $(MOD))_BUILTIN }
CDEFS += ${BUILTIN_DEFS}

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99

#-----

# Compiler flags.

#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
#
# Flags for C and C++ (arm-elf-gcc/arm-elf-g++)

ifeq ($(DEBUG),YES)
CFLAGS += -g$(DEBUGF)
endif

CFLAGS += -DARCH_POSIX
CFLAGS += $(CDEFS)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS)) -I.

#CFLAGS += -fomit-frame-pointer

CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -Wno-deprecated-declarations
# Compiler flags to generate dependency files:
CFLAGS += -MD -MP -MF $(OUTDIR)/dep/$(@F).d

# flags only for C
#CONLYFLAGS += -Wnested-externs
CONLYFLAGS += $(CSTANDARD)

# Linker flags.
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
#LDFLAGS = -Wl,-Map=$(OUTDIR)/$(TARGET).map,--cref,--gc-sections
LDFLAGS += $(patsubst %,-L%,$(EXTRA_LIBDIRS))
LDFLAGS += $(patsubst %,-l%,$(EXTRA_LIBS))
LDFLAGS += -lm
LDFLAGS += -lpthread

ifdef WINDOWS
LDFLAGS += -lws2_32
endif

# Define programs and commands.
REMOVE  = rm -f

$(OUTDIR)/sim_firmwareinfo.c: $(ROOT_DIR)/make/templates/firmwareinfotemplate.c
	$(V1) $(ROOT_DIR)/make/scripts/version-info.py \
		--path=$(ROOT_DIR) \
		--template=$^ \
		--outfile=$@ \
		--type=$(BOARD_TYPE) \
		--revision=$(BOARD_REVISION) \
		--uavodir=$(ROOT_DIR)/shared/uavobjectdefinition


# List of all source files.
SRC       += $(OUTDIR)/sim_firmwareinfo.c

.PHONY: elf
elf: $(OUTDIR)/$(TARGET)

$(OUTDIR)/$(TARGET): $(OUTDIR)/$(TARGET).elf
	mv $< $@

result = ${shell echo "test"}
ifeq (${result}, test)
	quote = '
else
	quote =
endif

getmodname = $(firstword $(subst /, ,$1))

MOD_GEN := $(foreach MOD,$(MODULES) $(OPTMODULES),$(call getmodname,$(MOD)))

BUILD_FWFILES=NO
BUILD_UAVO=YES

include $(MAKE_INC_DIR)/firmware-common.mk
