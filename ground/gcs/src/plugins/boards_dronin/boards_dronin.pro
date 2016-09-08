TEMPLATE = lib
TARGET = Dronin
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)

OTHER_FILES += Dronin.pluginspec

HEADERS += \
    droninplugin.h \
    simulation.h \
    simulationconfiguration.h \
    playuavosd.h

SOURCES += \
    droninplugin.cpp \
    simulation.cpp \
    simulationconfiguration.cpp \
    playuavosd.cpp

RESOURCES += \
    dronin.qrc

FORMS += \
	simulationconfiguration.ui
