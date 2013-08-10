/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QSurface>
#include "qmcthreadobject.h"
#include "qsgthreadobject.h"
#include "qmozcontext.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedLiteMessagePump.h"

QMCThreadObject::QMCThreadObject(QSGThreadObject* sgThreadObj)
  : mGLContext(NULL)
  , mGLSurface(NULL)
  , mSGThreadObj(sgThreadObj)
  , mLoop(NULL)
  , mOwnGLContext(false)
{
    mLoop = QMozContext::GetInstance()->GetApp()->CreateEmbedLiteMessagePump(NULL);
    if (sgThreadObj->thread() != thread()) {
        mOwnGLContext = true;
        mGLContext = new QOpenGLContext;
        mGLContext->setShareContext(mSGThreadObj->context());
        mGLSurface = mSGThreadObj->context()->surface();
        if (mGLContext->create()) {
            mGLContext->makeCurrent(mGLSurface);
        }
    } else {
        mGLContext = mSGThreadObj->context();
        if (mGLContext) {
            mGLSurface = mSGThreadObj->context()->surface();
            if (mGLSurface) {
                mGLContext->makeCurrent(mGLSurface);
            }
        }
    }
    Q_EMIT updateGLContextInfo(mGLContext && mGLSurface, mGLSurface ? mGLSurface->size() : QSize());
}

QMCThreadObject::~QMCThreadObject()
{
    delete mLoop;
    if (mOwnGLContext) {
        delete mGLContext;
    }
}

void QMCThreadObject::RenderToCurrentContext(QMatrix affine)
{
    printf(">>>>>>Func:%s::%d Ups we supposed to render something\n", __PRETTY_FUNCTION__, __LINE__);
}
