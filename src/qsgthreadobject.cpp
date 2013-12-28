/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QSurface>
#include "qsgthreadobject.h"

QSGThreadObject::QSGThreadObject()
  : mGLContext(0)
  , mGLSurface(0)
{
}

void
QSGThreadObject::setupCurrentGLContext()
{
    mGLContext = QOpenGLContext::currentContext();
    mGLSurface = mGLContext->surface();
    Q_EMIT updateGLContextInfo(mGLContext && mGLSurface, mGLSurface ? mGLSurface->size() : QSize());
}

void
QSGThreadObject::makeContextCurrent()
{
    mGLContext->makeCurrent(mGLSurface);
}
