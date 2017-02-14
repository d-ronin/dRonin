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
    cc3d.h \
    config_cc_hw_widget.h \
    revolution.h \
    $$USB_ID_HEADER

SOURCES += \
    openpilotplugin.cpp \
    cc3d.cpp \
    config_cc_hw_widget.cpp \
    revolution.cpp

RESOURCES += \
    openpilot.qrc \
    ../coreplugin/core.qrc

FORMS += \
    cc_hw_settings.ui
