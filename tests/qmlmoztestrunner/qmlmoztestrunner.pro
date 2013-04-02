TEMPLATE = app
TARGET = qmlmoztestrunner
CONFIG += warn_on
SOURCES += main.cpp

INCLUDEPATH+=../../src
LIBS += -L../../ -lqtembedwidget
LIBS += -lQtQuickTest
contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2) {
    QT += opengl
}

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
