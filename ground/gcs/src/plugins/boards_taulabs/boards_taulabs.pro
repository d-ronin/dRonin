TEMPLATE = lib
TARGET = TauLabs

include(../../gcsplugin.pri)
include(../../../usbids.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)

OTHER_FILES += TauLabs.pluginspec

HEADERS += \
    taulabsplugin.h \
    sparky.h \
    sparky2.h \
    taulink.h

SOURCES += \
    taulabsplugin.cpp \
    sparky.cpp \
    sparky2.cpp \
    taulink.cpp

RESOURCES += \
    taulabs.qrc
