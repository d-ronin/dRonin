TEMPLATE = lib
TARGET = PfdQml
QT += svg
QT += qml
QT += quick

include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += \
    pfdqmlplugin.h \
    pfdqmlgadget.h \
    pfdqmlgadgetwidget.h \
    pfdqmlgadgetfactory.h \
    pfdqmlgadgetconfiguration.h \
    pfdqmlgadgetoptionspage.h

SOURCES += \
    pfdqmlplugin.cpp \
    pfdqmlgadget.cpp \
    pfdqmlgadgetfactory.cpp \
    pfdqmlgadgetwidget.cpp \
    pfdqmlgadgetconfiguration.cpp \
    pfdqmlgadgetoptionspage.cpp

OTHER_FILES += PfdQml.pluginspec

FORMS += pfdqmlgadgetoptionspage.ui

