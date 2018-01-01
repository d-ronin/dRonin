#
# Rules to add ChibiOS/RT to a PiOS target
#
# Note that the PIOS target-specific makefile will detect that CHIBIOS_DIR
# has been defined and add in the target-specific pieces separately.
#

# ChibiOS
CHIBIOS := $(PIOS)/ChibiOS

include $(CHIBIOS)/os/hal/ports/STM32/STM32F4xx/platform.mk
include $(CHIBIOS)/os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.mk

include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f4xx.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/rules.mk

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
EXTRAINCDIRS += $(CHIBIOS)/os/common/startup/ARMCMx/devices/STM32F4xx/
