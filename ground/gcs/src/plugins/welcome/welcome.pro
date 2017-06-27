TEMPLATE = lib
TARGET = Welcome
QT += network qml quick
DEFINES += WELCOME_LIBRARY

include(../../gcsplugin.pri)

include(../../libs/extensionsystem/extensionsystem.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../libs/utils/utils.pri)

HEADERS += welcomeplugin.h \
    welcomemode.h \
    welcome_global.h

SOURCES += welcomeplugin.cpp \
    welcomemode.cpp \

OTHER_FILES += Welcome.pluginspec

# Needed for bringing browser from background to foreground using QDesktopServices: http://bugreports.qt-project.org/browse/QTBUG-8336
TARGET.CAPABILITY += SwEvent
