/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QThread>
#include <QSurface>
#include "qsgthreadobject.h"
#include "qmozcontext.h"

#if defined(QT_OPENGL_ES_2)
#include <EGL/egl.h>
#else
#include <GL/glx.h>
#endif

QSGThreadObject::QSGThreadObject()
  : mRenderTarget(0)
{
    wrapRenderThreadGLContext();
}

QSGThreadObject::~QSGThreadObject()
{
    delete mRenderTarget;
}

void
QSGThreadObject::wrapRenderThreadGLContext()
{
#if defined(QT_OPENGL_ES_2)
    void* context = (void*)eglGetCurrentContext();
    void* surface = (void*)eglGetCurrentSurface(EGL_DRAW);
#else
    void* context = (void*)glXGetCurrentContext();
    void* surface = (void*)glXGetCurrentDrawable();
#endif
    mRenderTarget = QMozContext::GetInstance()->createEmbedLiteRenderTarget(context, surface);
}
