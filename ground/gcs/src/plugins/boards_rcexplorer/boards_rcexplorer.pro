TEMPLATE = lib
TARGET = RcExplorer
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../../usbids.pri)

OTHER_FILES += RcExplorer.pluginspec

HEADERS += \
    rcexplorerplugin.h \
    f3fc.h

SOURCES += \
    rcexplorerplugin.cpp \
    f3fc.cpp

RESOURCES += \
    rcexplorer.qrc
