# linux.mk
#
# Goals:
#   Configure an environment that will allow dRonin GCS and firmware to be built
#   on a Linux system.

QT_SPEC ?= linux-g++
QT_CLANG_SPEC ?= linux-clang

UAVOBJGENERATOR="$(BUILD_DIR)/ground/uavobjgenerator/uavobjgenerator"
