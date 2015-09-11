/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozwindow_p.h"

#include "qmozwindow.h"

#include <QDebug>
#include <QGuiApplication>
#include <dlfcn.h>
#include <EGL/egl.h>
#if defined(ENABLE_GLX)
#include <GL/glx.h>
#endif

EGLContext (EGLAPIENTRY * _eglGetCurrentContext)(void) = nullptr;
EGLSurface (EGLAPIENTRY * _eglGetCurrentSurface)(EGLint readdraw) = nullptr;
#if defined(ENABLE_GLX)
GLXContext (*_glxGetCurrentContext)(void) = nullptr;
GLXDrawable (*_glxGetCurrentDrawable)(void) = nullptr;
#endif

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

    QString platform = qApp->platformName().toLower();

    if (platform == "wayland" || platform == "wayland-egl") {
        getEGLContext(context, surface);
        return true;
    }
#if defined(ENABLE_GLX)
    if (platform == "xcb") {
        getGLXContext(context, surface);
        return true;
    }
#endif

    qCritical() << "Unsupported QPA platform type:" << platform;
    return false;
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

#if defined(ENABLE_GLX)
void QMozWindowPrivate::getGLXContext(void*& context, void*& surface)
{
    if (!_glxGetCurrentContext || !_glxGetCurrentDrawable) {
        void* handle = dlopen("libGL.so.1", RTLD_LAZY);
        if (!handle)
            return;

        *(void **)(&_glxGetCurrentContext) = dlsym(handle, "glXGetCurrentContext");
        *(void **)(&_glxGetCurrentDrawable) = dlsym(handle, "glXGetCurrentDrawable");

        Q_ASSERT(_glxGetCurrentContext && _glxGetCurrentDrawable);

        dlclose(handle);
    }

    surface = reinterpret_cast<void*>(_glxGetCurrentDrawable());
    context = _glxGetCurrentContext();
}
#endif

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
