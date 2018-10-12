TEMPLATE = lib
TARGET = Holybro
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../../usbids.pri)

OTHER_FILES += Holybro.pluginspec

HEADERS += \
    holybroplugin.h \
    kakutef4v2.h

SOURCES += \
    holybroplugin.cpp \
    kakutef4v2.cpp

RESOURCES += \
    holybro.qrc
