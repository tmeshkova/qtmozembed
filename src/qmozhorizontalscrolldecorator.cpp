/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtGlobal>
#include "qmozhorizontalscrolldecorator.h"

QMozHorizontalScrollDecorator::QMozHorizontalScrollDecorator(QObject *parent)
    : QObject(parent)
    , mX(0.0)
    , mWidth(0.0)
{
}

QMozHorizontalScrollDecorator::~QMozHorizontalScrollDecorator()
{
}

qreal QMozHorizontalScrollDecorator::x() const
{
    return mX;
}

void QMozHorizontalScrollDecorator::setX(qreal x)
{
    if (qIsNull(x))
        return;

    if (x != mX) {
        mX = x;
        Q_EMIT xChanged();
    }
}

qreal QMozHorizontalScrollDecorator::width() const
{
    return mWidth;
}

void QMozHorizontalScrollDecorator::setWidth(qreal width)
{
    if (qIsNull(width))
        return;

    // Fuzzy compare?, maybe worth checking against small threshold.
    if (width != mWidth) {
        mWidth = width;
        Q_EMIT widthChanged();
    }
}

