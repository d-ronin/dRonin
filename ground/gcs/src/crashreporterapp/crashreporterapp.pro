include(../../gcs.pri)
include(../../gcsversioninfo.pri)

QT += core gui network widgets

CONFIG -= app_bundle

TARGET = crashreporterapp
TEMPLATE = app
DESTDIR = $$GCS_APP_PATH
macx: DESTDIR = $$GCS_BIN_PATH

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS += mainwindow.h

FORMS += mainwindow.ui

RESOURCES += resources.qrc
