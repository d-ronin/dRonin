#
# Rules to (help) build PiOS common library
#

#
# Directory containing this makefile
#
PIOS_COMMON         := $(dir $(lastword $(MAKEFILE_LIST)))

#
# PIOS device library source and includes
#
PIOS_SRC             = $(filter-out $(PIOS_COMMON)pios_board_info.c $(wildcard $(PIOS_COMMON)printf*.c), $(wildcard $(PIOS_COMMON)*.c))
ifndef USE_USB
PIOS_SRC            := $(filter-out $(wildcard $(PIOS_COMMON)pios_usb*.c), $(PIOS_SRC))
endif
SRC                 += $(PIOS_SRC)

EXTRAINCDIRS        += $(PIOS_COMMON)../inc


