#
# Rules to (help) build the F10x device support.
#

#
# Directory containing this makefile
#
PIOS_DEVLIB			:=	$(dir $(lastword $(MAKEFILE_LIST)))

#
# Hardcoded linker script names for now
#
LINKER_SCRIPTS_APP	 =	$(PIOS_DEVLIB)/sections_chibios.ld

#
# ChibiOS settings
#
CDEFS += -DCRT1_AREAS_NUMBER=0
ADEFS += -DCRT0_INIT_RAM_AREAS=FALSE

#
# Compiler options implied by the F30x
#
CDEFS				+= -DSTM32F10X
CDEFS				+= -DSTM32F10X_MD
CDEFS				+= -DHSE_VALUE=$(OSCILLATOR_FREQ)
CDEFS 				+= -DUSE_STDPERIPH_DRIVER
ARCHFLAGS			+= -mcpu=cortex-m3
FLOATABI			= soft

#
# PIOS device library source and includes

SRC += $(BOARD_INFO_DIR)/cmsis_system.c
CMSIS_DEVICEDIR		+=	$(PIOS_DEVLIB)/Libraries/CMSIS/Core/CM3
EXTRAINCDIRS		+=	$(PIOS_DEVLIB)/inc
EXTRAINCDIRS		+=	$(CMSIS_DEVICEDIR)

#
# ST Peripheral library
#
PERIPHLIB			 =	$(PIOS_DEVLIB)/Libraries/STM32F10x_StdPeriph_Driver
EXTRAINCDIRS		+=	$(PERIPHLIB)/inc
SRC					+=	$(wildcard $(PERIPHLIB)/src/*.c)

#
# ST USB Device Lib
#
USBDEVLIB                       =       $(PIOS_DEVLIB)/Libraries/STM32_USB-FS-Device_Driver
EXTRAINCDIRS                    +=      $(USBDEVLIB)/inc
SRC                             +=      $(wildcard $(USBDEVLIB)/src/*.c)

#
# Rules to add ChibiOS/RT to a PiOS target
#
# Note that the PIOS target-specific makefile will detect that CHIBIOS_DIR
# has been defined and add in the target-specific pieces separately.
#

# ChibiOS
CHIBIOS := $(PIOSCOMMONLIB)/ChibiOS

include $(CHIBIOS)/os/hal/ports/STM32/STM32F1xx/platform.mk
include $(CHIBIOS)/os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.mk

include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f1xx.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
include $(CHIBIOS)/os/rt/rt.mk

SRC += $(PLATFORMSRC)
SRC += $(HALSRC)
SRC += $(PORTSRC)
SRC += $(KERNSRC)
SRC += $(OSALSRC)
SRC += $(STARTUPSRC)

ASRC += $(PORTASM)
ASRC += $(STARTUPASM)

EXTRAINCDIRS += $(PLATFORMINC)
EXTRAINCDIRS += $(HALINC)
EXTRAINCDIRS += $(PORTINC)
EXTRAINCDIRS += $(KERNINC)
EXTRAINCDIRS += $(OSALINC)
EXTRAINCDIRS += $(STARTUPINC)

EXTRAINCDIRS += $(CHIBIOS)/os/license
EXTRAINCDIRS += $(CHIBIOS)/os/common/startup/ARMCMx/devices/STM32F1xx/

