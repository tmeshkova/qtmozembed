QT += opengl
CONFIG += qt thread debug ordered create_pc create_prl no_install_prl

TARGET = qtembedwidget
TEMPLATE = lib

SOURCES += qmozcontext.cpp \
           EmbedQtKeyUtils.cpp \
           qdeclarativemozview.cpp \
           qgraphicsmozview.cpp

HEADERS += qmozcontext.h \
           EmbedQtKeyUtils.h \
           qdeclarativemozview.h \
           qgraphicsmozview.h

CONFIG(opengl) {
     message(Building with OpenGL support.)
} else {
     message(OpenGL support is not available.)
}

OBJECTS_DIR += release
DESTDIR = ./release
MOC_DIR += ./release/tmp/moc/release_static
RCC_DIR += ./release/tmp/rcc/release_static

include(qmozembed.pri)

PREFIX = /usr

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
