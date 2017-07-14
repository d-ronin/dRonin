INCLUDEPATH *= $$PWD

# work around Eigen's abuse of enums to allow compiling with recent compilers
QMAKE_CXXFLAGS += -Wno-int-in-bool-context
