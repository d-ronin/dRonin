FLOATABI ?= soft
UAVOBJLIB := $(OUTDIR)/../uavobjects_arm$(FLOATABI)fp/libuavobject.a

# Define programs and commands.
REMOVE  = rm -f

ifeq ($(BUILD_UAVO), YES)
SRC += $(wildcard $(OPUAVSYNTHDIR)/*.c )
endif

# List of all source files.
ALLSRC     :=  $(ASRC) $(SRC) $(CPPSRC)

# List of all source files without directory and file-extension.
ALLSRCBASE = $(notdir $(basename $(ALLSRC)))
# Define all object files.
ALLOBJ     = $(addprefix $(OUTDIR)/, $(addsuffix .o, $(ALLSRCBASE)))
# Define all listing files (used for make clean).
LSTFILES   = $(addprefix $(OUTDIR)/, $(addsuffix .lst, $(ALLSRCBASE)))
# Define all depedency-files (used for make clean).
DEPFILES   = $(addprefix $(OUTDIR)/dep/, $(addsuffix .o.d, $(ALLSRCBASE)))

# Default target.
all: gccversion build

build: elf

ifneq ($(BUILD_FWFILES), NO)
build: hex bin lss sym
endif

# Compile: create object files from C source files.

# Link: create ELF output file from object files.
$(eval $(call LINK_TEMPLATE, $(OUTDIR)/$(TARGET).elf, $(ALLOBJ) $(LIBS)))

# Assemble: create object files from assembler source files.
$(foreach src, $(ASRC), $(eval $(call ASSEMBLE_TEMPLATE, $(src))))

$(foreach src, $(SRC), $(eval $(call COMPILE_C_TEMPLATE, $(src))))

# Compile: create object files from C++ source files.
$(foreach src, $(CPPSRC), $(eval $(call COMPILE_CPP_TEMPLATE, $(src))))

$(OUTDIR)/$(TARGET).bin.o: $(OUTDIR)/$(TARGET).bin

# Allows the bin to be padded up to the firmware description blob base
# Required for boards which don't use the TL bootloader to put
# the blob at the correct location
ifdef PAD_TLFW_FW_DESC
FW_DESC_BASE := $(shell echo $$(($(FW_BANK_SIZE)-$(FW_DESC_SIZE))))
else 
FW_DESC_BASE = 0
endif

$(eval $(call TLFW_TEMPLATE,$(OUTDIR)/$(TARGET).bin,$(BOARD_TYPE),$(BOARD_REVISION)))

# Add jtag targets (program and wipe)
$(eval $(call JTAG_TEMPLATE,$(OUTDIR)/$(TARGET).elf,$(FW_BANK_BASE),$(FW_BANK_SIZE),$(OPENOCD_JTAG_CONFIG),$(OPENOCD_CONFIG)))

.PHONY: elf lss sym hex bin bino tlfw
elf: $(OUTDIR)/$(TARGET).elf
lss: $(OUTDIR)/$(TARGET).lss
sym: $(OUTDIR)/$(TARGET).sym
hex: $(OUTDIR)/$(TARGET).hex
bin: $(OUTDIR)/$(TARGET).bin
bino: $(OUTDIR)/$(TARGET).bin.o
tlfw: $(OUTDIR)/$(TARGET).tlfw

# Display sizes of sections.
$(eval $(call SIZE_TEMPLATE, $(OUTDIR)/$(TARGET).elf))

# Create output files directory
# all known MS Windows OS define the ComSpec environment variable
ifdef ComSpec
$(shell md $(subst /,\\,$(OUTDIR)) 2>NUL)
else
$(shell mkdir -p $(OUTDIR) 2>/dev/null)
endif

# Include the dependency files.
ifdef ComSpec
-include $(shell md $(subst /,\\,$(OUTDIR))\dep 2>NUL) $(wildcard $(OUTDIR)/dep/*)
else
-include $(shell mkdir $(OUTDIR) 2>/dev/null) $(shell mkdir $(OUTDIR)/dep 2>/dev/null) $(wildcard $(OUTDIR)/dep/*)
endif

# Listing of phony targets.
.PHONY : all build clean clean_list install docs
