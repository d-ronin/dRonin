TEMPLATE = lib
TARGET = UAVObjects
DEFINES += UAVOBJECTS_LIBRARY
QT += qml

include(../../gcsplugin.pri)

include(../../libs/extensionsystem/extensionsystem.pri)
include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)

include(uavobject_synthetics.pri)

HEADERS += uavobjects_global.h \
    uavobject.h \
    uavmetaobject.h \
    uavobjectmanager.h \
    uavdataobject.h \
    uavobjectfield.h \
    uavobjectsinit.h \
    uavobjectsplugin.h

SOURCES += uavobject.cpp \
    uavmetaobject.cpp \
    uavobjectmanager.cpp \
    uavdataobject.cpp \
    uavobjectfield.cpp \
    uavobjectsplugin.cpp

OTHER_FILES += UAVObjects.pluginspec

# Add in all of the synthetic/generated uavobject files
HEADERS += $$files($$UAVOBJECT_SYNTHETICS/*.h)
SOURCES += $$files($$UAVOBJECT_SYNTHETICS/*.cpp)
