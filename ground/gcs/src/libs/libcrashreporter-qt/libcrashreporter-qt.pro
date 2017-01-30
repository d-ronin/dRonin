TEMPLATE = lib
CONFIG += staticlib create_prl link_prl
TARGET = crashreporter
include(../../gcslibrary.pri)
QT += core

INCLUDEPATH *= $$BREAKPAD/include/breakpad
LIBS *= -L$$BREAKPAD/lib
LIBS *= -l$$qtLibraryName(breakpad_client)

SOURCES += libcrashreporter-handler/Handler.cpp
HEADERS += libcrashreporter-handler/Handler.h
