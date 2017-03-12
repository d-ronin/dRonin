TEMPLATE = lib
TARGET = TauLabs
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)
include(../../../usbids.pri)

OTHER_FILES += TauLabs.json

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

FORMS += \
