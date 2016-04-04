TEMPLATE = lib 
TARGET = RfmBindWizard
QT += svg


include(../../gcsplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)
include(../../plugins/config/config.pri)

LIBS *= -l$$qtLibraryName(Uploader)
HEADERS += rfmbindwizardplugin.h \ 
    rfmbindwizard.h \
    pages/abstractwizardpage.h \
    pages/radioprobepage.h \
    pages/tlendpage.h \
    pages/tlstartpage.h \
    pages/configurepage.h \
    pages/coordinatorpage.h \
    pages/coordinatedpage.h

SOURCES += rfmbindwizardplugin.cpp \
    rfmbindwizard.cpp \
    pages/abstractwizardpage.cpp \
    pages/radioprobepage.cpp \
    pages/tlendpage.cpp \
    pages/tlstartpage.cpp \
    pages/configurepage.cpp \
    pages/coordinatorpage.cpp \
    pages/coordinatedpage.cpp

OTHER_FILES += RfmBindWizard.pluginspec \
    RfmBindWizard.json

FORMS += \
    pages/startpage.ui \
    pages/endpage.ui \
    pages/configurepage.ui \
    pages/coordinatorpage.ui \
    pages/coordinatedpage.ui

