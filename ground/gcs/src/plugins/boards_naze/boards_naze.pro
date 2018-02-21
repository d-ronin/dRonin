TEMPLATE = lib
TARGET = Naze
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../../usbids.pri)

OTHER_FILES += Naze.pluginspec

HEADERS += \
    nazeplugin.h \
    naze32pro.h

SOURCES += \
    nazeplugin.cpp \
    naze32pro.cpp

RESOURCES += \
    naze.qrc
