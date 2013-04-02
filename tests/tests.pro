TEMPLATE = subdirs

SUBDIRS = imports qmlmoztestrunner

OTHER_FILES += auto/* auto/scripts/*

auto.files = auto/*
auto.path = /opt/tests/qtmozembed/auto

definition.files = test-definition/tests.xml
definition.path = /opt/tests/qtmozembed/test-definition

INSTALLS += auto definition
