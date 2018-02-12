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
GIT_BRANCH = $$system(git symbolic-ref -q --short HEAD)
GIT_TAG    = $$system(git name-rev --tags --name-only --no-undefined HEAD 2>/dev/null)
GIT_DIRTY  = true
system(git diff-index --quiet HEAD --): GIT_DIRTY = false

VERSION_INFO = $$cat($$system_path($$PWD/version.template.h), blob)
VERSION_INFO = $$replace(VERSION_INFO, __COMMIT__, $$GIT_COMMIT)
VERSION_INFO = $$replace(VERSION_INFO, __BRANCH__, $$GIT_BRANCH)
VERSION_INFO = $$replace(VERSION_INFO, __TAG__, $$GIT_TAG)
VERSION_INFO = $$replace(VERSION_INFO, __DIRTY__, $$GIT_DIRTY)
VERSION_INFO = $$replace(VERSION_INFO, __CI_BUILD_TAG__, $$(BUILD_TAG))
VERSION_INFO = $$replace(VERSION_INFO, __CI_NODE__, $$(NODE_NAME))
write_file($$OUT_PWD/version.h, VERSION_INFO)
