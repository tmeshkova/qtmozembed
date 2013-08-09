/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qsgthreadobject.h"

#include <QThread>
#include <QTimer>
#include <QSurface>

#include "qmozcontext.h"
#include "qmozview_defined_wrapper.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "qmessagepump.h"
#include "quickmozview.h"

using namespace mozilla;
using namespace mozilla::embedlite;

QSGThreadObject::QSGThreadObject(QuickMozView* aView)
  : mView(aView)
  , mGLContext(0)
  , mGLSurface(0)
{
    QTimer::singleShot(0, this, SLOT(onInitialized()));
}

void
QSGThreadObject::onInitialized()
{
    setupCurrentGLContext();
}

QSGThreadObject::~QSGThreadObject()
{
}

void
QSGThreadObject::setupCurrentGLContext()
{
    mGLContext = QOpenGLContext::currentContext();
    mGLSurface = mGLContext->surface();
    Q_EMIT updateGLContextInfo(mGLContext && mGLSurface, mGLSurface->size());
}

void
QSGThreadObject::makeContextCurrent()
{
    mGLContext->makeCurrent(mGLSurface);
}

void
QSGThreadObject::RenderToCurrentContext(QMatrix affine, QSize size)
{
    Q_EMIT renderRequest(affine, size);
}
