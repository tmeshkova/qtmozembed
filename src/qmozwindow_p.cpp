/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozwindow_p.h"

#include "qmozwindow.h"

#include <QDebug>
#include <dlfcn.h>
#include <EGL/egl.h>

EGLContext (EGLAPIENTRY * _eglGetCurrentContext)(void) = nullptr;
EGLSurface (EGLAPIENTRY * _eglGetCurrentSurface)(EGLint readdraw) = nullptr;

QMozWindowPrivate::QMozWindowPrivate(QMozWindow& window)
    : q(window)
    , mWindow(nullptr)
    , mReadyToPaint(true)
{
}

QMozWindowPrivate::~QMozWindowPrivate()
{
}

bool QMozWindowPrivate::RequestGLContext(void*& context, void*& surface)
{
    q.requestGLContext();

    // The QPA interface does allow us to obtain the EGLContext from the
    // QOpenGLContet, but so far there is now way to do the same for
    // EGLSurface. It seems doing raw EGL calls is the only way.
    getEGLContext(context, surface);

    return true;
}

void QMozWindowPrivate::getEGLContext(void*& context, void*& surface)
{
    if (!_eglGetCurrentContext || !_eglGetCurrentSurface) {
        void* handle = dlopen("libEGL.so.1", RTLD_LAZY);
        if (!handle)
            return;

	*(void **)(&_eglGetCurrentContext) = dlsym(handle, "eglGetCurrentContext");
	*(void **)(&_eglGetCurrentSurface) = dlsym(handle, "eglGetCurrentSurface");

	Q_ASSERT(_eglGetCurrentContext && _eglGetCurrentSurface);

	dlclose(handle);
    }

    surface = _eglGetCurrentSurface(EGL_DRAW);
    context = _eglGetCurrentContext();
}

bool QMozWindowPrivate::setReadyToPaint(bool ready)
{
    QMutexLocker lock(&mReadyToPaintMutex);
    if (mReadyToPaint != ready) {
        mReadyToPaint = ready;
	return true;
    }
    return false;
}

void QMozWindowPrivate::WindowInitialized()
{
    q.initialized();
}

void QMozWindowPrivate::DrawUnderlay()
{
    q.drawUnderlay();
}

void QMozWindowPrivate::DrawOverlay(const nsIntRect& aRect)
{
    q.drawOverlay(QRect(aRect.x, aRect.y, aRect.width, aRect.height));
}

void QMozWindowPrivate::CompositorCreated()
{
    q.compositorCreated();
}

void QMozWindowPrivate::CompositingFinished()
{
    q.compositingFinished();
}

bool QMozWindowPrivate::PreRender()
{
    QMutexLocker lock(&mReadyToPaintMutex);
    return mReadyToPaint;
}
