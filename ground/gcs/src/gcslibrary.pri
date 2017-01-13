include(../gcs.pri)

win32 {
    DLLDESTDIR = $$GCS_APP_PATH
}

DESTDIR = $$GCS_LIBRARY_PATH

include(rpath.pri)

TARGET = $$qtLibraryName($$TARGET)

contains(QT_CONFIG, reduce_exports):CONFIG += hGCS_symbols

!macx {
    win32 {
        target.path = /bin
        target.files = $$DESTDIR/$${TARGET}.dll
    } else {
        target.path = /$$GCS_LIBRARY_BASENAME/dronin
    }
    INSTALLS += target
}

# copy pdb for breakpad and/or release debugging
win32-msvc* {
	debug|force_debug_info {
		QMAKE_POST_LINK += $(COPY_FILE) $(DESTDIR_TARGET:.dll=.pdb) $$shell_path($$DLLDESTDIR)
	}
}
