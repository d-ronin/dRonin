TEMPLATE = lib
QT += multimedia
TARGET = NotifyPlugin
 
include(../../gcsplugin.pri)

include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavtalk/uavtalk.pri)

HEADERS += notifyplugin.h \
    notifypluginoptionspage.h \
    notifyitemdelegate.h \
    notifytablemodel.h \
    notificationitem.h \
    notifylogging.h

SOURCES += notifyplugin.cpp \
    notifypluginoptionspage.cpp \
    notifyitemdelegate.cpp \
    notifytablemodel.cpp \
    notificationitem.cpp \
    notifylogging.cpp
 
OTHER_FILES += NotifyPlugin.pluginspec

FORMS += \
    notifypluginoptionspage.ui

RESOURCES += \
    res.qrc
