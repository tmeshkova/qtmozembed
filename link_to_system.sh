#!/bin/sh

TARGET_DIR=$1
if [ "$TARGET_DIR" = "" ]; then
  echo "TARGET_DIR ex: /usr"
  TARGET_DIR=/usr
fi

BARCH=`uname -m`
OBJDIR="obj-$BARCH-dir"

PREFIX=$TARGET_DIR/lib
mkdir -p $PREFIX

FILES_LIST="
$OBJDIR/libqtembedwidget.so
$OBJDIR/libqtembedwidget.so.1
$OBJDIR/libqtembedwidget.so.1.0
$OBJDIR/libqtembedwidget.so.1.0.0
"

for str in $FILES_LIST; do
    fname="${str##*/}"
    rm -f $PREFIX/$fname;
    ln -s $(pwd)/$str $PREFIX/$fname;
done

rm -f $PREFIX/pkgconfig/qtembedwidget.pc;
ln -s $(pwd)/$OBJDIR/pkgconfig/qtembedwidget.pc $PREFIX/pkgconfig/qtembedwidget.pc

PREFIX=$TARGET_DIR/include
mkdir -p $PREFIX

FILES_LIST="
EmbedQtKeyUtils.h
qdeclarativemozview.h
qgraphicsmozview.h
qmozcontext.h
"

for str in $FILES_LIST; do
    fname="${str##*/}"
    rm -f $PREFIX/$fname;
    ln -s $(pwd)/$str $PREFIX/$fname;
done
