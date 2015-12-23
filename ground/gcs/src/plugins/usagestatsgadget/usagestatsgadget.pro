QT += network
TEMPLATE = lib
TARGET = UsageStats
include(../../taulabsgcsplugin.pri)
include(usagestats_dependencies.pri)
HEADERS += \
    usagestatsplugin.h \
    usagestatsoptionpage.h
SOURCES += \
    usagestatsplugin.cpp \
    usagestatsoptionpage.cpp
DEFINES += USAGESTATS_LIBRARY
OTHER_FILES += UsageStats.pluginspec \
    UsageStats.json

FORMS += \
    usagestatsoptionpage.ui
