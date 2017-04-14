include(../../gcs.pri)
QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG-=app_bundle

TARGET = crashreporterapp
TEMPLATE = app
DESTDIR = $$GCS_APP_PATH
macx {
DESTDIR = $$GCS_BIN_PATH
}

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

# TODO: should really use the GCS revision rather than crashreporterapp
# (it is possible to build it seperately, then the results are misleading)
GIT_COMMIT = $$system(git rev-parse -q --short HEAD)
GIT_BRANCH = $$system(git for-each-ref --count=1 --format=$$system_quote(%(refname:strip=2)) --contains HEAD refs/heads)
GIT_TAG    = $$system(git for-each-ref --count=1 --format=$$system_quote(%(refname:strip=2)) --points-at HEAD refs/tags)
GIT_DIFF   = $$system(git status --porcelain)
GIT_DIRTY  = true
isEmpty(GIT_DIFF): GIT_DIRTY = false

DEFINES += GIT_COMMIT=\\\"$$GIT_COMMIT\\\" \
           GIT_BRANCH=\\\"$$GIT_BRANCH\\\" \
           GIT_TAG=\\\"$$GIT_TAG\\\" \
           GIT_DIRTY=\\\"$$GIT_DIRTY\\\"
