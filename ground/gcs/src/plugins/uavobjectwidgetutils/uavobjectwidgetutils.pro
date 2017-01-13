TEMPLATE = lib
TARGET = UAVObjectWidgetUtils
DEFINES += UAVOBJECTWIDGETUTILS_LIBRARY
QT += svg

include(../../gcsplugin.pri)
include(uavobjectwidgetutils_dependencies.pri)

HEADERS += uavobjectwidgetutils_global.h \
    uavobjectwidgetutilsplugin.h \
    configtaskwidget.h \
    mixercurvewidget.h \
    mixercurvepoint.h \
    mixercurveline.h \
    smartsavebutton.h \
    popupwidget.h \
    connectiondiagram.h

SOURCES += uavobjectwidgetutilsplugin.cpp \
    configtaskwidget.cpp \
    mixercurvewidget.cpp \
    mixercurvepoint.cpp \
    mixercurveline.cpp \
    smartsavebutton.cpp \
    popupwidget.cpp \
    connectiondiagram.cpp

OTHER_FILES += UAVObjectWidgetUtils.pluginspec \
    UAVObjectWidgetUtils.json

FORMS += connectiondiagram.ui
