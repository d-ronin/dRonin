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

include $(PIOSCOMMONLIB)/ChibiOS/os/hal/platforms/STM32F1xx/platform.mk
include $(PIOSCOMMONLIB)/ChibiOS/os/hal/hal.mk
include $(PIOSCOMMONLIB)/ChibiOS/os/ports/GCC/ARMCMx/STM32F1xx/port.mk
include $(PIOSCOMMONLIB)/ChibiOS/os/kernel/kernel.mk

SRC += $(PLATFORMSRC)
SRC += $(HALSRC)
SRC += $(PORTSRC)
SRC += $(KERNSRC)

EXTRAINCDIRS += $(PLATFORMINC)
EXTRAINCDIRS += $(HALINC)
EXTRAINCDIRS += $(PORTINC)
EXTRAINCDIRS += $(KERNINC)


