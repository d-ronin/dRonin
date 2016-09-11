# List of all the STM32F2xx/STM32F4xx platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/STM32F4xx/stm32_dma.c \
              ${CHIBIOS}/os/hal/platforms/STM32F4xx/hal_lld.c 

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/STM32F4xx \
              ${CHIBIOS}/os/hal/platforms/STM32 \
              ${CHIBIOS}/os/hal/platforms/STM32/GPIOv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/I2Cv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/OTGv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/RTCv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/SPIv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/USARTv1
