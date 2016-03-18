
TEMPLATE = lib
QT += xml
QT += widgets
QT += xmlpatterns
TARGET = UAVSettingsImportExport
DEFINES += UAVSETTINGSIMPORTEXPORT_LIBRARY
include(../../gcsplugin.pri)
include(uavsettingsimportexport_dependencies.pri)

HEADERS += uavsettingsimportexport.h \
    importsummary.h \
    uavsettingsimportexportmanager.h
SOURCES += uavsettingsimportexport.cpp \
    importsummary.cpp \
    uavsettingsimportexportmanager.cpp

OTHER_FILES += uavsettingsimportexport.pluginspec \
    uavsettingsimportexport.json

FORMS += \
    importsummarydialog.ui
