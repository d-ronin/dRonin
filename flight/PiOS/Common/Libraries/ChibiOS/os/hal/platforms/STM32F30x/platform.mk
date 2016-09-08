# List of all the STM32F30x platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/STM32F30x/stm32_dma.c \
              ${CHIBIOS}/os/hal/platforms/STM32F30x/hal_lld.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/STM32F30x \
              ${CHIBIOS}/os/hal/platforms/STM32 \
              ${CHIBIOS}/os/hal/platforms/STM32/GPIOv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/I2Cv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/RTCv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/SPIv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/USARTv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/USBv1
