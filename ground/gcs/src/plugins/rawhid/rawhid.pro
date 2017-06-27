TEMPLATE = lib
TARGET = RawHID
DEFINES += RAWHID_LIBRARY

include(../../gcsplugin.pri)

include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += rawhid_global.h \
    rawhidplugin.h \
    rawhid.h \
    hidapi/hidapi.h \
    rawhid_const.h \
    usbmonitor.h \
    usbsignalfilter.h \
    usbdevice.h

SOURCES += rawhidplugin.cpp \
    rawhid.cpp \
    usbsignalfilter.cpp \
    usbdevice.cpp \
    usbmonitor.cpp

OTHER_FILES += RawHID.pluginspec

# Platform Specific USB HID Stuff
win32 { 
    SOURCES += hidapi/hidapi_windows.c 
    LIBS += -lhid \
        -lsetupapi
        win32-msvc* {
            LIBS += -lUser32
        }
}
macx { 
    SOURCES += hidapi/hidapi_mac.c
    LIBS += -framework IOKit \
        -framework CoreFoundation
}
linux {
    SOURCES += hidapi/hidapi_linux.c
    LIBS += -ludev -lrt
}
