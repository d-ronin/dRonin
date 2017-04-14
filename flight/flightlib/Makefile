# make the build output a bit nicer
BUILD_TYPE = fl

BOARD_SHORT_NAME=unkn

ifneq ($(strip $(filter flightlib_armsoftfp,$(MAKECMDGOALS))),)
BOARD_SHORT_NAME=slib
endif

ifneq ($(strip $(filter flightlib_armhardfp,$(MAKECMDGOALS))),)
BOARD_SHORT_NAME=hlib
endif

flightlib_armsoftfp: LIBARCHFLAGS = -mfloat-abi=soft -mcpu=cortex-m3
flightlib_armsoftfp: $(OUTDIR)/libflightlib.a
flightlib_armhardfp: LIBARCHFLAGS = -mfloat-abi=hard -mcpu=cortex-m4 -march=armv7e-m -mfpu=fpv4-sp-d16
flightlib_armhardfp: $(OUTDIR)/libflightlib.a

include $(MAKE_INC_DIR)/firmware-defs.mk

OPUAVTALKINC = $(OPUAVTALK)/inc
OPUAVOBJINC = $(OPUAVOBJ)/inc
FLIGHTLIBINC = $(FLIGHTLIB)/inc
MATHINC = $(FLIGHTLIB)/math
PIOSINC = $(PIOS)/inc
PIOSCOMMON = $(PIOS)/Common
PIOSCOMMONLIB = $(PIOSCOMMON)/Libraries
STMLIBDIR = $(APPLIBDIR)
STMSPDSRCDIR = $(STMSPDDIR)/src
STMSPDINCDIR = $(STMSPDDIR)/inc
STMUSBSRCDIR = $(STMUSBDIR)/src
STMUSBINCDIR = $(STMUSBDIR)/inc
MSDDIR  = $(APPLIBDIR)/msd
DOXYGENDIR = ../Doc/Doxygen

# List C source files here. (C dependencies are automatically generated.)
# use file-extension c for "c-only"-files

EXTRAINCDIRS  += $(FLIGHTLIBINC)
EXTRAINCDIRS  += $(MATHINC)
EXTRAINCDIRS  += $(OPUAVTALKINC)
EXTRAINCDIRS  += $(OPUAVOBJ)
EXTRAINCDIRS  += $(OPUAVOBJINC)
EXTRAINCDIRS  += $(OPUAVSYNTHDIR)
EXTRAINCDIRS  += $(PIOS)
EXTRAINCDIRS  += $(PIOSINC)
EXTRAINCDIRS  += $(PIOSCOMMON)
EXTRAINCDIRS  += $(APPLIBDIR)
EXTRAINCDIRS  += $(SHAREDAPIDIR)

CFLAGS += -g3
CFLAGS += -std=gnu99
CFLAGS += -Os -fconserve-stack
CFLAGS += $(LIBARCHFLAGS)

CFLAGS += $(CDEFS)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS)) -I.

CFLAGS += -fomit-frame-pointer

CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wdouble-promotion
CFLAGS += -Wa,-adhlns=$(addprefix $(OUTDIR)/, $(notdir $(addsuffix .lst, $(basename $<))))
# Compiler flags to generate dependency files:
CFLAGS += -MD -MP -MF $(OUTDIR)/dep/$(@F).d
CFLAGS += -DPIOS_NO_HW

# Linker flags.
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS = -nostartfiles -Wl,-Map=$(OUTDIR)/$(TARGET).map,--cref,--gc-sections
LDFLAGS += $(patsubst %,-L%,$(EXTRA_LIBDIRS))
LDFLAGS += -lc_nano
LDFLAGS += $(patsubst %,-l%,$(EXTRA_LIBS))
LDFLAGS += $(MATH_LIB)
LDFLAGS += -Wl,--warn-common
LDFLAGS += -Wl,--fatal-warnings

SRC += $(wildcard $(FLIGHTLIB)/*.c)
SRC += $(wildcard $(FLIGHTLIB)/math/*.c)
SRC += $(wildcard $(FLIGHTLIB)/mavlink/*.c)
SRC += $(wildcard $(FLIGHTLIB)/mgrs/*.c)
SRC += $(wildcard $(FLIGHTLIB)/msp/*.c)
SRC += $(wildcard $(FLIGHTLIB)/rscode/*.c)

NO_AUTO_UAVO = YES

include $(MAKE_INC_DIR)/firmware-common.mk

$(eval $(call ARCHIVE_TEMPLATE, $(OUTDIR)/libflightlib.a, $(ALLOBJ)))
