/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QSGThreadObject_H
#define QSGThreadObject_H

#include <QObject>
#include <QSize>
#include <QtGui/QOpenGLContext>

class QSGThreadObject : public QObject
{
    Q_OBJECT
public:
    QSGThreadObject();
    ~QSGThreadObject() {}

public Q_SLOTS:
    void setupCurrentGLContext();
    void makeContextCurrent();
    QOpenGLContext* context() { return mGLContext; }

Q_SIGNALS:
    void updateGLContextInfo(bool hasContext, QSize viewPortSize);

private:
    QOpenGLContext* mGLContext;
    QSurface* mGLSurface;
};

#endif // QSGThreadObject_H
