TOOLS_DIR = $$PWD/../tools

# if ccache is in CONFIG, use it
ccache {
    *-g++*|*-clang* {
        QMAKE_CC=$$(CCACHE_BIN) $$QMAKE_CC
        QMAKE_CXX=$$(CCACHE_BIN) $$QMAKE_CXX
    }
}

DR_QT_MAJOR_VERSION=5
DR_QT_MINOR_VERSION=9
DR_QT_PATCH_VERSION=2

# Must match make/tools.mk
BREAKPAD = $$TOOLS_DIR/breakpad/20170922
