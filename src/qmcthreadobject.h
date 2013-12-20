/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QMCThreadObject_H
#define QMCThreadObject_H

#include <QObject>
#include <QMatrix>

class QSGThreadObject;
class QuickMozView;

class QMCThreadObject : public QObject
{
    Q_OBJECT
public:
    QMCThreadObject(QuickMozView* aView);
    virtual ~QMCThreadObject() {}

private:
    QuickMozView* mView;
};

#endif // QMCThreadObject_H
