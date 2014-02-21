/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtGlobal>
#include "qmozscrolldecorator.h"

QMozScrollDecorator::QMozScrollDecorator(QObject *parent)
    : QObject(parent)
    , mPos(0)
    , mSize(0)
    , mMoving(false)
{
}

QMozScrollDecorator::~QMozScrollDecorator()
{
}

int QMozScrollDecorator::position() const
{
    return mPos;
}

void QMozScrollDecorator::setPosition(qreal pos)
{
    if (mPos != pos) {
        mPos = pos;
        Q_EMIT positionChanged();
    }
}

int QMozScrollDecorator::size() const
{
    return mSize;
}

void QMozScrollDecorator::setSize(qreal size)
{
    if (qIsNull(size))
        return;

    if (mSize != size) {
        mSize = size;
        Q_EMIT sizeChanged();
    }
}

bool QMozScrollDecorator::moving() const
{
    return mMoving;
}

void QMozScrollDecorator::setMoving(bool moving)
{
    if (mMoving != moving) {
        mMoving = moving;
        Q_EMIT movingChanged();
    }
}
