#
# Rules to (help) build the Posix-targeted code.
#

include $(MAKE_INC_DIR)/system-id.mk

#
# Directory containing this makefile
#
PIOS_DEVLIB			:=	$(dir $(lastword $(MAKEFILE_LIST)))

#
# Compiler options implied by posix
#
ARCHFLAGS			+= -DARCH_POSIX
ARCHFLAGS			+= -D_GNU_SOURCE

# The posix code is not threaded but it is possibly re-entrant into the
# system libraries because of task switching.  As a result, request the
# re-entrant libraries and -D_REENTRANT
ifdef MACOSX
# If we are building with clang, it doesn't like the pthread argument being
# passed at link time.  Annoying.
CONLYFLAGS			+= -pthread
else
ARCHFLAGS			+= -pthread
endif

ifeq ($(CROSS_SIM),32)
# Build 32 bit code.
ARCHFLAGS                      += -m32
endif
