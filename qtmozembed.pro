TEMPLATE = subdirs
SUBDIRS = src
isEmpty(NO_TESTS) {
  SUBDIRS += tests
}
