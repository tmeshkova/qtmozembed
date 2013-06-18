MODULENAME = QtMozilla
TARGET = qtmozembedpluginqt4

TEMPLATE = lib
CONFIG += qt plugin

QT += declarative script
CONFIG += mobility link_pkgconfig
MOBILITY += qtmozembed

RELATIVE_PATH=..
VDEPTH_PATH=qmlplugin4
include($$RELATIVE_PATH/relative-objdir.pri)
QTMOZEMBED_SOURCE_PATH = $$PWD/$$RELATIVE_PATH/src

LIBS+=-lX11
LIBS+= -L$$RELATIVE_PATH/$$OBJ_BUILD_PATH/src -lqtembedwidget

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
