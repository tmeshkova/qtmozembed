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

QMozWindow::QMozWindow(QObject* parent)
    : QObject(parent)
    , d(new QMozWindowPrivate(*this))
{
    d->mWindow = QMozContext::GetInstance()->GetApp()->CreateWindow();
    d->mWindow->SetListener(d.data());
}

QMozWindow::~QMozWindow()
{
    QMozContext::GetInstance()->GetApp()->DestroyWindow(d->mWindow);
    d->mWindow->SetListener(nullptr);
    d->mWindow = nullptr;
}

void QMozWindow::setSize(QSize size)
{
    if (size != mSize) {
        mSize = size;
        d->mWindow->SetSize(size.width(), size.height());
    }
}
