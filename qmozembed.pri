QMAKE_CXXFLAGS += -include mozilla-config.h
unix:QMAKE_CXXFLAGS += -fno-short-wchar -std=c++0x -fPIC
DEFINES += XPCOM_GLUE=1 XPCOM_GLUE_USE_NSPR=1 MOZ_GLUE_IN_PROGRAM=1

isEmpty(OBJ_PATH) {
  message(OBJ_PATH not defined)
  CONFIG += link_pkgconfig
  SDK_HOME=$$system(pkg-config --variable=sdkdir libxul-embedding)
  GECKO_LIB_DIR = $$SDK_HOME/lib
  GECKO_INCLUDE_DIR = $$SDK_HOME/include
} else {
  message(OBJ_PATH defined $$OBJ_PATH)
  GECKO_LIB_DIR = $$OBJ_PATH/dist/lib
  GECKO_INCLUDE_DIR = $$OBJ_PATH/dist/include
}

INCLUDEPATH += $$GECKO_INCLUDE_DIR $$GECKO_INCLUDE_DIR/nspr
LIBS += -L$$GECKO_LIB_DIR -lxpcomglue -Wl,--whole-archive -lmozglue
LIBS += -Wl,--no-whole-archive -rdynamic -ldl

DEFINES += BUILD_GRE_HOME=\"\\\"$$GRE_HOME\\\"\"

# Copy default mozilla flags to avoid some gcc warnings
*-g++*: QMAKE_CXXFLAGS += -Wno-attributes
*-g++*: QMAKE_CXXFLAGS += -Wno-ignored-qualifiers
*-g++*: QMAKE_CXXFLAGS += -pedantic
*-g++*: QMAKE_CXXFLAGS += -Wall
*-g++*: QMAKE_CXXFLAGS += -Wno-unused-parameter
*-g++*: QMAKE_CXXFLAGS += -Wpointer-arith
*-g++*: QMAKE_CXXFLAGS += -Woverloaded-virtual
*-g++*: QMAKE_CXXFLAGS += -Werror=return-type
*-g++*: QMAKE_CXXFLAGS += -Wtype-limits
*-g++*: QMAKE_CXXFLAGS += -Wempty-body
*-g++*: QMAKE_CXXFLAGS += -Wno-ctor-dtor-privacy
*-g++*: QMAKE_CXXFLAGS += -Wno-format
*-g++*: QMAKE_CXXFLAGS += -Wno-overlength-strings
*-g++*: QMAKE_CXXFLAGS += -Wno-invalid-offsetof
*-g++*: QMAKE_CXXFLAGS += -Wno-variadic-macros
*-g++*: QMAKE_CXXFLAGS += -Wno-long-long
*-g++*: QMAKE_CXXFLAGS += -Wno-psabi
