TEMPLATE = lib
TARGET = Naze
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)

OTHER_FILES += Naze.pluginspec

HEADERS += \
    nazeplugin.h \
    naze.h \
    $$UAVOBJECT_SYNTHETICS/hwnaze.h

SOURCES += \
    nazeplugin.cpp \
    naze.cpp \
    $$UAVOBJECT_SYNTHETICS/hwnaze.cpp

RESOURCES += \
    naze.qrc
