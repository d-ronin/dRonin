QT += network
TEMPLATE = lib
TARGET = UsageStats
DEFINES += USAGESTATS_LIBRARY

include(../../gcsplugin.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/debuggadget/debuggadget.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uploader/uploader.pri)

HEADERS += \
    usagestatsplugin.h \
    usagestatsoptionpage.h

SOURCES += \
    usagestatsplugin.cpp \
    usagestatsoptionpage.cpp

OTHER_FILES += UsageStats.pluginspec

FORMS += \
    usagestatsoptionpage.ui
