
#
# Compiler options implied by the F4xx
#
CDEFS				+= -DSTM32F10X
CDEFS				+= -DSTM32F10X_MD
CDEFS				+= -DHSE_VALUE=$(OSCILLATOR_FREQ)
CDEFS 				+= -DUSE_STDPERIPH_DRIVER
ARCHFLAGS			+= -mcpu=cortex-m3
FLOATABI			+= soft

#
# PIOS device library source and includes
#
EXTRAINCDIRS		+=	$(PIOS_DEVLIB)/inc

#
# CMSIS for the F1
#
include $(PIOSCOMMONLIB)/CMSIS/library.mk
CMSIS_DEVICEDIR	:=	$(PIOS_DEVLIB)/Libraries/CMSIS/Core/CM3
SRC			+=	$(BOARD_INFO_DIR)/cmsis_system.c
EXTRAINCDIRS		+=	$(CMSIS_DEVICEDIR)

#
# ST Peripheral library
#
PERIPHLIB			 =	$(PIOS_DEVLIB)/Libraries/STM32F10x_StdPeriph_Driver
EXTRAINCDIRS		+=	$(PERIPHLIB)/inc
SRC					+=	$(wildcard $(PERIPHLIB)/src/*.c)

#
# ST USB Device library
#
USBDEVLIB			=	$(PIOS_DEVLIB)/Libraries/STM32_USB-FS-Device_Driver
EXTRAINCDIRS			+=	$(USBDEVLIB)/inc
SRC				+=	$(wildcard $(USBDEVLIB)/src/*.c)
