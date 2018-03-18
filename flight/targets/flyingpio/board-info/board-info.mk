BOARD_TYPE          := 0x88
BOARD_REVISION      := 0x01
HW_TYPE             := 0x00		# seems to be unused

CHIP                := STM32F030K6T6
BOARD               := FLYINGPIO

USB_VEND            := ""
USB_PROD            := ""

include $(MAKE_INC_DIR)/firmware-arches.mk

FW_BANK_BASE        := 0x08000000  # Start of firmware flash
FW_BANK_SIZE        := 0x00008000  # 32kbytes, including firmware descriptor.

FW_DESC_SIZE        := 0x00000064

EF_BANK_BASE        := 0x08000000  # Start of entire flash image (usually start of bootloader as well)
EF_BANK_SIZE        := 0x00008000  # Size of the entire flash image (from bootloader until end of firmware)

OSCILLATOR_FREQ     :=   8000000
SYSCLK_FREQ         :=  40000000   # Actual fmax is 48000000, but margin is nice

