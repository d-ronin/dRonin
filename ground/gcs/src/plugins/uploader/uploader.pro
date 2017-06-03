TEMPLATE = lib
TARGET = Uploader
DEFINES += UPLOADER_LIBRARY
QT += svg widgets
QT += testlib
QT += network
include(../../gcsplugin.pri)
include(uploader_dependencies.pri)

CONFIG(release, debug|release):DEFINES += FIRMWARE_RELEASE_CONFIG

INCLUDEPATH *= ../../libs/glc_lib
HEADERS += uploadergadget.h \
    uploadergadgetfactory.h \
    uploadergadgetwidget.h \
    uploaderplugin.h \
    uploader_global.h \
    bl_messages.h \
    tl_dfu.h \
    upgradeassistantdialog.h

SOURCES += uploadergadget.cpp \
    uploadergadgetfactory.cpp \
    uploadergadgetwidget.cpp \
    uploaderplugin.cpp \
    upgradeassistantdialog.cpp \
    tl_dfu.cpp

OTHER_FILES += Uploader.pluginspec \
    Uploader.json

FORMS += \
    uploader.ui \
    upgradeassistant.ui

RESOURCES += \
    uploader.qrc
