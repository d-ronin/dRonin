TEMPLATE = lib
TARGET = DialGadget
QT += svg

include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += dialplugin.h
HEADERS += dialgadget.h
HEADERS += dialgadgetwidget.h
HEADERS += dialgadgetfactory.h
HEADERS += dialgadgetconfiguration.h
HEADERS += dialgadgetoptionspage.h

SOURCES += dialplugin.cpp
SOURCES += dialgadget.cpp
SOURCES += dialgadgetfactory.cpp
SOURCES += dialgadgetwidget.cpp
SOURCES += dialgadgetconfiguration.cpp
SOURCES += dialgadgetoptionspage.cpp

OTHER_FILES += DialGadget.pluginspec

FORMS += dialgadgetoptionspage.ui

RESOURCES += dial.qrc
