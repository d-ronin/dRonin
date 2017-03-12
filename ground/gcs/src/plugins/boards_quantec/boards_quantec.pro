TEMPLATE = lib
TARGET = Quantec
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../../usbids.pri)

OTHER_FILES += Quantec.pluginspec

HEADERS += \
    quantecplugin.h \
    quanton.h

SOURCES += \
    quantecplugin.cpp \
    quanton.cpp

RESOURCES += \
    quantec.qrc
