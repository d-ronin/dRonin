# List of the ChibiOS/RT ARM generic port files.
PORTSRC = ${CHIBIOS}/os/common/ports/ARM/chcore.c

PORTASM = $(CHIBIOS)/os/common/ports/ARM/compilers/GCC/chcoreasm.S

PORTINC = ${CHIBIOS}/os/common/ports/ARM \
          ${CHIBIOS}/os/common/ports/ARM/compilers/GCC
