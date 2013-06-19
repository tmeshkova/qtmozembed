CONFIG += qt thread debug ordered create_pc create_prl no_install_prl

TARGET = qtembedwidget
TEMPLATE = lib
VERSION = 1.0.1

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
}
SOURCES += qdeclarativemozview.cpp
HEADERS += qdeclarativemozview.h


CONFIG(opengl) {
     message(Building with OpenGL support.)
} else {
     message(OpenGL support is not available.)
}

include(qmozembed.pri)

RELATIVE_PATH=..
VDEPTH_PATH=src
include($$RELATIVE_PATH/relative-objdir.pri)

PREFIX = /usr

contains(QT_MAJOR_VERSION, 4) {
  QT += opengl
  PKGCONFIG += QJson
} else {
  QT += quick opengl declarative
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
