include(../../gcs.pri)
QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = crashreporterapp
TEMPLATE = app
DESTDIR = $$GCS_APP_PATH

#include(../libs/utils/utils.pri)

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

GIT_COMMIT = $$system(git rev-parse -q --short HEAD)
GIT_BRANCH = $$system(git symbolic-ref -q --short HEAD)
GIT_TAG    = $$system(git name-rev --tags --name-only HEAD)
GIT_DIRTY  = true
isEmpty($$system(git diff-files --shortstat)) {
	GIT_DIRTY = false
}

DEFINES += GIT_COMMIT=\\\"$$GIT_COMMIT\\\" \
           GIT_BRANCH=\\\"$$GIT_BRANCH\\\" \
           GIT_TAG=\\\"$$GIT_TAG\\\" \
           GIT_DIRTY=\\\"$$GIT_DIRTY\\\"
