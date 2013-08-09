/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmcthreadobject.h"
#include "qsgthreadobject.h"
#include "qmozcontext.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

QMCThreadObject::QMCThreadObject(QSGThreadObject* sgThreadObj)
  : mGLContext(0)
  , mGLSurface(0)
  , mSGThreadObj(sgThreadObj)
{
    mLoop = QMozContext::GetInstance()->GetApp()->CreateEmbedLiteMessagePump(NULL);
    mGLContext = new QOpenGLContext;
    mGLContext->setShareContext(mSGThreadObj->context());
    mGLSurface = mSGThreadObj->context()->surface();
    if (mGLContext->create()) {
        mGLContext->makeCurrent(mGLSurface);
    }
}
