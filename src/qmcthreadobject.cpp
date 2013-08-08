/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmcthreadobject.h"

#include <QThread>
#include <QTimer>
#include <QSurface>

#include "qmozcontext.h"
#include "qmozview_defined_wrapper.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "qmessagepump.h"
#include "quickmozview.h"
#include "qsgthreadobject.h"
#include <QtGui/QOffscreenSurface>

using namespace mozilla;
using namespace mozilla::embedlite;

QMCThread::QMCThread(QuickMozView* aView, QSGThreadObject* sgThreadObj)
  : mView(aView)
  , mSGThreadObj(sgThreadObj)
  , mQtPump(0)
{
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    mGLContext = mSGThreadObj->context();
    mGLSurface = mSGThreadObj->context()->surface();
    start();
}

void
QMCThread::run()
{
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    mQtPump = new MessagePumpQt(QMozContext::GetInstance()->GetApp());
    QMozContext::GetInstance()->GetApp()->StartRenderLoopWithCustomPump(mQtPump->EmbedLoop());
    mMCRenderer = new QMCThreadObject(mView, mSGThreadObj);
    exec();
}

QMCThreadObject::QMCThreadObject(QuickMozView* aView, QSGThreadObject* sgThreadObj)
  : mView(aView)
  , mGLContext(0)
  , mGLSurface(0)
  , mSGThreadObj(sgThreadObj)
{
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    mLoop = QMozContext::GetInstance()->GetApp()->CreateEmbedLiteMessagePump(NULL);
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    mMCPThread = pthread_self();
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    connect(mSGThreadObj, SIGNAL(renderRequest(QMatrix,QSize)), this, SLOT(onRenderRequested(QMatrix,QSize)), Qt::BlockingQueuedConnection);
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    //connect(this, SIGNAL(renderRequest(QMatrix,QSize)), this, SLOT(onRenderRequested(QMatrix,QSize)), Qt::BlockingQueuedConnection);
//    connect(this, SIGNAL(threadSwitch()), this, SLOT(onThreadSwitch()), Qt::BlockingQueuedConnection);
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    QTimer::singleShot(0, this, SLOT(onInitialized()));
    mGLContext = new QOpenGLContext;
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    mGLContext->setShareContext(mSGThreadObj->context());
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    bool retval = mGLContext->create();
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
//    mGLSurface = new QOffscreenSurface;
//    mGLSurface->create();
    mGLSurface = mSGThreadObj->context()->surface();
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    makeContextCurrent();
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
}

void QMCThreadObject::onThreadSwitch(void* self)
{
    printf(">>>>>>Func:%s::%d ENTER curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    QMCThreadObject* app = static_cast<QMCThreadObject*>(self);
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    app->wakeup();
    printf(">>>>>>Func:%s::%d EXIT curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
}

void QMCThreadObject::wakeup()
{
    printf(">>>>>>Func:%s::%d ENTER curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    mView->RenderToCurrentContext2(mLastMatr, mLastSize);
    waitCondition.wakeAll();
    printf(">>>>>>Func:%s::%d EXIT curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
}

void
QMCThreadObject::ProcessInGeckoThread(QMatrix matr, QSize size)
{
    static int ENTER_VAL = 0;
    ENTER_VAL++;
    int localVal = ENTER_VAL;
    printf(">>>>>>Func:%s::%d ENTER curThread:%p, curThrId:%p, unID:%i\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(),localVal);
    if (!mutex.tryLock()) {
        return;
    }
//    mutex.lock();
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, unID:%i\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(),localVal);
    mLastMatr = matr;
    mLastSize = size;
    mLoop->PostTask(&QMCThreadObject::onThreadSwitch, this, 16);
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, unID:%i\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(),localVal);
    waitCondition.wait(&mutex);
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, unID:%i\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(),localVal);
    mutex.unlock();
    printf(">>>>>>Func:%s::%d EXIT curThread:%p, curThrId:%p, unID:%i\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(),localVal);
//    if (QThread::currentThread() != thread()) {
//        printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, selfT:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), pthread_self());
//        Q_EMIT threadSwitch();
//    } else {
//    pthread_t curThread = pthread_self();
//    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, selfT:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), pthread_self());
//    void* prevThread = NULL;
////    mView->update();
//    int retval = pthread_join(mMCPThread, &prevThread);
//    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, selfT:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), pthread_self());
//    retval = pthread_join(curThread, &prevThread);
//    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, selfT:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), pthread_self());
//    }
}

void
QMCThreadObject::onInitialized()
{
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, mGLContext:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), mGLContext);
    setupCurrentGLContext();
}

QMCThreadObject::~QMCThreadObject()
{
}

void
QMCThreadObject::setupCurrentGLContext()
{
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, mGLContext:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), mGLContext);
//    Q_EMIT updateGLContextInfo(mGLContext && mGLSurface, mGLSurface->size());
}

void
QMCThreadObject::makeContextCurrent()
{
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, mGLContext:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), mGLContext);
    mGLContext->makeCurrent(mGLSurface);
}

void
QMCThreadObject::onRenderRequested(QMatrix matr, QSize size)
{
    printf(">>>>>>Func:%s::%d START curThread:%p, curThrId:%p, mGLContext:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), mGLContext);
    mView->RenderToCurrentContext2(matr, size);
    printf(">>>>>>Func:%s::%d END curThread:%p, curThrId:%p, mGLContext:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), mGLContext);
}
