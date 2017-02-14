TEMPLATE = lib
TARGET = BrainFPV
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)
include(../../../usbids.pri)

OTHER_FILES += TheBrainFPV.pluginspec

HEADERS += \
    brainfpvplugin.h \
    brain.h \
    brainconfiguration.h \
    brainre1.h \
    brainre1configuration.h \
    $$USB_ID_HEADER

SOURCES += \
    brainfpvplugin.cpp \
    brain.cpp \
    brainconfiguration.cpp \
    brainre1.cpp \
    brainre1configuration.cpp

RESOURCES += \
    brainfpv.qrc

FORMS += \
    brainconfiguration.ui \
    brainre1configuration.ui
