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

isEmpty(DEFAULT_COMPONENT_PATH) {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"/usr/lib/mozembedlite/\\\"\"
} else {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$DEFAULT_COMPONENT_PATH\\\"\"
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
