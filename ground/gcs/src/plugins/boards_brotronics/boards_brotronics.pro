TEMPLATE = lib
TARGET = Brotronics
include(../../taulabsgcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)

OTHER_FILES += Brotronics.pluginspec

HEADERS += \
    brotronicsplugin.h \
    acbro.h

SOURCES += \
    brotronicsplugin.cpp \
    acbro.cpp

RESOURCES += \
    brotronics.qrc
