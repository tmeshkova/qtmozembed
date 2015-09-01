MODULENAME = Qt5Mozilla
TARGET  = qmlmozembedpluginqt5

TEMPLATE = lib
CONFIG += qt plugin c++11
QT += qml quick

SOURCES += main.cpp qmlmozcontext.cpp
HEADERS += qmlmozcontext.h

RELATIVE_PATH=..
VDEPTH_PATH=qmlplugin5
include($$RELATIVE_PATH/relative-objdir.pri)

LIBS+= -L$$RELATIVE_PATH/$$OBJ_BUILD_PATH/src -lqt5embedwidget
QTMOZEMBED_SOURCE_PATH = $$PWD/$$RELATIVE_PATH/src
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

!isEmpty(OBJ_BUILD_PATH) {
  QMAKE_POST_LINK = rm -f $$OBJECTS_DIR/qmldir
  QMAKE_POST_LINK += && rm -f $$OBJECTS_DIR/Qt5Mozilla
  QMAKE_POST_LINK += && ln -s $$RELATIVE_PATH/$$RELATIVE_PATH/qmlplugin5/qmldir $$OBJECTS_DIR/qmldir
  QMAKE_POST_LINK += && ln -s . $$OBJECTS_DIR/Qt5Mozilla
}
