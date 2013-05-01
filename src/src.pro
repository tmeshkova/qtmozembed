CONFIG += qt thread debug ordered create_pc create_prl no_install_prl

TARGET = qtembedwidget
TEMPLATE = lib

SOURCES += qmozcontext.cpp \
           EmbedQtKeyUtils.cpp \
           qgraphicsmozview.cpp \
           qgraphicsmozview_p.cpp \
           geckoworker.cpp

HEADERS += qmozcontext.h \
           EmbedQtKeyUtils.h \
           qgraphicsmozview.h \
           qgraphicsmozview_p.h \
           geckoworker.h

!contains(QT_MAJOR_VERSION, 4) {
SOURCES += quickmozview.cpp
HEADERS += quickmozview.h
} else {
SOURCES += qdeclarativemozview.cpp
HEADERS += qdeclarativemozview.h
}

CONFIG(opengl) {
     message(Building with OpenGL support.)
} else {
     message(OpenGL support is not available.)
}

isEmpty(OBJ_DEB_DIR) {
  OBJ_DEB_DIR=$$OBJ_BUILD_PATH
}

OBJECTS_DIR += ../$$OBJ_DEB_DIR
DESTDIR = ../$$OBJ_DEB_DIR
MOC_DIR += ../$$OBJ_DEB_DIR/tmp/moc/release_static
RCC_DIR += ../$$OBJ_DEB_DIR/tmp/rcc/release_static

include(qmozembed.pri)

PREFIX = /usr

contains(QT_MAJOR_VERSION, 4) {
  QT += opengl
  PKGCONFIG += QJson
} else {
  QT += quick opengl
}

target.path = $$PREFIX/lib

QMAKE_PKGCONFIG_NAME = qtembedwidget
QMAKE_PKGCONFIG_DESCRIPTION = Model that emits process info
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$target.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

# install forwarding headers
# match only the camel case forwarding headers here
FORWARDING_HEADERS = $$system( find q*.h )

forwarding_headers.path = $$PREFIX/include
forwarding_headers.files = $$FORWARDING_HEADERS
INSTALLS += forwarding_headers target
