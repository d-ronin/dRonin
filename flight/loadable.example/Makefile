DRONINPATH     := $(shell cd ../.. ; pwd)
FLIGHTPATH     = $(DRONINPATH)/flight
TOOLS_DIR      = $(DRONINPATH)/tools
ARM_SDK_DIR    = $(TOOLS_DIR)/gcc-arm-none-eabi-6-2017-q2-update
ARM_SDK_PREFIX = $(ARM_SDK_DIR)/bin/arm-none-eabi-
TCHAIN_PREFIX  = $(ARM_SDK_PREFIX)
CC             = $(TCHAIN_PREFIX)gcc
OBJCOPY        = $(TCHAIN_PREFIX)objcopy

SRC = loadabletest.c

MCU = cortex-m4

CFLAGS = -mthumb -mcpu=$(MCU) -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
	 -g3 -Os -fpic \
	 -msingle-pic-base -mpic-register=r9 -mno-pic-data-is-text-relative \
	 -Wa,-adhlns=$(addsuffix .lst, $(basename $<)) \
	 -I$(FLIGHTPATH)/Libraries/inc \
	 -I$(FLIGHTPATH)/PiOS/inc \
	 -I$(FLIGHTPATH)/PiOS/STM32/inc \
	 -I$(DRONINPATH)/build/uavobject-synthetics/flight \
	 -I$(FLIGHTPATH)/UAVObjects/inc

LDFLAGS = -Tmodule_layout.ld -nostartfiles -Xlinker -Map=output.map

output.bin: output.elf
	$(OBJCOPY) -O binary $< $@

output.elf: $(SRC) Makefile module_layout.ld
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(SRC)
