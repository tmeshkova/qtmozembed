/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtGlobal>
#include "qmozhorizontalscrolldecorator.h"

QMozHorizontalScrollDecorator::QMozHorizontalScrollDecorator(QObject *parent)
    : QObject(parent)
    , mX(0)
    , mWidth(0)
{
}

QMozHorizontalScrollDecorator::~QMozHorizontalScrollDecorator()
{
}

int QMozHorizontalScrollDecorator::x() const
{
    return mX;
}

void QMozHorizontalScrollDecorator::setX(qreal x)
{
    int tmpX = x;
    if (tmpX != mX) {
        mX = tmpX;
        Q_EMIT xChanged();
    }
}

int QMozHorizontalScrollDecorator::width() const
{
    return mWidth;
}

void QMozHorizontalScrollDecorator::setWidth(qreal width)
{
    if (qIsNull(width))
        return;

    int tmpWidth = width;
    if (tmpWidth != mWidth) {
        mWidth = tmpWidth;
        Q_EMIT widthChanged();
    }
}

