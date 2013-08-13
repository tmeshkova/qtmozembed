/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QSurface>
#include <QOpenGLFramebufferObject>
#include <QOffscreenSurface>
#include "qmcthreadobject.h"
#include "qsgthreadobject.h"
#include "qmozcontext.h"
#include "quickmozview.h"
#include "qmozviewtexsgnode.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedLiteMessagePump.h"

QMCThreadObject::QMCThreadObject(QuickMozView* aView, QSGThreadObject* sgThreadObj, QSize aGLSize)
  : mView(aView)
  , mGLContext(NULL)
  , mGLSurface(NULL)
  , mOffGLSurface(NULL)
  , mSGThreadObj(sgThreadObj)
  , mLoop(NULL)
  , mOwnGLContext(false)
  , m_renderFbo(NULL)
  , m_displayFbo(NULL)
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
            if (getenv("FORCE_OFFSCREEN_FBO_RENDER") != 0 || !mGLContext->makeCurrent(mGLSurface)) {
                qDebug() << "failed to make Gecko QOpenGLContext current from Qt render thread QSurface, let's make separate offscreen surface here";
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
    } else {
        mLoop = QMozContext::GetInstance()->GetApp()->CreateEmbedLiteMessagePump(NULL);
    }
}

void QMCThreadObject::setTexSGNode(QMozViewTexSGNode* node)
{
    mSGnode = node;
}

QMCThreadObject::~QMCThreadObject()
{
    delete mLoop;
    if (mOwnGLContext)
        delete mGLContext;
    delete mOffGLSurface;
    delete m_renderFbo;
    delete m_displayFbo;
}

void QMCThreadObject::PostInvalidateToRenderThread()
{
    if (!mLoop) {
        Q_EMIT workInGeckoCompositorThread();
    } else {
        mLoop->PostTask(&QMCThreadObject::doWorkInGeckoCompositorThread, this);
    }
}

void QMCThreadObject::RenderToCurrentContext(QMatrix affine)
{
    mProcessingMatrix = affine;
    if (!mLoop) {
        Q_EMIT workInGeckoCompositorThread();
    } else {
        mutex.lock();
        mLoop->PostTask(&QMCThreadObject::doWorkInGeckoCompositorThread, this);
        waitCondition.wait(&mutex);
        mutex.unlock();
    }
}

void QMCThreadObject::doWorkInGeckoCompositorThread(void* self)
{
    QMCThreadObject* me = static_cast<QMCThreadObject*>(self);
    me->ProcessRenderInGeckoCompositorThread();
}

void QMCThreadObject::PostNotificationUpdate(void* self)
{
    QMCThreadObject* me = static_cast<QMCThreadObject*>(self);
    me->PostNotificationUpdate();
}

void QMCThreadObject::PostNotificationUpdate()
{
    mView->update();
}

void QMCThreadObject::ProcessRenderInGeckoCompositorThread()
{
    if (!mOffGLSurface) {
        mGLContext->makeCurrent(mGLSurface);
        mView->RenderToCurrentContext(mProcessingMatrix);
    } else {
        mGLContext->makeCurrent(mOffGLSurface);
        m_size = mGLSurface ? mGLSurface->size() : QSize();
        if (!m_renderFbo) {
            // Initialize the buffers and renderer
            QOpenGLFramebufferObjectFormat format;
            format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
            m_renderFbo = new QOpenGLFramebufferObject(m_size, format);
            m_displayFbo = new QOpenGLFramebufferObject(m_size, format);
        }
        m_renderFbo->bind();
        glViewport(0, 0, m_size.width(), m_size.height());
        mView->RenderToCurrentContext(mProcessingMatrix);
        glFlush();
        m_renderFbo->bindDefault();
        qSwap(m_renderFbo, m_displayFbo);
//        QImage img = m_displayFbo->toImage();
//        static int counts = 0;
//        img.save(QString("/tmp/images/img%1.png").arg(counts++));
        if (mSGnode) {
            mSGnode->newTexture(m_displayFbo->texture(), m_size);
        }
        QMozContext::GetInstance()->GetApp()->PostTask(&QMCThreadObject::PostNotificationUpdate, this);
    }

    if (mLoop) {
        waitCondition.wakeOne();
    }
}

void QMCThreadObject::prepareTexture()
{
    if (mSGnode) {
        mSGnode->prepareNode();
    }
}
