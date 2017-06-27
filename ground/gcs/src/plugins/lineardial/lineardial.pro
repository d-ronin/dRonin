TEMPLATE = lib
TARGET = LineardialGadget
QT += svg

include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += lineardialplugin.h
HEADERS += lineardialgadget.h
HEADERS += lineardialgadgetwidget.h
HEADERS += lineardialgadgetfactory.h
HEADERS += lineardialgadgetconfiguration.h
HEADERS += lineardialgadgetoptionspage.h

SOURCES += lineardialplugin.cpp
SOURCES += lineardialgadget.cpp
SOURCES += lineardialgadgetfactory.cpp
SOURCES += lineardialgadgetwidget.cpp
SOURCES += lineardialgadgetconfiguration.cpp
SOURCES += lineardialgadgetoptionspage.cpp

OTHER_FILES += LineardialGadget.pluginspec

FORMS += lineardialgadgetoptionspage.ui

RESOURCES += lineardial.qrc
