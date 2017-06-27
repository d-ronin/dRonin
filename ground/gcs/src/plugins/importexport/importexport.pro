TEMPLATE = lib
TARGET = ImportExportGadget
QT += widgets
QT += xml

DEFINES += IMPORTEXPORT_LIBRARY

include(../../gcsplugin.pri)

include(../../libs/extensionsystem/extensionsystem.pri)
include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += importexportplugin.h \
    importexportgadgetwidget.h \
    importexportdialog.h

SOURCES += importexportplugin.cpp \
    importexportgadgetwidget.cpp \
    importexportdialog.cpp

OTHER_FILES += ImportExportGadget.pluginspec

FORMS += importexportgadgetwidget.ui \
    importexportdialog.ui
