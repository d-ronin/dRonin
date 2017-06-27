TEMPLATE = lib
QT += widgets
TARGET = UAVObjectBrowser

include(../../gcsplugin.pri)

include(../../libs/qscispinbox/qscispinbox.pri)
include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += browserplugin.h \
    uavobjectbrowserconfiguration.h \
    uavobjectbrowser.h \
    uavobjectbrowserwidget.h \
    uavobjectbrowserfactory.h \
    uavobjectbrowseroptionspage.h \
    uavobjecttreemodel.h \
    treeitem.h \
    browseritemdelegate.h \
    fieldtreeitem.h

SOURCES += browserplugin.cpp \
    uavobjectbrowserconfiguration.cpp \
    uavobjectbrowser.cpp \
    uavobjectbrowserfactory.cpp \
    uavobjectbrowserwidget.cpp \
    uavobjectbrowseroptionspage.cpp \
    uavobjecttreemodel.cpp \
    treeitem.cpp \
    browseritemdelegate.cpp \
    fieldtreeitem.cpp

OTHER_FILES += UAVObjectBrowser.pluginspec

FORMS += uavobjectbrowser.ui \
    uavobjectbrowseroptionspage.ui \
    viewoptions.ui

RESOURCES += \
    uavobjectbrowser.qrc
