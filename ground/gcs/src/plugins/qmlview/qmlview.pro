TEMPLATE = lib
TARGET = QMLView
QT += svg
QT += qml
QT += quick

include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += \
    qmlviewplugin.h \
    qmlviewgadget.h \
    qmlviewgadgetwidget.h \
    qmlviewgadgetfactory.h \
    qmlviewgadgetconfiguration.h \
    qmlviewgadgetoptionspage.h


SOURCES += \
    qmlviewplugin.cpp \
    qmlviewgadget.cpp \
    qmlviewgadgetfactory.cpp \
    qmlviewgadgetwidget.cpp \
    qmlviewgadgetconfiguration.cpp \
    qmlviewgadgetoptionspage.cpp

OTHER_FILES += QMLView.pluginspec

FORMS += qmlviewgadgetoptionspage.ui

