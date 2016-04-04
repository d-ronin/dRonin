TEMPLATE = lib
TARGET = Aggregation

include(../../gcslibrary.pri)

DEFINES += AGGREGATION_LIBRARY

HEADERS = aggregate.h \
    aggregation_global.h

SOURCES = aggregate.cpp

