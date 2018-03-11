# macosx.mk
#
# Goals:
#   Configure an environment that will allow dRonin GCS and firmware to be built
#   on a Mac OSX system.

QT_SPEC ?= macx-clang
QT_CLANG_SPEC ?= macx-clang

UAVOBJGENERATOR="$(BUILD_DIR)/ground/uavobjgenerator/uavobjgenerator"
