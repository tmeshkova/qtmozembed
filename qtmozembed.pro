TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = src
isEmpty(NO_TESTS) {
  SUBDIRS += tests
}
