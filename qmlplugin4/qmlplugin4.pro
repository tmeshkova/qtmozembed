MODULENAME = QtMozilla
TARGET = qtmozembedpluginqt4

TEMPLATE = lib
CONFIG += qt plugin

QTMOZEMBED_SOURCE_PATH = $$PWD/../src

QT += declarative script
CONFIG += mobility link_pkgconfig
MOBILITY += qtmozembed

LIBS+=-lX11
isEmpty(OBJ_BUILD_PATH) {
  LIBS+= -L../ -lqtembedwidget
} else {
  LIBS+= -L../$$OBJ_BUILD_PATH -lqtembedwidget
}

isEmpty(DEFAULT_COMPONENT_PATH) {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"/usr/lib/mozembedlite/\\\"\"
} else {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$DEFAULT_COMPONENT_PATH\\\"\"
}

INCLUDEPATH += $$QTMOZEMBED_SOURCE_PATH

HEADERS += qmlmozcontext.h

SOURCES += \
    main.cpp \
    qmlmozcontext.cpp

import.files = qmldir
import.path = $$TARGETPATH
INSTALLS += import

TARGET = $$qtLibraryTarget($$TARGET)
TARGETPATH = /opt/tests/qtmozembed/imports/$$MODULENAME

target.path = $$TARGETPATH
INSTALLS += target
