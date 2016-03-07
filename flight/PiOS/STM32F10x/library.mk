#
# Rules to (help) build the F10x device support.
#

#
# Directory containing this makefile
#
PIOS_DEVLIB         := $(dir $(lastword $(MAKEFILE_LIST)))

#
# Compiler options implied by the F1xx
#
CDEFS               += -DUSE_STDPERIPH_DRIVER
ARCHFLAGS           += -mcpu=cortex-m3 -march=armv7-m -mfloat-abi=soft
FLOATABI            += soft

#
# PiOS device library source and includes
#
PIOS_F1SRC           = $(filter-out $(PIOS_DEVLIB)pios_i2c.c, $(wildcard $(PIOS_DEVLIB)*.c))
ifndef USE_USB
PIOS_F1SRC          := $(filter-out $(PIOS_DEVLIB)pios_usb.c $(wildcard $(PIOS_DEVLIB)pios_usb*.c), $(PIOS_F1SRC))
endif
SRC                 += $(PIOS_F1SRC)

EXTRAINCDIRS        += $(PIOS_DEVLIB)inc

#
# PiOS startup related options
#
CDEFS               += -DINIT_STACK_SIZE=1200
CDEFS               += -DIRQ_STACK_SIZE=576

#
# ST Peripheral library
#
PERIPHLIB            = $(PIOS_DEVLIB)Libraries/STM32F10x_StdPeriph_Driver/
EXTRAINCDIRS        += $(PERIPHLIB)inc
SRC                 += $(wildcard $(PERIPHLIB)src/*.c)

ifdef USE_USB
#
# ST USB Device library
#
USBDEVLIB            = $(PIOS_DEVLIB)/Libraries/STM32_USB-FS-Device_Driver/
EXTRAINCDIRS        += $(USBDEVLIB)inc
SRC                 += $(wildcard $(USBDEVLIB)src/*.c)
endif
