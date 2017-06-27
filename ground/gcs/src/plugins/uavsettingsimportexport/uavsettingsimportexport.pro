TEMPLATE = lib
QT += xml
QT += widgets
QT += xmlpatterns
TARGET = UAVSettingsImportExport
DEFINES += UAVSETTINGSIMPORTEXPORT_LIBRARY

include(../../gcsplugin.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavtalk/uavtalk.pri)

HEADERS += uavsettingsimportexport.h \
    importsummary.h \
    uavsettingsimportexportmanager.h

SOURCES += uavsettingsimportexport.cpp \
    importsummary.cpp \
    uavsettingsimportexportmanager.cpp

OTHER_FILES += uavsettingsimportexport.pluginspec

FORMS += \
    importsummarydialog.ui
