TEMPLATE = lib
QT += widgets
TARGET = UAVObjectUtil
DEFINES += UAVOBJECTUTIL_LIBRARY

include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += uavobjectutil_global.h \
    uavobjectutilmanager.h \
    uavobjectutilplugin.h \
    devicedescriptorstruct.h

SOURCES += uavobjectutilmanager.cpp \
    uavobjectutilplugin.cpp \
    devicedescriptorstruct.cpp

OTHER_FILES += UAVObjectUtil.pluginspec
