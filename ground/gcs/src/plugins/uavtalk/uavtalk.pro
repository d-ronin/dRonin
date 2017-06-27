QT += network
QT += widgets
TEMPLATE = lib
TARGET = UAVTalk
DEFINES += UAVTALK_LIBRARY

include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += uavtalk.h \
    uavtalkplugin.h \
    telemetrymonitor.h \
    telemetrymanager.h \
    uavtalk_global.h \
    telemetry.h

SOURCES += uavtalk.cpp \
    uavtalkplugin.cpp \
    telemetrymonitor.cpp \
    telemetrymanager.cpp \
    telemetry.cpp

OTHER_FILES += UAVTalk.pluginspec
