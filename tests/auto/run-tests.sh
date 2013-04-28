#!/bin/sh

# Create a temporary DBus session to isolate us from the normal environment.
export `dbus-launch`
QMLMOZTESTRUNNER=/usr/lib/qt4/bin/qmlmoztestrunner
if [ "$QTTESTPATH" != "" ]; then
  QMLMOZTESTRUNNER=$QTTESTPATH/qmlmoztestrunner/qmlmoztestrunner
fi
export QTTESTPATH=${QTTESTPATH:-"/opt/tests/qtmozembed"}
export QML_IMPORT_PATH=$QTTESTPATH/imports

#export NSPR_LOG_MODULES=all:5

$QMLMOZTESTRUNNER -opengl $@
exit_code=$?

kill $DBUS_SESSION_BUS_PID

exit $exit_code
