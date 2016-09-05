QT += network
TEMPLATE = lib
TARGET = UsageStats
DEFINES += USAGESTATS_LIBRARY
include(../../gcsplugin.pri)
include(usagestats_dependencies.pri)
HEADERS += \
    usagestatsplugin.h \
    usagestatsoptionpage.h
SOURCES += \
    usagestatsplugin.cpp \
    usagestatsoptionpage.cpp

OTHER_FILES += UsageStats.pluginspec \
    UsageStats.json

FORMS += \
    usagestatsoptionpage.ui
