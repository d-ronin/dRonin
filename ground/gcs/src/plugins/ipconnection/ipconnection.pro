TEMPLATE = lib
QT += widgets network
TARGET = IPconnection
include(../../gcsplugin.pri)
include(ipconnection_dependencies.pri)
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
RESOURCES += 
DEFINES += IPconnection_LIBRARY
OTHER_FILES += IPconnection.pluginspec \
    IPconnection.json
QT += network
