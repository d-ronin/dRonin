TEMPLATE = lib
TARGET = TauLabs
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)

OTHER_FILES += TauLabs.json

HEADERS += \
    taulabsplugin.h \
    sparky.h \
    sparky2.h \
    taulink.h \
    $$UAVOBJECT_SYNTHETICS/hwsparky.h \
    $$UAVOBJECT_SYNTHETICS/hwsparky2.h \
    $$UAVOBJECT_SYNTHETICS/hwtaulink.h

SOURCES += \
    taulabsplugin.cpp \
    sparky.cpp \
    sparky2.cpp \
    taulink.cpp \
    $$UAVOBJECT_SYNTHETICS/hwsparky.cpp \
    $$UAVOBJECT_SYNTHETICS/hwsparky2.cpp \
    $$UAVOBJECT_SYNTHETICS/hwtaulink.cpp

RESOURCES += \
    taulabs.qrc

FORMS += \
