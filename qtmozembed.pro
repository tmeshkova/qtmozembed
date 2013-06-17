TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = src
SUBDIRS += qmlplugin4
qmlplugin4.depends = src
contains(QT_MAJOR_VERSION, 5) {
 SUBDIRS += qmlplugin5
 qmlplugin5.depends = src
}
isEmpty(NO_TESTS) {
  SUBDIRS += tests
}
