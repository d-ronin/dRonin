TEMPLATE = lib
TARGET = Utils

!win32-msvc*:QMAKE_CXXFLAGS += -Wno-sign-compare
win32: QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings

QT += gui \
    network \
    xml \
    svg \
    opengl \
    qml \
    quick \
    widgets

DEFINES += QTCREATOR_UTILS_LIB

include(../../gcslibrary.pri)

SOURCES += \
    pathchooser.cpp \
    hostosinfo.cpp \
    basevalidatinglineedit.cpp \
    qtcolorbutton.cpp \
    treewidgetcolumnstretcher.cpp \
    styledbar.cpp \
    coordinateconversions.cpp \
    pathutils.cpp \
    worldmagmodel.cpp \
    homelocationutil.cpp \
    mytabbedstackwidget.cpp \
    mytabwidget.cpp \
    svgimageprovider.cpp

SOURCES += xmlconfig.cpp

win32 {
    SOURCES += \
        winutils.cpp
    HEADERS += winutils.h
}
else:SOURCES +=

HEADERS += utils_global.h \
    pathchooser.h \
    hostosinfo.h \
    basevalidatinglineedit.h \
    qtcolorbutton.h \
    treewidgetcolumnstretcher.h \
    qtcassert.h \
    styledbar.h \
    coordinateconversions.h \
    pathutils.h \
    worldmagmodel.h \
    homelocationutil.h \
    mytabbedstackwidget.h \
    mytabwidget.h \
    svgimageprovider.h


HEADERS += xmlconfig.h

FORMS +=

RESOURCES += utils.qrc
