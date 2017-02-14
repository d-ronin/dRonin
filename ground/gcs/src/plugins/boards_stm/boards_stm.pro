TEMPLATE = lib
TARGET = Stm
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../../usbids.pri)

OTHER_FILES += Stm.pluginspec

HEADERS += \
    stmplugin.h \
    discoveryf4.h \
    $$USB_ID_HEADER

SOURCES += \
    stmplugin.cpp \
    discoveryf4.cpp

RESOURCES += \
    stm.qrc

