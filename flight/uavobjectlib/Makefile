# make the build output a bit nicer
BUILD_TYPE = uv

BOARD_SHORT_NAME=unkn

ifneq ($(strip $(filter uavobjects_armsoftfp,$(MAKECMDGOALS))),)
BOARD_SHORT_NAME=suav
endif

ifneq ($(strip $(filter uavobjects_armhardfp,$(MAKECMDGOALS))),)
BOARD_SHORT_NAME=huav
endif

uavobjects_armsoftfp: LIBARCHFLAGS = -mfloat-abi=soft -mcpu=cortex-m3
uavobjects_armsoftfp: $(OUTDIR)/libuavobject.a
uavobjects_armhardfp: LIBARCHFLAGS = -mfloat-abi=hard -mcpu=cortex-m4 -march=armv7e-m -mfpu=fpv4-sp-d16
uavobjects_armhardfp: $(OUTDIR)/libuavobject.a

include $(MAKE_INC_DIR)/firmware-defs.mk

OPUAVTALKINC = $(OPUAVTALK)/inc
OPUAVOBJINC = $(OPUAVOBJ)/inc
FLIGHTLIBINC = $(FLIGHTLIB)/inc
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

EXTRAINCDIRS  += $(OPUAVTALK)
EXTRAINCDIRS  += $(OPUAVTALKINC)
EXTRAINCDIRS  += $(OPUAVOBJ)
EXTRAINCDIRS  += $(OPUAVOBJINC)
EXTRAINCDIRS  += $(OPUAVSYNTHDIR)
EXTRAINCDIRS  += $(PIOS)
EXTRAINCDIRS  += $(PIOSINC)
EXTRAINCDIRS  += $(PIOSCOMMON)
EXTRAINCDIRS  += $(APPLIBDIR)

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

SRC += $(filter-out $(OPUAVSYNTHDIR)/uavobjectsinit.c,$(wildcard $(OPUAVSYNTHDIR)/*.c))
NO_AUTO_UAVO = YES

include $(MAKE_INC_DIR)/firmware-common.mk

$(eval $(call ARCHIVE_TEMPLATE, $(OUTDIR)/libuavobject.a, $(ALLOBJ)))
