/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozwindow.h"

#include "qmozcontext.h"
#include "qmozwindow_p.h"

#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedLiteWindow.h"

#include <QDebug>

using namespace mozilla::embedlite;

namespace {

mozilla::ScreenRotation QtToMozillaRotation(Qt::ScreenOrientation orientation)
{
    switch (orientation) {
    case Qt::PrimaryOrientation:
    case Qt::PortraitOrientation:
        return mozilla::ROTATION_0;
    case Qt::LandscapeOrientation:
        return mozilla::ROTATION_90;
    case Qt::InvertedLandscapeOrientation:
        return mozilla::ROTATION_270;
    case Qt::InvertedPortraitOrientation:
        return mozilla::ROTATION_180;
    default:
        Q_UNREACHABLE();
        return mozilla::ROTATION_0;
    }
}

} // namespace

QMozWindow::QMozWindow(QObject* parent)
    : QObject(parent)
    , d(new QMozWindowPrivate(*this))
    , mOrientation(Qt::PrimaryOrientation)
{
    d->mWindow = QMozContext::GetInstance()->GetApp()->CreateWindow();
    d->mWindow->SetListener(d.data());
}

QMozWindow::~QMozWindow()
{
    d->mWindow->SetListener(nullptr);
    QMozContext::GetInstance()->GetApp()->DestroyWindow(d->mWindow);
    d->mWindow = nullptr;
}

void QMozWindow::setSize(QSize size)
{
    if (size != mSize) {
        mSize = size;
        d->mWindow->SetSize(size.width(), size.height());
    }
}

void QMozWindow::setContentOrientation(Qt::ScreenOrientation orientation)
{
    if (orientation != mOrientation) {
        mOrientation = orientation;
        d->mWindow->SetContentOrientation(QtToMozillaRotation(orientation));
    }
}

Qt::ScreenOrientation QMozWindow::contentOrientation() const
{
    return mOrientation;
}

void* QMozWindow::getPlatformImage(int* width, int* height)
{
    return d->mWindow->GetPlatformImage(width, height);
}

void QMozWindow::suspendRendering()
{
    d->mWindow->SuspendRendering();
}

void QMozWindow::resumeRendering()
{
    d->mWindow->ResumeRendering();
}

void QMozWindow::scheduleUpdate()
{
    d->mWindow->ScheduleUpdate();
}

bool QMozWindow::setReadyToPaint(bool ready)
{
    return d->setReadyToPaint(ready);
}

bool QMozWindow::readyToPaint() const
{
    return d->PreRender();
}
