CONFIG += qt thread debug ordered create_pc create_prl no_install_prl

contains(QT_MAJOR_VERSION, 5) {
  TARGET = qt5embedwidget
} else {
  TARGET = qtembedwidget
}
TEMPLATE = lib

isEmpty(VERSION) {
    GIT_TAG = $$system(git describe --tags --abbrev=0)
    GIT_VERSION = $$system(echo $$GIT_TAG | sed 's/nemo[/]//')
    isEmpty(GIT_VERSION) {
        # if you're trying to build this from a tarball, I'm sorry. but being
        # able to specify the version in just one place (git tags) is a lot less
        # error-prone and easy.
        warning("Can't find a valid git tag version, got: $$GIT_TAG")
        GIT_VERSION = 0.0.0
    }
    !isEmpty(GIT_VERSION): VERSION = $$GIT_VERSION
}

SOURCES += qmozcontext.cpp \
           qmessagepump.cpp \
           EmbedQtKeyUtils.cpp \
           qgraphicsmozview_p.cpp \
           geckoworker.cpp

HEADERS += qmozcontext.h \
           qmessagepump.h \
           EmbedQtKeyUtils.h \
           geckoworker.h \
           qmozview_defined_wrapper.h

!contains(QT_MAJOR_VERSION, 4) {
  SOURCES += quickmozview.cpp qmozviewsgnode.cpp qsgthreadobject.cpp qmcthreadobject.cpp
  HEADERS += quickmozview.h qmozviewsgnode.h qsgthreadobject.h qmcthreadobject.h
}
contains(QT_MAJOR_VERSION, 4) {
  SOURCES += qdeclarativemozview.cpp qgraphicsmozview.cpp
  HEADERS += qdeclarativemozview.h qgraphicsmozview.h
} else {
  !isEmpty(BUILD_QT5QUICK1) {
    SOURCES += qdeclarativemozview.cpp qgraphicsmozview.cpp
    HEADERS += qdeclarativemozview.h qgraphicsmozview.h
  }
}


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
  QT += quick qml quick-private
  !isEmpty(BUILD_QT5QUICK1) {
    QT += declarative widgets opengl
  }
}

#DEFINES += Q_DEBUG_LOG

target.path = $$PREFIX/lib

contains(QT_MAJOR_VERSION, 5) {
  QMAKE_PKGCONFIG_NAME = qt5embedwidget
} else {
  QMAKE_PKGCONFIG_NAME = qtembedwidget
}
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
