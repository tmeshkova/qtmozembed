/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QSGThreadObject_H
#define QSGThreadObject_H

#include <QObject>
#include <QSize>
#include <QtGui/QOpenGLContext>

namespace mozilla {
namespace embedlite {
class EmbedLiteRenderTarget;
}}

class QSGThreadObject : public QObject
{
    Q_OBJECT
public:
    QSGThreadObject();
    virtual ~QSGThreadObject();

    mozilla::embedlite::EmbedLiteRenderTarget* getTargetContextWrapper() { return mRenderTarget; }

private:
    void wrapRenderThreadGLContext();

    mozilla::embedlite::EmbedLiteRenderTarget* mRenderTarget;
};

#endif // QSGThreadObject_H
