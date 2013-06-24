TEMPLATE = subdirs

SUBDIRS = qmlmoztestrunner

OTHER_FILES += auto/* auto/scripts/*

auto.files = auto/*
auto.path = /opt/tests/qtmozembed/auto

components.files = components/*
components.path = /opt/tests/qtmozembed/components

contains(QT_MAJOR_VERSION, 4) {
definition.files = test-definition/tests.xml
} else {
definition.files = test-definition-qt5/tests.xml
}
definition.path = /opt/tests/qtmozembed/test-definition

INSTALLS += auto definition components
