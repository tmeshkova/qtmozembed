/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QThread>
#include <QSurface>
#include "qsgthreadobject.h"
#include "qmozcontext.h"

QSGThreadObject::QSGThreadObject()
  : mGLContext(0)
  , mGLSurface(0)
  , mRenderTarget(0)
{
}

void
QSGThreadObject::onWrapRenderThreadGLContext()
{
     printf(">>>>>>Func:%s::%d curT:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
    mRenderTarget = QMozContext::GetInstance()->createEmbedLiteRenderTarget();
    Q_EMIT onRenderThreadReady();
}

void
QSGThreadObject::setupCurrentGLContext()
{
     printf(">>>>>>Func:%s::%d curT:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
    mGLContext = QOpenGLContext::currentContext();
    mGLSurface = mGLContext->surface();
    Q_EMIT updateGLContextInfo(mGLContext && mGLSurface, mGLSurface ? mGLSurface->size() : QSize());
}
