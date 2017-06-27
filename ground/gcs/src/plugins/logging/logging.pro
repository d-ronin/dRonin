TEMPLATE = lib
TARGET = LoggingGadget
DEFINES += LOGGING_LIBRARY
QT += svg

include(../../gcsplugin.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/scope/scope.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavtalk/uavtalk.pri)

HEADERS += loggingplugin.h \
    logfile.h \
    logginggadgetwidget.h \
    logginggadget.h \
    logginggadgetfactory.h \
    loggingdevice.h \
    flightlogdownload.h

SOURCES += loggingplugin.cpp \
    logfile.cpp \
    logginggadgetwidget.cpp \
    logginggadget.cpp \
    logginggadgetfactory.cpp \
    loggingdevice.cpp \
    flightlogdownload.cpp

OTHER_FILES += LoggingGadget.pluginspec

FORMS += logging.ui \
    flightlogdownload.ui
