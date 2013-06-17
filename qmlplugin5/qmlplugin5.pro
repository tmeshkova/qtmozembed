MODULENAME = Qt5Mozilla
TARGET  = qmlmozembedpluginqt5

TEMPLATE = lib
CONFIG += qt plugin
QT += qml declarative widgets quick

SOURCES += main.cpp qmlmozcontext.cpp
HEADERS += qmlmozcontext.h

isEmpty(OBJ_BUILD_PATH) {
  LIBS+= -L../ -lqtembedwidget
} else {
  LIBS+= -L../$$OBJ_BUILD_PATH -lqtembedwidget
}
QTMOZEMBED_SOURCE_PATH = $$PWD/../src
INCLUDEPATH += $$QTMOZEMBED_SOURCE_PATH

isEmpty(DEFAULT_COMPONENT_PATH) {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"/usr/lib/mozembedlite/\\\"\"
} else {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$DEFAULT_COMPONENT_PATH\\\"\"
}

TARGET = $$qtLibraryTarget($$TARGET)
TARGETPATH = /usr/lib/qt5/qml/$$MODULENAME

target.path = $$TARGETPATH
INSTALLS += target

import.files = qmldir
import.path = $$TARGETPATH
INSTALLS += import
