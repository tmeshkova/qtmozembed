/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QThread>
#include "qmcthreadobject.h"
#include "qsgthreadobject.h"
#include "quickmozview.h"

QMCThreadObject::QMCThreadObject(QuickMozView* aView, QSGThreadObject* sgThreadObj)
  : mView(aView)
  , mSGThreadObj(sgThreadObj)
{
}

void QMCThreadObject::checkIfHasTexture()
{
    printf(">>>>>>Func:%s::%d Thr:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread());
    int textureID = 0, width = 0, height = 0;
    if (mView->GetPendingTexture(mSGThreadObj->GetTargetContextWrapper(), &textureID, &width, &height)) {
      Q_EMIT textureReady(textureID, QSize(width, height));
    }
}
