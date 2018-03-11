# windows.mk
#
# Goals:
#   Configure an environment that will allow dRonin GCS and firmware to be built
#   on a Windows system.
#   

QT_SPEC ?= win32-g++
QT_CLANG_SPEC ?= win32-clang-msvc

# this might need to switch on debug/release
UAVOBJGENERATOR := "$(BUILD_DIR)/ground/uavobjgenerator/debug/uavobjgenerator.exe"
