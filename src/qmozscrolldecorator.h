/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QMOZSCROLLDECORATOR_H
#define QMOZSCROLLDECORATOR_H

#include <QObject>

class QMozScrollDecorator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(int size READ size NOTIFY sizeChanged FINAL)
    Q_PROPERTY(bool moving READ moving NOTIFY movingChanged FINAL)

public:
    QMozScrollDecorator(QObject *parent = 0);
    virtual ~QMozScrollDecorator();

    int position() const;
    void setPosition(qreal pos);

    int size() const;
    void setSize(qreal size);

    bool moving() const;
    void setMoving(bool moving);

Q_SIGNALS:
    void positionChanged();
    void sizeChanged();
    void movingChanged();

private:
    int mPos;
    int mSize;
    bool mMoving;
};

#endif
