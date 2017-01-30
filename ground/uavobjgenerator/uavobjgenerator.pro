include(../tools.pri)

QT += xml
QT -= gui

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET=10.9
}

# we are a bit looser on Qt release for uavogen
# lets doc builders etc. use PPAs which always carry only latest patch release
!equals(QT_MAJOR_VERSION, $$DR_QT_MAJOR_VERSION) | !equals(QT_MINOR_VERSION, $$DR_QT_MINOR_VERSION) | lessThan(QT_PATCH_VERSION, $$DR_QT_PATCH_VERSION) {
    error("Qt $${DR_QT_MAJOR_VERSION}.$${DR_QT_MINOR_VERSION}.$${DR_QT_PATCH_VERSION}+ required. Please run 'make qt_sdk_install'.")
}

cache()

TARGET = uavobjgenerator
CONFIG += console
# use ISO C++ (no GNU extensions) to ensure maximum portability
CONFIG += c++11 strict_c++
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    uavobjectparser.cpp \
    generators/generator_io.cpp \
    generators/java/uavobjectgeneratorjava.cpp \
    generators/flight/uavobjectgeneratorflight.cpp \
    generators/gcs/uavobjectgeneratorgcs.cpp \
    generators/matlab/uavobjectgeneratormatlab.cpp \
    generators/wireshark/uavobjectgeneratorwireshark.cpp \
    generators/generator_common.cpp
HEADERS += uavobjectparser.h \
    generators/generator_io.h \
    generators/java/uavobjectgeneratorjava.h \
    generators/gcs/uavobjectgeneratorgcs.h \
    generators/matlab/uavobjectgeneratormatlab.h \
    generators/wireshark/uavobjectgeneratorwireshark.h \
    generators/generator_common.h
