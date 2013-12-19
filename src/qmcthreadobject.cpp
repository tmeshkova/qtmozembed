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
{
    m_size = aGLSize;
    if (aView->thread() != thread()) {
        // printf(">>>>>>Func:%s::%d Thr:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
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

void QMCThreadObject::RenderToCurrentContext(QMatrix affine)
{
    if (mIsDestroying) {
        return;
    }

    // printf(">>>>>>Func:%s::%d Thr:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
    mProcessingMatrix = affine;
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
        // printf(">>>>>>Func:%s::%d Thr:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
//        mGLContext->makeCurrent(mGLSurface);
        mView->RenderToCurrentContext(mProcessingMatrix);
        Q_EMIT compositorHasTexture();
    } else if (mView && !mIsDestroying) {
        // printf(">>>>>>Func:%s::%d Thr:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
        mGLContext->makeCurrent(mOffGLSurface);
        m_size = mGLSurface ? mGLSurface->size() : QSize();
        if (!m_renderTarget) {
            // printf(">>>>>>Func:%s::%d ><>><<<<<<<<<<<<<<<<<<<<<<<<,\n", __PRETTY_FUNCTION__, __LINE__);
            // Initialize the buffers and renderer
            m_renderTarget = mView->CreateEmbedLiteRenderTarget(m_size);
        }

        mProcessingMatrix = QMatrix();
        mView->RenderToCurrentContext(mProcessingMatrix, m_renderTarget);
        glFlush();

        if (mSGnode) {
            // printf(">>>>>>Func:%s::%d ><>><<<<<<<<<<<<<<<<<<<<<<<<, newTex:%i\n", __PRETTY_FUNCTION__, __LINE__, m_renderTarget->texture());
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
    // printf(">>>>>>Func:%s::%d Thr:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
    if (mSGnode) {
        mSGnode->prepareNode();
    }
}

void QMCThreadObject::renderNext()
{
    // printf(">>>>>>Func:%s::%d Thr:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
}

void QMCThreadObject::checkIfHasTexture()
{
    // printf(">>>>>>Func:%s::%d Thr:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
    int textureID = 0, width = 0, height = 0;
    if (mView->GetPendingTexture(mSGThreadObj->GetTargetContextWrapper(), &textureID, &width, &height)) {
      Q_EMIT textureReady(textureID, QSize(width, height));
    }
}
