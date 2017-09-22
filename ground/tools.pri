TOOLS_DIR = $$PWD/../tools

# If the PYTHON environment variable isn't set (by Make)
# then we set it ourselves.
PYTHON_LOCAL = $$(PYTHON)

isEmpty(PYTHON_LOCAL) {
    unix: PYTHON_LOCAL = python2
    win32: PYTHON_LOCAL = python
    macx: PYTHON_LOCAL = python
}

# if ccache is in CONFIG, use it
ccache {
    *-g++*|*-clang* {
        QMAKE_CC=$$(CCACHE_BIN) $$QMAKE_CC
        QMAKE_CXX=$$(CCACHE_BIN) $$QMAKE_CXX
    }
}

DR_QT_MAJOR_VERSION=5
DR_QT_MINOR_VERSION=8
DR_QT_PATCH_VERSION=0

# Must match make/tools.mk
BREAKPAD = $$TOOLS_DIR/breakpad/20170922
