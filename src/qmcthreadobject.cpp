/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QSurface>
#include <QPainter>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QOffscreenSurface>
#include "qmcthreadobject.h"
#include "qsgthreadobject.h"
#include "qmozcontext.h"
#include "quickmozview.h"
#include "qmozviewsgnode.h"
#include "qgraphicsmozview_p.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedLiteMessagePump.h"
#include "mozilla/embedlite/EmbedLiteRenderTarget.h"

QMCThreadObject::QMCThreadObject(QuickMozView* aView, QSGThreadObject* sgThreadObj, QSize aGLSize)
  : mView(aView)
  , mGLContext(NULL)
  , mGLSurface(NULL)
  , mOffGLSurface(NULL)
  , mSGThreadObj(sgThreadObj)
  , mOwnGLContext(false)
  , m_renderTarget(NULL)
  , mSGnode(NULL)
{
    m_size = aGLSize;
    if (sgThreadObj->thread() != thread()) {
        mOwnGLContext = true;
        mGLContext = new QOpenGLContext;
        mGLContext->setFormat(mSGThreadObj->context()->format());
        mGLContext->setShareContext(mSGThreadObj->context());
        mGLSurface = mSGThreadObj->context()->surface();
        if (mGLContext->create()) {
            mSGThreadObj->context()->doneCurrent();
            if (!mGLContext->makeCurrent(mGLSurface)) {
                mOffGLSurface = new QOffscreenSurface;
                mOffGLSurface->setFormat(mSGThreadObj->context()->format());
                mOffGLSurface->create();
                mGLContext->makeCurrent(mOffGLSurface);
            }
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
    if (aView->thread() == thread()) {
        if (mOffGLSurface) {
            connect(this, SIGNAL(workInGeckoCompositorThread()), this, SLOT(ProcessRenderInGeckoCompositorThread()));
        } else {
            connect(this, SIGNAL(workInGeckoCompositorThread()), this, SLOT(ProcessRenderInGeckoCompositorThread()), Qt::BlockingQueuedConnection);
        }
    }
}

void QMCThreadObject::setSGNode(QMozViewSGNode* node)
{
    mSGnode = node;
}

QMCThreadObject::~QMCThreadObject()
{
    if (mOwnGLContext)
        delete mGLContext;
    delete mOffGLSurface;
    delete m_renderTarget;
}

void QMCThreadObject::RenderToCurrentContext(QMatrix affine)
{
    mProcessingMatrix = affine;
    Q_EMIT workInGeckoCompositorThread();
}

void QMCThreadObject::ProcessRenderInGeckoCompositorThread()
{
    if (!mOffGLSurface) {
        mGLContext->makeCurrent(mGLSurface);
        mView->RenderToCurrentContext(mProcessingMatrix);
    } else {
        mGLContext->makeCurrent(mOffGLSurface);
        m_size = mGLSurface ? mGLSurface->size() : QSize();
        if (!m_renderTarget) {
            // Initialize the buffers and renderer
            m_renderTarget = mView->CreateEmbedLiteRenderTarget(m_size);
        }

        mProcessingMatrix = QMatrix();
        mView->RenderToCurrentContext(mProcessingMatrix, m_renderTarget);
        glFlush();

        if (mSGnode) {
            mSGnode->newTexture(m_renderTarget->texture(), m_size);
        }
    }
}

void QMCThreadObject::prepareTexture()
{
    if (mSGnode) {
        mSGnode->prepareNode();
    }
}
