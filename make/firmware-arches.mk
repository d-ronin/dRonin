ifneq "$(findstring native,$(CHIP))" ""
MCU                 := native
ARCH_TYPES          := posix
else ifneq "$(findstring STM32F446,$(CHIP))" ""
OPENOCD_JTAG_CONFIG ?= stlink-v2.cfg
OPENOCD_CONFIG      := stm32f4x.cfg
MCU                 := cortex-m4
STM32_TYPE          := STM32F446xx
ARCH_TYPES          := STM32F4xx STM32
else ifneq "$(findstring STM32F40,$(CHIP))" ""
OPENOCD_JTAG_CONFIG ?= stlink-v2.cfg
OPENOCD_CONFIG      := stm32f4x.cfg
MCU                 := cortex-m4
STM32_TYPE          := STM32F40_41xxx
ARCH_TYPES          := STM32F4xx STM32
else ifneq "$(findstring STM32F30,$(CHIP))" ""
OPENOCD_JTAG_CONFIG ?= stlink-v2.cfg
OPENOCD_CONFIG      := stm32f3x.cfg
MCU                 := cortex-m4
STM32_TYPE          := STM32F30X
ARCH_TYPES          := STM32F30x STM32
else ifneq "$(findstring STM32F1,$(CHIP))" ""
OPENOCD_JTAG_CONFIG ?= stlink-v2.cfg
OPENOCD_CONFIG      := stm32f1x.cfg
MCU                 := cortex-m3
# This is a little bit fudged, but fits the rest of stuff better
STM32_TYPE          := STM32F10X_MD
ARCH_TYPES          := STM32F10x STM32
else ifneq "$(findstring STM32F0,$(CHIP))" ""
OPENOCD_JTAG_CONFIG ?= stlink-v2.cfg
OPENOCD_CONFIG      := stm32f0x.cfg
MCU                 := cortex-m0
STM32_TYPE          := STM32F0XX
ARCH_TYPES          := STM32F0xx STM32
endif

vpath % $(BOARD_ROOT_DIR)/$(BUILD_TYPE)

vpath % $(foreach d,$(ARCH_TYPES) Common,$(PIOS)/$(d))
