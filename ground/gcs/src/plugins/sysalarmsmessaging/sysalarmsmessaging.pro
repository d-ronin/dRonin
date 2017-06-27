TEMPLATE = lib
TARGET = SysAlarmsMessaging
QT += svg

include(../../gcsplugin.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavtalk/uavtalk.pri)

HEADERS += sysalarmsmessagingplugin.h

SOURCES += sysalarmsmessagingplugin.cpp

OTHER_FILES += SysAlarmsMessaging.pluginspec
