cache()

include(gcs.pri)
!build_pass:message("QMAKE_CXX = $$QMAKE_CXX")
!build_pass:message("QMAKE_CC = $$QMAKE_CC")

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src share copydata
unix:!macx:!isEmpty(copydata) {
	bin_install.target = FORCE
	bin_install.commands = cp $$GCS_SOURCE_TREE/bin/gcs $$GCS_APP_PATH/$$GCS_APP_WRAPPER
	QMAKE_EXTRA_TARGETS += bin_install
}

!equals(QT_MAJOR_VERSION, $$DR_QT_MAJOR_VERSION) | !equals(QT_MINOR_VERSION, $$DR_QT_MINOR_VERSION) | !equals(QT_PATCH_VERSION, $$DR_QT_PATCH_VERSION) {
    error("Qt $${DR_QT_MAJOR_VERSION}.$${DR_QT_MINOR_VERSION}.$${DR_QT_PATCH_VERSION} required. Please run 'make qt_sdk_install'.")
}

copydata.file = copydata.pro
