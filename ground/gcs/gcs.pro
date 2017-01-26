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

!equals(QT_MAJOR_VERSION, 5) | !equals(QT_MINOR_VERSION, 6) | !equals(QT_PATCH_VERSION, 2) {
        message("Cannot build dRonin GCS with Qt version $${QT_VERSION}.")
        error("Use 5.6.2 (make qt_sdk_install).")
}

copydata.file = copydata.pro
