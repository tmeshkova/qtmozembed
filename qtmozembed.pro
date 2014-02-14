TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = src
!isEmpty(BUILD_QT5QUICK1) {
 SUBDIRS += qmlplugin4
 qmlplugin4.depends = src
}
SUBDIRS += qmlplugin5
qmlplugin5.depends = src

isEmpty(NO_TESTS) {
  SUBDIRS += tests
}
