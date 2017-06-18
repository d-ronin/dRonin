TEMPLATE = lib
TARGET = RunGuard

QT *= core

DEFINES += RUNGUARD_LIB

include(../../gcslibrary.pri)

SOURCES += \
    runguard.cpp

HEADERS += \
    runguard_global.h \
    runguard.h
