MODULENAME = QtMozilla
TARGET = qtmozembedplugin

include (../imports.pri)

QTMOZEMBED_SOURCE_PATH = $$PWD/../../../src

QT += dbus declarative script
CONFIG += mobility link_pkgconfig
MOBILITY += qtmozembed

LIBS+=-lX11
isEmpty(OBJ_BUILD_PATH) {
LIBS+= -L../../../ -lqtembedwidget
} else {
LIBS+= -L../../../$$OBJ_BUILD_PATH -lqtembedwidget
}

INCLUDEPATH += $$QTMOZEMBED_SOURCE_PATH

HEADERS += \
        qmlmozcontext.h

SOURCES += \
        main.cpp \
        qmlmozcontext.cpp

import.files = qmldir
import.path = $$TARGETPATH
INSTALLS += import
