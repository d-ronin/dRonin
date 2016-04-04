TEMPLATE = lib
TARGET = BrainFPV
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)

OTHER_FILES += TheBrainFPV.pluginspec

HEADERS += \
    brainfpvplugin.h \
    brain.h \
    brainconfiguration.h

SOURCES += \
    brainfpvplugin.cpp \
    brain.cpp \
    brainconfiguration.cpp

RESOURCES += \
    brainfpv.qrc

FORMS += \
    brainconfiguration.ui
