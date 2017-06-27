TEMPLATE = lib
TARGET = TauLinkGadget
DEFINES += TAULINK_LIBRARY
QT += svg

include(../../gcsplugin.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)
include(../../plugins/uavtalk/uavtalk.pri)

HEADERS += taulinkplugin.h \
    taulinkgadgetwidget.h \
    taulinkgadget.h \
    taulinkgadgetfactory.h

SOURCES += taulinkplugin.cpp \
    taulinkgadgetwidget.cpp \
    taulinkgadget.cpp \
    taulinkgadgetfactory.cpp

OTHER_FILES += TauLinkGadget.pluginspec

FORMS += taulink.ui
