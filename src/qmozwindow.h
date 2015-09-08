/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QMOZWINDOW_H
#define QMOZWINDOW_H

#include <QObject>
#include <QPointer>
#include <QRect>
#include <QScopedPointer>
#include <QSize>

class QMozWindowPrivate;

class QMozWindowListener
{
public:
    virtual bool invalidate() = 0;
    virtual bool preRender() = 0;
};

class QMozWindow: public QObject
{
    Q_OBJECT

public:
    explicit QMozWindow(QObject* parent = nullptr);
    ~QMozWindow();

    void setListener(QMozWindowListener*);
    void setSize(QSize);
    QSize size() const { return mSize; }
    void setContentOrientation(Qt::ScreenOrientation);
    void* getPlatformImage(int* width, int* height);
    void suspendRendering();
    void resumeRendering();
    void scheduleUpdate();

Q_SIGNALS:
    void requestGLContext();
    void initialized();
    void drawOverlay(QRect);
    void drawUnderlay();
    void compositorCreated();
    void compositingFinished();

private:
    friend class QOpenGLWebPage;
    friend class QuickMozView;
    friend class QMozWindowPrivate;

    QScopedPointer<QMozWindowPrivate> d;
    QMozWindowListener* mListener;

    QSize mSize;

    Q_DISABLE_COPY(QMozWindow)
};

#endif // QMOZWINDOW_H
