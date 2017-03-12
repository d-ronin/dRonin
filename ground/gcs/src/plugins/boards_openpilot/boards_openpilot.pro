TEMPLATE = lib
TARGET = OpenPilot
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)
include(../../../usbids.pri)

OTHER_FILES += OpenPilot.pluginspec \
                OpenPilot.json

HEADERS += \
    openpilotplugin.h \
    revolution.h

SOURCES += \
    openpilotplugin.cpp \
    revolution.cpp

RESOURCES += \
    openpilot.qrc \
    ../coreplugin/core.qrc
