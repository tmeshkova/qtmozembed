/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QMCThreadObject_H
#define QMCThreadObject_H

#include <QObject>
#include <QSize>
#include <QThread>
#include <QMatrix>
#include <QtGui/QOpenGLContext>
#include "qmessagepump.h"
#include <pthread.h>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

class QuickMozView;
//class MessagePumpQt;
class QSGThreadObject;
class QOffscreenSurface;
class QMCThreadObject;
class QMCThread : public QThread
{
    Q_OBJECT
public:
    QMCThread(QuickMozView* aView, QSGThreadObject* sgThreadObj);
    ~QMCThread() {}

    virtual void run();

    QMCThreadObject* mMCRenderer;
private:
    QuickMozView* mView;
    QSGThreadObject* mSGThreadObj;
    QOpenGLContext* mGLContext;
//    QOffscreenSurface* mGLSurface;
    QSurface* mGLSurface;
    MessagePumpQt* mQtPump;
};

class QMCThreadObject : public QObject
{
    Q_OBJECT
public:
    QMCThreadObject(QuickMozView* aView, QSGThreadObject* sgThreadObj);
    ~QMCThreadObject();

    void makeContextCurrent();
    void ProcessInGeckoThread(QMatrix affine, QSize size);
    void wakeup();

public Q_SLOTS:
    void onRenderRequested(QMatrix, QSize);


Q_SIGNALS:
    void updateGLContextInfo(bool hasContext, QSize viewPortSize);
    void threadSwitch();

private:
    static void onThreadSwitch(void* self);

    QuickMozView* mView;
    QOpenGLContext* mGLContext;
//    QOffscreenSurface* mGLSurface;
    QSurface* mGLSurface;
    QSGThreadObject* mSGThreadObj;
    QOpenGLContext* mSharedGLContext;
    pthread_t mMCPThread;
    mozilla::embedlite::EmbedLiteMessagePump* mLoop;
    QMutex mutex;
    QWaitCondition waitCondition;
    QMatrix mLastMatr;
    QSize mLastSize;
};

#endif // QMCThreadObject_H
