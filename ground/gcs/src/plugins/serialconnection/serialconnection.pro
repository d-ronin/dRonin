TEMPLATE = lib
TARGET = Serial
QT += widgets
QT += serialport

include(../../gcsplugin.pri)

include(../../libs/extensionsystem/extensionsystem.pri)

include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += serialplugin.h \
            serialpluginconfiguration.h \
            serialpluginoptionspage.h \
            serialdevice.h
SOURCES += serialplugin.cpp \
            serialpluginconfiguration.cpp \
            serialpluginoptionspage.cpp \
            serialdevice.cpp
FORMS += \
    serialpluginoptions.ui

RESOURCES +=

OTHER_FILES += Serial.pluginspec
