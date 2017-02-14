TEMPLATE = lib
TARGET = AeroQuad
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../../usbids.pri)

OTHER_FILES += AeroQuad.json

HEADERS += \
    aeroquadplugin.h \
    aq32.h \
    $$USB_ID_HEADER

SOURCES += \
    aeroquadplugin.cpp \
    aq32.cpp

RESOURCES += \
    aeroquad.qrc
