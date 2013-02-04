#!/bin/sh

TARGET_DIR=$1
if [ "$TARGET_DIR" = "" ]; then
  echo "TARGET_DIR ex: /usr/lib"
  TARGET_DIR=/usr/lib
fi

mkdir -p $TARGET_DIR

FILES_LIST="
release/libqtembedwidget.so
release/libqtembedwidget.so.1
release/libqtembedwidget.so.1.0
release/libqtembedwidget.so.1.0.0
"

for str in $FILES_LIST; do
    fname="${str##*/}"
    rm -f $TARGET_DIR/$fname;
    ln -s $(pwd)/$str $TARGET_DIR/$fname;
done
