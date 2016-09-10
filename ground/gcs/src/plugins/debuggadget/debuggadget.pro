TEMPLATE = lib
TARGET = DebugGadget
QT += widgets

include(../../gcsplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

DEFINES += DEBUGGADGET_LIBRARY

HEADERS += debugplugin.h \
    debugengine.h
HEADERS += debuggadget.h
HEADERS += debuggadgetwidget.h
HEADERS += debuggadgetfactory.h
SOURCES += debugplugin.cpp \
    debugengine.cpp
SOURCES += debuggadget.cpp
SOURCES += debuggadgetfactory.cpp
SOURCES += debuggadgetwidget.cpp

OTHER_FILES += DebugGadget.pluginspec \
               DebugGadget.json

FORMS += \
    debug.ui
