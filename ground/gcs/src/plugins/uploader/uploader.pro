TEMPLATE = lib
TARGET = Uploader
DEFINES += UPLOADER_LIBRARY
QT += svg widgets
QT += testlib
QT += network

include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/rawhid/rawhid.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavsettingsimportexport/uavsettingsimportexport.pri)
include(../../plugins/uavtalk/uavtalk.pri)

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

OTHER_FILES += Uploader.pluginspec

FORMS += \
    uploader.ui \
    upgradeassistant.ui

RESOURCES += \
    uploader.qrc
