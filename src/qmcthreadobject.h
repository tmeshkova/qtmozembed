/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QMCThreadObject_H
#define QMCThreadObject_H

#include <QObject>
#include <QSize>
#include <QMatrix>
#include <QtGui/QOpenGLContext>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

namespace mozilla {
namespace embedlite {
class EmbedLiteMessagePump;
class EmbedLiteRenderTarget;
}}

class QSGThreadObject;
class QuickMozView;
class QOffscreenSurface;
class QOpenGLFramebufferObject;
class QMCThreadObject : public QObject
{
    Q_OBJECT
public:
    QMCThreadObject(QuickMozView* aView, QSGThreadObject* sgThreadObj, QSize aGLSize);
    ~QMCThreadObject();
    void RenderToCurrentContext(QMatrix affine);
    void setView(QuickMozView* aView);
    void prepareTexture();
    void checkIfHasTexture();

Q_SIGNALS:
    void workInGeckoCompositorThread();
    void textureReady(int id, const QSize &size);
    void compositorHasTexture();

public Q_SLOTS:
    void ProcessRenderInGeckoCompositorThread();
    void renderNext();

private:
    static void doWorkInGeckoCompositorThread(void* self);

    QuickMozView* mView;
    QOpenGLContext* mGLContext;
    QSurface* mGLSurface;
    QOffscreenSurface* mOffGLSurface;
    QSGThreadObject* mSGThreadObj;
    bool mOwnGLContext;
    QMatrix mProcessingMatrix;
    QSize m_size;
    mozilla::embedlite::EmbedLiteRenderTarget* m_renderTarget;
    mozilla::embedlite::EmbedLiteMessagePump* mLoop;
    QMutex mutex;
    QWaitCondition waitCondition;
    QMutex destroyLock;
    QWaitCondition destroyLockCondition;
    void* mRenderTask;
    bool mIsDestroying;
    bool mIsRendering;
};

#endif // QMCThreadObject_H
