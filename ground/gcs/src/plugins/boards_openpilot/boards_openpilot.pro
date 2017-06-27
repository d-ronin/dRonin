TEMPLATE = lib
TARGET = OpenPilot

include(../../gcsplugin.pri)
include(../../../usbids.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)

OTHER_FILES += OpenPilot.pluginspec

HEADERS += \
    openpilotplugin.h \
    revolution.h

SOURCES += \
    openpilotplugin.cpp \
    revolution.cpp

RESOURCES += \
    openpilot.qrc \
    ../coreplugin/core.qrc
