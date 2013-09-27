/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtGlobal>
#include "qmozverticalscrolldecorator.h"

QMozVerticalScrollDecorator::QMozVerticalScrollDecorator(QObject *parent)
    : QObject(parent)
    , mY(0.0)
    , mHeight(0.0)
{
}

QMozVerticalScrollDecorator::~QMozVerticalScrollDecorator()
{
}

qreal QMozVerticalScrollDecorator::y() const
{
    return mY;
}

void QMozVerticalScrollDecorator::setY(qreal y)
{
    if (qIsNull(y))
        return;

    if (y != mY) {
        mY = y;
        Q_EMIT yChanged();
    }
}

qreal QMozVerticalScrollDecorator::height() const
{
    return mHeight;
}

void QMozVerticalScrollDecorator::setHeight(qreal height)
{
    if (qIsNull(height))
        return;

    // Fuzzy compare?, maybe worth checking against small threshold.
    if (height != mHeight) {
        mHeight = height;
        Q_EMIT heightChanged();
    }
}
