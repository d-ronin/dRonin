TEMPLATE = lib
TARGET = SystemHealthGadget
QT += svg

include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavtalk/uavtalk.pri)

HEADERS += systemhealthplugin.h
HEADERS += systemhealthgadget.h
HEADERS += systemhealthgadgetwidget.h
HEADERS += systemhealthgadgetfactory.h
HEADERS += systemhealthgadgetconfiguration.h
HEADERS += systemhealthgadgetoptionspage.h

SOURCES += systemhealthplugin.cpp
SOURCES += systemhealthgadget.cpp
SOURCES += systemhealthgadgetfactory.cpp
SOURCES += systemhealthgadgetwidget.cpp
SOURCES += systemhealthgadgetconfiguration.cpp
SOURCES += systemhealthgadgetoptionspage.cpp

OTHER_FILES += SystemHealthGadget.pluginspec

FORMS += systemhealthgadgetoptionspage.ui

RESOURCES += \
    systemhealth.qrc
