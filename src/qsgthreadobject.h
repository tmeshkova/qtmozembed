/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QSGThreadObject_H
#define QSGThreadObject_H

#include <QObject>
#include <QSize>
#include <QtGui/QOpenGLContext>
#include "qmessagepump.h"

class QuickMozView;
//class MessagePumpQt;
class QSGThreadObject : public QObject, public MessagePumpQtListener
{
    Q_OBJECT
public:
    QSGThreadObject(QuickMozView* aView);
    ~QSGThreadObject();

    void scheduleUpdate();
    void makeContextCurrent();

public Q_SLOTS:
    void setupCurrentGLContext();
    void onInitialized();

Q_SIGNALS:
    void updateGLContextInfo(bool hasContext, QSize viewPortSize);

private:
    QuickMozView* mView;
    QOpenGLContext* mGLContext;
    QSurface* mGLSurface;
    MessagePumpQt* mQtPump;

};

#endif // QSGThreadObject_H
