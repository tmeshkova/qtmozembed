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
  , mLoop(NULL)
  , mRenderTask(NULL)
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
        connect(this, SIGNAL(workInGeckoCompositorThread()), this, SLOT(ProcessRenderInGeckoCompositorThread()), Qt::QueuedConnection);
    } else {
        mLoop = QMozContext::GetInstance()->GetApp()->CreateEmbedLiteMessagePump(NULL);
    }
}

void QMCThreadObject::setView(QuickMozView* aView)
{
    mView = aView;
}

void QMCThreadObject::setSGNode(QMozViewSGNode* node)
{
    mSGnode = node;
}

QMCThreadObject::~QMCThreadObject()
{
    delete m_renderTarget;
    if (mOwnGLContext)
        delete mGLContext;
    mGLContext = nullptr;
    delete mOffGLSurface;
    if (mRenderTask && mLoop) {
        destroyLock.lock();
        destroyLockCondition.wait(&destroyLock);
        destroyLock.unlock();
        mRenderTask = nullptr;
    }
    delete mLoop;
}

void QMCThreadObject::RenderToCurrentContext(QMatrix affine)
{
    mProcessingMatrix = affine;
    mutex.lock();
    if (!mLoop) {
        Q_EMIT workInGeckoCompositorThread();
    } else {
        mRenderTask = mLoop->PostTask(&QMCThreadObject::doWorkInGeckoCompositorThread, this);
    }
    waitCondition.wait(&mutex);
    mutex.unlock();
    destroyLockCondition.wakeAll();

}

void QMCThreadObject::doWorkInGeckoCompositorThread(void* self)
{
    QMCThreadObject* me = static_cast<QMCThreadObject*>(self);
    me->mRenderTask = nullptr;
    me->ProcessRenderInGeckoCompositorThread();
}

void QMCThreadObject::ProcessRenderInGeckoCompositorThread()
{
    if (!mOffGLSurface && mView && mGLContext) {
        mGLContext->makeCurrent(mGLSurface);
        mView->RenderToCurrentContext(mProcessingMatrix);
    } else if (mView && mGLContext) {
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
            mSGnode->prepareNode();
        }
    }
    waitCondition.wakeAll();
}

void QMCThreadObject::prepareTexture()
{
    if (mSGnode) {
        mSGnode->prepareNode();
    }
}
