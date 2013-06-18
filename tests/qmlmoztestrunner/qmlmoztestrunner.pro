TEMPLATE = app
TARGET = qmlmoztestrunner
CONFIG += warn_on link_pkgconfig
SOURCES += main.cpp qtestrunner.cpp
HEADERS += qtestrunner.h

RELATIVE_PATH=../..
VDEPTH_PATH=tests/qmlmoztestrunner
include($$RELATIVE_PATH/relative-objdir.pri)

INCLUDEPATH+=$$RELATIVE_PATH/src
LIBS+= -L$$RELATIVE_PATH/$$OBJ_BUILD_PATH/src -lqtembedwidget

isEmpty(DEFAULT_COMPONENT_PATH) {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"/usr/lib/mozembedlite/\\\"\"
} else {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$DEFAULT_COMPONENT_PATH\\\"\"
}

contains(QT_MAJOR_VERSION, 4) {
  LIBS += -lQtQuickTest
  PKGCONFIG += QJson
} else {
  PKGCONFIG += Qt5QuickTest
  QT += declarative qml quick
}
contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2) {
    QT += opengl
}

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
