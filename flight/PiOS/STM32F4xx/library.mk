#
# Rules to (help) build the F4xx device support.
#

#
# Compiler options implied by the F4xx
#
CDEFS				+= -DSTM32F4XX
CDEFS				+= -DHSE_VALUE=$(OSCILLATOR_FREQ)
CDEFS 				+= -DUSE_STDPERIPH_DRIVER
ARCHFLAGS			+= -mcpu=cortex-m4 -march=armv7e-m -mfpu=fpv4-sp-d16 -mfloat-abi=hard
FLOATABI			+= hard

#
# PIOS device library source and includes
#
EXTRAINCDIRS		+=	$(PIOS_DEVLIB)/inc

#
# CMSIS for the F4
#
include $(FLIGHTLIB)/CMSIS/library.mk
CMSIS_DEVICEDIR	:=	$(PIOS_DEVLIB)/Libraries/CMSIS/Device/ST/STM32F4xx
SRC			+=	$(BOARD_INFO_DIR)/cmsis_system.c
EXTRAINCDIRS		+=	$(CMSIS_DEVICEDIR)/Include

#
# ST Peripheral library
#
PERIPHLIB			 =	$(PIOS_DEVLIB)/Libraries/STM32F4xx_StdPeriph_Driver
EXTRAINCDIRS		+=	$(PERIPHLIB)/inc
SRC					+=	$(wildcard $(PERIPHLIB)/src/*.c)

#
# ST USB OTG library
#
USBOTGLIB			=	$(PIOS_DEVLIB)/Libraries/STM32_USB_OTG_Driver
USBOTGLIB_SRC			=	usb_core.c usb_dcd.c usb_dcd_int.c
EXTRAINCDIRS			+=	$(USBOTGLIB)/inc
SRC				+=	$(addprefix $(USBOTGLIB)/src/,$(USBOTGLIB_SRC))

#
# ST USB Device library
#
USBDEVLIB			=	$(PIOS_DEVLIB)/Libraries/STM32_USB_Device_Library
EXTRAINCDIRS			+=	$(USBDEVLIB)/Core/inc
SRC				+=	$(wildcard $(USBDEVLIB)/Core/src/*.c)
