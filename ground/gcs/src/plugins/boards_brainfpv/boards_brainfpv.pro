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
    brainconfiguration.h \
    brainre1.h \
    brainre1configuration.h \
    $$UAVOBJECT_SYNTHETICS/hwbrain.h \
    $$UAVOBJECT_SYNTHETICS/hwbrainre1.h

SOURCES += \
    brainfpvplugin.cpp \
    brain.cpp \
    brainconfiguration.cpp \
    brainre1.cpp \
    brainre1configuration.cpp \
    $$UAVOBJECT_SYNTHETICS/hwbrain.cpp \
    $$UAVOBJECT_SYNTHETICS/hwbrainre1.cpp

RESOURCES += \
    brainfpv.qrc

FORMS += \
    brainconfiguration.ui \
    brainre1configuration.ui
