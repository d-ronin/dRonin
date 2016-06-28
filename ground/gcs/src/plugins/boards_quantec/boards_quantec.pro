TEMPLATE = lib
TARGET = Quantec
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)

OTHER_FILES += Quantec.pluginspec

HEADERS += \
    quantecplugin.h \
    quanton.h \
    $$UAVOBJECT_SYNTHETICS/hwquanton.h

SOURCES += \
    quantecplugin.cpp \
    quanton.cpp \
    $$UAVOBJECT_SYNTHETICS/hwquanton.cpp

RESOURCES += \
    quantec.qrc
