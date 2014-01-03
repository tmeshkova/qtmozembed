/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QSurface>
#include <QThread>
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
  , mIsDestroying(false)
  , mIsRendering(false)
  , mProcessingOpacity(1.0)
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
#if defined(GL_PROVIDER_GLX)
            if (!mGLContext->makeCurrent(mGLSurface))
#endif
            {
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
    if (aView->thread() != thread()) {
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
    mIsDestroying = true;
    if (mIsRendering) {
        destroyLock.lock();
        destroyLockCondition.wait(&destroyLock);
        destroyLock.unlock();
        mRenderTask = nullptr;
    }
    delete m_renderTarget;
    if (mOwnGLContext)
        delete mGLContext;
    mGLContext = nullptr;
    delete mOffGLSurface;
    delete mLoop;
}

void QMCThreadObject::RenderToCurrentContext(QMatrix affine, float aOpacity)
{
    if (mIsDestroying) {
        return;
    }

    mProcessingMatrix = affine;
    mProcessingOpacity = aOpacity;
    mutex.lock();
    mIsRendering = true;
    if (!mLoop) {
        mRenderTask = QMozContext::GetInstance()->GetApp()->PostTask(&QMCThreadObject::doWorkInGeckoCompositorThread, this, 1);
    } else {
        mRenderTask = mLoop->PostTask(&QMCThreadObject::doWorkInGeckoCompositorThread, this, 1);
    }
    if (mIsRendering) {
      waitCondition.wait(&mutex);
    }
    mIsRendering = false;

    mutex.unlock();
    destroyLockCondition.wakeOne();
}

void QMCThreadObject::doWorkInGeckoCompositorThread(void* self)
{
    QMCThreadObject* me = static_cast<QMCThreadObject*>(self);
    me->ProcessRenderInGeckoCompositorThread();
    me->mRenderTask = nullptr;
}

void QMCThreadObject::ProcessRenderInGeckoCompositorThread()
{
    if (!mOffGLSurface && mView && !mIsDestroying) {
        mGLContext->makeCurrent(mGLSurface);
        mView->RenderToCurrentContext(mProcessingMatrix, mProcessingOpacity);
    } else if (mView && !mIsDestroying) {
        mGLContext->makeCurrent(mOffGLSurface);
        m_size = mGLSurface ? mGLSurface->size() : QSize();
        if (!m_renderTarget) {
            // Initialize the buffers and renderer
            m_renderTarget = mView->CreateEmbedLiteRenderTarget(m_size);
        }

        mProcessingMatrix = QMatrix();
        mView->RenderToCurrentContext(mProcessingMatrix, mProcessingOpacity, m_renderTarget);
        glFlush();

        if (mSGnode) {
            mSGnode->newTexture(m_renderTarget->texture(), m_size);
            mSGnode->prepareNode();
        }
    }
    mRenderTask = nullptr;
    mIsRendering = false;
    waitCondition.wakeOne();
}

void QMCThreadObject::prepareTexture()
{
    if (mSGnode) {
        mSGnode->prepareNode();
    }
}
