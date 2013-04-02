TEMPLATE = lib

TARGET = $$qtLibraryTarget($$TARGET)
TARGETPATH = /opt/tests/qtmozembed/imports/$$MODULENAME

target.path = $$TARGETPATH
INSTALLS += target
