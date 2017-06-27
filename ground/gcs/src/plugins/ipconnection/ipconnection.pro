TEMPLATE = lib
QT += widgets network
TARGET = IPconnection

include(../../gcsplugin.pri)

include(../../libs/extensionsystem/extensionsystem.pri)

include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += ipconnectionplugin.h \
    ipconnection_global.h \
    ipconnectionconfiguration.h \
    ipconnectionoptionspage.h \
    ipdevice.h

SOURCES += ipconnectionplugin.cpp \
    ipconnectionconfiguration.cpp \
    ipconnectionoptionspage.cpp \
    ipdevice.cpp

FORMS += ipconnectionoptionspage.ui

DEFINES += IPconnection_LIBRARY

OTHER_FILES += IPconnection.pluginspec
