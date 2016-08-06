TEMPLATE = lib
TARGET = DTFUHF
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)

OTHER_FILES += dtfuhf.pluginspec

HEADERS += \
    dtfplugin.h \
    dtfc.h \
    dtfcconfiguration.h \
    $$UAVOBJECT_SYNTHETICS/hwdtfc.h

SOURCES += \
    dtfplugin.cpp \
    dtfc.cpp \
    dtfcconfiguration.cpp \
    $$UAVOBJECT_SYNTHETICS/hwdtfc.cpp

RESOURCES += \
    dtfuhf.qrc

FORMS += \
    dtfcconfiguration.ui
