TEMPLATE = app
TARGET = qmlmoztestrunner
CONFIG += warn_on link_pkgconfig
SOURCES += main.cpp qtestrunner.cpp testviewcreator.cpp
HEADERS += qtestrunner.h testviewcreator.h

RELATIVE_PATH=../..
VDEPTH_PATH=tests/qmlmoztestrunner
include($$RELATIVE_PATH/relative-objdir.pri)

INCLUDEPATH+=$$RELATIVE_PATH/src
LIBS+= -L$$RELATIVE_PATH/$$OBJ_BUILD_PATH/src -lqt5embedwidget

isEmpty(DEFAULT_COMPONENT_PATH) {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"/usr/lib/mozembedlite/\\\"\"
} else {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$DEFAULT_COMPONENT_PATH\\\"\"
}

PKGCONFIG += Qt5QuickTest
QT += qml quick

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
