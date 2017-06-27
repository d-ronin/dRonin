TEMPLATE = lib
TARGET = UAVObjectWidgetUtils
DEFINES += UAVOBJECTWIDGETUTILS_LIBRARY
QT += svg

include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavsettingsimportexport/uavsettingsimportexport.pri)
include(../../plugins/uavtalk/uavtalk.pri)

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

OTHER_FILES += UAVObjectWidgetUtils.pluginspec

FORMS += connectiondiagram.ui
