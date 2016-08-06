TEMPLATE = lib
TARGET = Brotronics
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)

OTHER_FILES += Brotronics.pluginspec

HEADERS += \
    brotronicsplugin.h \
    lux.h \
    luxconfiguration.h \
    $$UAVOBJECT_SYNTHETICS/hwlux.h

SOURCES += \
    brotronicsplugin.cpp \
    lux.cpp \
    luxconfiguration.cpp \
    $$UAVOBJECT_SYNTHETICS/hwlux.cpp

RESOURCES += \
    brotronics.qrc

FORMS += \
	luxconfiguration.ui
