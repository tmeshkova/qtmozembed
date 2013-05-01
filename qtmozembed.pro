TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = src
contains(QT_MAJOR_VERSION, 4) {
  isEmpty(NO_TESTS) {
    SUBDIRS += tests
  }
}
