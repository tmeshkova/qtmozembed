TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = src
SUBDIRS += qmlplugin5
qmlplugin5.depends = src

isEmpty(NO_TESTS) {
  SUBDIRS += tests
}
