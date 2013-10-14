/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtGlobal>
#include "qmozverticalscrolldecorator.h"

QMozVerticalScrollDecorator::QMozVerticalScrollDecorator(QObject *parent)
    : QObject(parent)
    , mY(0)
    , mHeight(0)
{
}

QMozVerticalScrollDecorator::~QMozVerticalScrollDecorator()
{
}

int QMozVerticalScrollDecorator::y() const
{
    return mY;
}

void QMozVerticalScrollDecorator::setY(qreal y)
{
    int tmpY = y;
    if (tmpY != mY) {
        mY = tmpY;
        Q_EMIT yChanged();
    }
}

int QMozVerticalScrollDecorator::height() const
{
    return mHeight;
}

void QMozVerticalScrollDecorator::setHeight(qreal height)
{
    if (qIsNull(height))
        return;

    int tmpHeight = height;
    if (tmpHeight != mHeight) {
        mHeight = tmpHeight;
        Q_EMIT heightChanged();
    }
}
