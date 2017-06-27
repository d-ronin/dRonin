TEMPLATE = lib
TARGET = ModelViewGadget

include(../../gcsplugin.pri)

include(../../libs/glc_lib/glc_lib.pri)
include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)

HEADERS += modelviewplugin.h \
    modelviewgadgetconfiguration.h \
    modelviewgadget.h \
    modelviewgadgetwidget.h \
    modelviewgadgetfactory.h \
    modelviewgadgetoptionspage.h

SOURCES += modelviewplugin.cpp \
    modelviewgadgetconfiguration.cpp \
    modelviewgadget.cpp \
    modelviewgadgetfactory.cpp \
    modelviewgadgetwidget.cpp \
    modelviewgadgetoptionspage.cpp

OTHER_FILES += ModelViewGadget.pluginspec

FORMS += modelviewoptionspage.ui

RESOURCES += \
    modelview.qrc

win32-msvc* {
    LIBS += opengl32.lib
} else:win32 {
    LIBS += -lopengl32
}
