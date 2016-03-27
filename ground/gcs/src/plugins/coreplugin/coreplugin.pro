TEMPLATE = lib
TARGET = Core
DEFINES += CORE_LIBRARY

QT += widgets
QT += xml \
    network \
    svg \
    sql
include(../../gcsplugin.pri)
include(../../libs/utils/utils.pri)
include(coreplugin_dependencies.pri)
INCLUDEPATH *= dialogs \
    uavgadgetmanager \
    actionmanager
DEPENDPATH += dialogs \
    uavgadgetmanager \
    actionmanager
SOURCES += mainwindow.cpp \
    generalsettings.cpp \
    uniqueidmanager.cpp \
    messagemanager.cpp \
    messageoutputwindow.cpp \
    versiondialog.cpp \
    iuavgadget.cpp \
    uavgadgetmanager/uavgadgetmanager.cpp \
    uavgadgetmanager/uavgadgetview.cpp \
    uavgadgetmanager/splitterorview.cpp \
    actionmanager/actionmanager.cpp \
    actionmanager/command.cpp \
    actionmanager/actioncontainer.cpp \
    actionmanager/commandsfile.cpp \
    dialogs/settingsdialog.cpp \
    dialogs/shortcutsettings.cpp \
    basemode.cpp \
    baseview.cpp \
    coreplugin.cpp \
    modemanager.cpp \
    coreimpl.cpp \
    plugindialog.cpp \
    manhattanstyle.cpp \
    minisplitter.cpp \
    styleanimator.cpp \
    icore.cpp \
    dialogs/ioptionspage.cpp \
    dialogs/iwizard.cpp \
    eventfilteringmainwindow.cpp \
    connectionmanager.cpp \
    iconnection.cpp \
    iuavgadgetconfiguration.cpp \
    uavgadgetinstancemanager.cpp \
    uavgadgetoptionspagedecorator.cpp \
    uavgadgetdecorator.cpp \
    workspacesettings.cpp \
    uavconfiginfo.cpp \
    authorsdialog.cpp \
    telemetrymonitorwidget.cpp \
    dialogs/importsettings.cpp \
    boardmanager.cpp \
    iboardtype.cpp \
    idevice.cpp \
    globalmessaging.cpp \
    alarmsmonitorwidget.cpp
HEADERS += mainwindow.h \
    generalsettings.h \
    uniqueidmanager.h \
    messagemanager.h \
    messageoutputwindow.h \
    iuavgadget.h \
    iuavgadgetfactory.h \
    uavgadgetmanager/uavgadgetmanager.h \
    uavgadgetmanager/uavgadgetview.h \
    uavgadgetmanager/splitterorview.h \
    actionmanager/actioncontainer.h \
    actionmanager/actionmanager.h \
    actionmanager/command.h \
    actionmanager/actionmanager_p.h \
    actionmanager/command_p.h \
    actionmanager/actioncontainer_p.h \
    actionmanager/commandsfile.h \
    dialogs/settingsdialog.h \
    dialogs/shortcutsettings.h \
    dialogs/iwizard.h \
    dialogs/ioptionspage.h \
    icontext.h \
    icore.h \
    imode.h \
    ioutputpane.h \
    coreconstants.h \
    iversioncontrol.h \
    iview.h \
    icorelistener.h \
    versiondialog.h \
    core_global.h \
    basemode.h \
    baseview.h \
    coreplugin.h \
    modemanager.h \
    coreimpl.h \
    plugindialog.h \
    manhattanstyle.h \
    styleanimator.h \
    minisplitter.h \
    eventfilteringmainwindow.h \
    connectionmanager.h \
    iconnection.h \
    iuavgadgetconfiguration.h \
    uavgadgetinstancemanager.h \
    uavgadgetoptionspagedecorator.h \
    uavgadgetdecorator.h \
    workspacesettings.h \
    uavconfiginfo.h \
    authorsdialog.h \
    iconfigurableplugin.h \
    telemetrymonitorwidget.h \
    dialogs/importsettings.h \
    boardmanager.h \
    iboardtype.h \
    idevice.h \
    globalmessaging.h \
    alarmsmonitorwidget.h
FORMS += dialogs/settingsdialog.ui \
    dialogs/shortcutsettings.ui \
    generalsettings.ui \
    uavgadgetoptionspage.ui \
    workspacesettings.ui \
    dialogs/importsettings.ui
RESOURCES += core.qrc
unix:!macx { 
    images.files = $${GCS_PROJECT_BRANDING_PRETTY}/gcs_logo_*.png
    images.files = images/qtcreator_logo_*.png
    images.path = /share/pixmaps
    INSTALLS += images
}
OTHER_FILES += Core.pluginspec \
    coreplugin.json

include(gcsversioninfo.pri)
