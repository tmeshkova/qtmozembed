/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QMOZWINDOW_PRIVATE_H
#define QMOZWINDOW_PRIVATE_H

#include <QObject>

#include "mozilla/embedlite/EmbedLiteWindow.h"

class QMozWindow;

class QMozWindowPrivate : public mozilla::embedlite::EmbedLiteWindowListener
{
public:
    QMozWindowPrivate(QMozWindow&);
    virtual ~QMozWindowPrivate();

protected:
    // EmbedLiteWindowListener:
    bool RequestGLContext(void*& context, void*& surface) override;
    void WindowInitialized() override;
    void DrawUnderlay() override;
    void DrawOverlay(const nsIntRect& aRect) override;
    bool PreRender() override;
    void CompositorCreated() override;
    void CompositingFinished() override;
    bool Invalidate() override;

private:
    friend class QMozWindow;
    friend class QOpenGLWebPage;
    friend class QuickMozView;

    void GetEGLContext(void*& context, void*& surface);

    QMozWindow& q;
    mozilla::embedlite::EmbedLiteWindow* mWindow;

    Q_DISABLE_COPY(QMozWindowPrivate)
};

#endif // QMOZWINDOW_PRIVATE_H

