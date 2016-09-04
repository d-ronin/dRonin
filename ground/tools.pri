# If the PYTHON environment variable isn't set (by Make)
# then we set it ourselves.
PYTHON_LOCAL = $$(PYTHON)

isEmpty(PYTHON_LOCAL) {
    unix: PYTHON_LOCAL = python2
    win32: PYTHON_LOCAL = python
    macx: PYTHON_LOCAL = python
}

# use ccache with gcc and clang in debug config
CONFIG(debug, debug|release):*-g++*|*-clang* {
    QMAKE_CXX=$$(CCACHE_BIN) $$QMAKE_CXX
}
