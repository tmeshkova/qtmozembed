TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = src
contains(QT_MAJOR_VERSION, 4) {
SUBDIRS += qmlplugin4
qmlplugin4.depends = src
}
contains(QT_MAJOR_VERSION, 5) {
!isEmpty(BUILD_QT5QUICK1) {
 SUBDIRS += qmlplugin4
 qmlplugin4.depends = src
}
 SUBDIRS += qmlplugin5
 qmlplugin5.depends = src
}
isEmpty(NO_TESTS) {
  SUBDIRS += tests
}
