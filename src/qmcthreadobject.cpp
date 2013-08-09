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
    mGLContext = mSGThreadObj->context();
    mGLSurface = mSGThreadObj->context()->surface();
    start();
}

void
QMCThread::run()
{
    mQtPump = new MessagePumpQt(QMozContext::GetInstance()->GetApp());
    mMCRenderer = new QMCThreadObject(mView, mSGThreadObj);
    exec();
}

QMCThreadObject::QMCThreadObject(QuickMozView* aView, QSGThreadObject* sgThreadObj)
  : mView(aView)
  , mGLContext(0)
  , mGLSurface(0)
  , mSGThreadObj(sgThreadObj)
{
    mLoop = QMozContext::GetInstance()->GetApp()->CreateEmbedLiteMessagePump(NULL);
    mMCPThread = pthread_self();
    connect(mSGThreadObj, SIGNAL(renderRequest(QMatrix,QSize)), this, SLOT(onRenderRequested(QMatrix,QSize)), Qt::BlockingQueuedConnection);
    mGLContext = new QOpenGLContext;
    mGLContext->setShareContext(mSGThreadObj->context());
    bool retval = mGLContext->create();
    mGLSurface = mSGThreadObj->context()->surface();
    makeContextCurrent();
}

void QMCThreadObject::onThreadSwitch(void* self)
{
    QMCThreadObject* app = static_cast<QMCThreadObject*>(self);
    app->wakeup();
}

void QMCThreadObject::wakeup()
{
    mView->RenderToCurrentContext2(mLastMatr, mLastSize);
    waitCondition.wakeAll();
}

void
QMCThreadObject::ProcessInGeckoThread(QMatrix matr, QSize size)
{
    static int ENTER_VAL = 0;
    ENTER_VAL++;
    int localVal = ENTER_VAL;
    if (!mutex.tryLock()) {
        return;
    }
    mLastMatr = matr;
    mLastSize = size;
    mLoop->PostTask(&QMCThreadObject::onThreadSwitch, this, 16);
    waitCondition.wait(&mutex);
    mutex.unlock();
}

QMCThreadObject::~QMCThreadObject()
{
}


void
QMCThreadObject::makeContextCurrent()
{
    mGLContext->makeCurrent(mGLSurface);
}

void
QMCThreadObject::onRenderRequested(QMatrix matr, QSize size)
{
    mView->RenderToCurrentContext2(matr, size);
}
