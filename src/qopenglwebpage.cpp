/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qopenglwebpage.h"

#include <qglobal.h>
#include <qqmlinfo.h>
#include <QOpenGLContext>
#include <QOpenGLFunctions_ES2>
#include <QWindow>
#include <QGuiApplication>
#include <QScreen>

#include "mozilla/embedlite/EmbedLiteApp.h"

#include "qmozview_p.h"
#include "mozilla/embedlite/EmbedLiteWindow.h"
#include "qmozcontext.h"
#include "qmozembedlog.h"
#include "qmozgrabresult.h"
#include "qmozscrolldecorator.h"
#include "qmozwindow.h"
#include "qmozwindow_p.h"

#define LOG_COMPONENT "QOpenGLWebPage"

using namespace mozilla;
using namespace mozilla::embedlite;

/*!
    \fn void QOpenGLWebPage::QOpenGLWebPage(QObject *parent)

    In order to use this, embedlite.compositor.external_gl_context preference  needs to be set.
*/
QOpenGLWebPage::QOpenGLWebPage(QObject *parent)
  : QObject(parent)
  , d(new QMozViewPrivate(new IMozQView<QOpenGLWebPage>(*this), this))
  , mParentID(0)
  , mPrivateMode(false)
  , mActive(false)
  , mLoaded(false)
  , mCompleted(false)
  , mSizeUpdateScheduled(false)
  , mThrottlePainting(false)
{
    d->mContext = QMozContext::GetInstance();
    d->mHasContext = true;

    connect(this, SIGNAL(viewInitialized()), this, SLOT(processViewInitialization()));
    connect(this, SIGNAL(loadProgressChanged()), this, SLOT(updateLoaded()));
    connect(this, SIGNAL(loadingChanged()), this, SLOT(updateLoaded()));
}

QOpenGLWebPage::~QOpenGLWebPage()
{
    if (d->mView) {
        d->mView->SetListener(NULL);
        d->mContext->GetApp()->DestroyView(d->mView);
    }
    QMutexLocker lock(&mGrabResultListLock);
    mGrabResultList.clear();
    delete d;
}

void QOpenGLWebPage::updateLoaded()
{
    bool loaded = loadProgress() == 100 && !loading();
    if (mLoaded != loaded) {
        mLoaded = loaded;
        Q_EMIT loadedChanged();
    }
}

void QOpenGLWebPage::createView()
{
    LOGT("QOpenGLWebPage");
    if (!d->mView) {
        // We really don't care about SW rendering on Qt5 anymore
        d->mContext->GetApp()->SetIsAccelerated(true);
        EmbedLiteWindow* win = d->mMozWindow->d->mWindow;
        d->mView = d->mContext->GetApp()->CreateView(win, mParentID, mPrivateMode);
        d->mView->SetListener(d);
        d->mView->SetDPI(QGuiApplication::primaryScreen()->physicalDotsPerInch());
    }
}

void QOpenGLWebPage::processViewInitialization()
{
    // This is connected to view initialization. View must be initialized
    // over here.
    Q_ASSERT(d->mViewInitialized);

    mCompleted = true;
    setActive(true);
    Q_EMIT completedChanged();
}

void QOpenGLWebPage::onDrawOverlay(const QRect &rect)
{
    QMutexLocker lock(&mGrabResultListLock);
    QList<QWeakPointer<QMozGrabResult> >::const_iterator it = mGrabResultList.begin();
    for (; it != mGrabResultList.end(); ++it) {
        QSharedPointer<QMozGrabResult> result = it->toStrongRef();
        if (result) {
            result->captureImage(rect);
        } else {
            qWarning() << "QMozGrabResult freed before being realized!";
        }
    }
    mGrabResultList.clear();
}

int QOpenGLWebPage::parentId() const
{
    return mParentID;
}

bool QOpenGLWebPage::privateMode() const
{
    return mPrivateMode;
}

void QOpenGLWebPage::setPrivateMode(bool privateMode)
{
    if (d->mView) {
        // View is created directly in componentComplete() if mozcontext ready
        qmlInfo(this) << "privateMode cannot be changed after view is created";
        return;
    }

    if (privateMode != mPrivateMode) {
        mPrivateMode = privateMode;
        Q_EMIT privateModeChanged();
    }
}

bool QOpenGLWebPage::enabled() const
{
    return d->mEnabled;
}

void QOpenGLWebPage::setEnabled(bool enabled)
{
    if (d->mEnabled != enabled) {
        d->mEnabled = enabled;
        Q_EMIT enabledChanged();
    }
}

bool QOpenGLWebPage::active() const
{
    return mActive;
}

void QOpenGLWebPage::setActive(bool active)
{
    // WebPage is in inactive state until the view is initialized.
    // ::processViewInitialization always forces active state so we
    // can just ignore early activation calls.
    if (!d->mViewInitialized)
        return;

    if (mActive != active) {
        mActive = active;
        d->mView->SetIsActive(mActive);
        Q_EMIT activeChanged();
    }
}

bool QOpenGLWebPage::loaded() const
{
    return mLoaded;
}

QMozWindow *QOpenGLWebPage::mozWindow() const
{
    return d->mMozWindow;
}

void QOpenGLWebPage::setMozWindow(QMozWindow* window)
{
    d->setMozWindow(window);
    connect(window, &QMozWindow::drawOverlay,
            this, &QOpenGLWebPage::onDrawOverlay, Qt::DirectConnection);
}

bool QOpenGLWebPage::throttlePainting() const
{
    return mThrottlePainting;
}

void QOpenGLWebPage::setThrottlePainting(bool throttle)
{
    if (mThrottlePainting != throttle) {
        mThrottlePainting = throttle;
        d->SetThrottlePainting(throttle);
    }
}

/*!
    \fn void QOpenGLWebPage::initialize()

    Call initialize to complete web page creation.
*/
void QOpenGLWebPage::initialize()
{
    Q_ASSERT(d->mMozWindow);
    if (!d->mContext->initialized()) {
        connect(d->mContext, SIGNAL(onInitialized()), this, SLOT(createView()));
    } else {
        createView();
    }
}

bool QOpenGLWebPage::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::InputMethodQuery: {
        QInputMethodQueryEvent *query = static_cast<QInputMethodQueryEvent*>(event);
        Qt::InputMethodQueries queries = query->queries();
        for (int bit = 0; bit < 32; bit++) {
            Qt::InputMethodQuery q = (Qt::InputMethodQuery)(1<<bit);
            if (queries & q) query->setValue(q, inputMethodQuery(q));
        }
        query->accept();
        break;
    }
    case QEvent::InputMethod:
        inputMethodEvent(static_cast<QInputMethodEvent*>(event));
        break;
    case QEvent::FocusIn:
        focusInEvent(static_cast<QFocusEvent*>(event));
        break;
    case QEvent::FocusOut:
        focusOutEvent(static_cast<QFocusEvent*>(event));
        break;
    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent*>(event));
        break;
    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent*>(event));
        break;

    default:
        return QObject::event(event);
    }
    return true;
}

bool QOpenGLWebPage::completed() const
{
    return mCompleted;
}

void QOpenGLWebPage::update()
{
    if (!d->mViewInitialized) {
        return;
    }

    d->mMozWindow->scheduleUpdate();
}

void QOpenGLWebPage::forceActiveFocus()
{
    if (!d->mViewInitialized) {
        return;
    }

    setActive(true);
    d->SetIsFocused(true);
}

void QOpenGLWebPage::setInputMethodHints(Qt::InputMethodHints hints)
{
    d->mInputMethodHints = hints;
}

void QOpenGLWebPage::inputMethodEvent(QInputMethodEvent* event)
{
    d->inputMethodEvent(event);
}

void QOpenGLWebPage::keyPressEvent(QKeyEvent* event)
{
    d->keyPressEvent(event);
}

void QOpenGLWebPage::keyReleaseEvent(QKeyEvent* event)
{
    return d->keyReleaseEvent(event);
}

QVariant QOpenGLWebPage::inputMethodQuery(Qt::InputMethodQuery property) const
{
    return d->inputMethodQuery(property);
}

void QOpenGLWebPage::focusInEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    d->SetIsFocused(true);
}

void QOpenGLWebPage::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    d->SetIsFocused(false);
}

void QOpenGLWebPage::forceViewActiveFocus()
{
    forceActiveFocus();
}

QUrl QOpenGLWebPage::url() const
{
    return QUrl(d->mLocation);
}

void QOpenGLWebPage::setUrl(const QUrl& url)
{
    load(url.toString());
}

QString QOpenGLWebPage::title() const
{
    return d->mTitle;
}

int QOpenGLWebPage::loadProgress() const
{
    return d->mProgress;
}

bool QOpenGLWebPage::canGoBack() const
{
    return d->mCanGoBack;
}

bool QOpenGLWebPage::canGoForward() const
{
    return d->mCanGoForward;
}

bool QOpenGLWebPage::loading() const
{
    return d->mIsLoading;
}

QRectF QOpenGLWebPage::contentRect() const
{
    return d->mContentRect;
}

QSizeF QOpenGLWebPage::scrollableSize() const
{
    return d->mScrollableSize;
}

QPointF QOpenGLWebPage::scrollableOffset() const
{
    return d->mScrollableOffset;
}

float QOpenGLWebPage::resolution() const
{
    return d->mContentResolution;
}

bool QOpenGLWebPage::isPainted() const
{
    return d->mIsPainted;
}

QColor QOpenGLWebPage::bgcolor() const
{
    return d->mBgColor;
}

bool QOpenGLWebPage::getUseQmlMouse()
{
    return false;
}

void QOpenGLWebPage::setUseQmlMouse(bool value)
{
    Q_UNUSED(value);
}

bool QOpenGLWebPage::dragging() const
{
    return d->mDragging;
}

bool QOpenGLWebPage::moving() const
{
    return d->mMoving;
}

bool QOpenGLWebPage::pinching() const{
    return d->mPinching;
}

QMozScrollDecorator* QOpenGLWebPage::verticalScrollDecorator() const
{
    return &d->mVerticalScrollDecorator;
}

QMozScrollDecorator* QOpenGLWebPage::horizontalScrollDecorator() const
{
    return &d->mHorizontalScrollDecorator;
}

bool QOpenGLWebPage::chromeGestureEnabled() const
{
    return d->mChromeGestureEnabled;
}

void QOpenGLWebPage::setChromeGestureEnabled(bool value)
{
    if (value != d->mChromeGestureEnabled) {
        d->mChromeGestureEnabled = value;
        Q_EMIT chromeGestureEnabledChanged();
    }
}

qreal QOpenGLWebPage::chromeGestureThreshold() const
{
    return d->mChromeGestureThreshold;
}

void QOpenGLWebPage::setChromeGestureThreshold(qreal value)
{
    if (value != d->mChromeGestureThreshold) {
        d->mChromeGestureThreshold = value;
        Q_EMIT chromeGestureThresholdChanged();
    }
}

bool QOpenGLWebPage::chrome() const
{
    return d->mChrome;
}

void QOpenGLWebPage::setChrome(bool value)
{
    if (value != d->mChrome) {
        d->mChrome = value;
        Q_EMIT chromeChanged();
    }
}

qreal QOpenGLWebPage::contentWidth() const
{
    return d->mScrollableSize.width();
}

qreal QOpenGLWebPage::contentHeight() const
{
    return d->mScrollableSize.height();
}

QMargins QOpenGLWebPage::margins() const
{
    return d->mMargins;
}

void QOpenGLWebPage::setMargins(QMargins margins)
{
    d->SetMargins(margins);
}

void QOpenGLWebPage::loadHtml(const QString& html, const QUrl& baseUrl)
{
    LOGT();
}

void QOpenGLWebPage::goBack()
{
    if (!d->mViewInitialized)
        return;
    d->mView->GoBack();
}

void QOpenGLWebPage::goForward()
{
    if (!d->mViewInitialized)
        return;
    d->mView->GoForward();
}

void QOpenGLWebPage::stop()
{
    if (!d->mViewInitialized)
        return;
    d->mView->StopLoad();
}

void QOpenGLWebPage::reload()
{
    if (!d->mViewInitialized)
        return;
    d->ResetPainted();
    d->mView->Reload(false);
}

void QOpenGLWebPage::load(const QString& url)
{
    d->load(url);
}

void QOpenGLWebPage::sendAsyncMessage(const QString& name, const QVariant& variant)
{
    d->sendAsyncMessage(name, variant);
}

void QOpenGLWebPage::addMessageListener(const QString& name)
{
    d->addMessageListener(name);
}

void QOpenGLWebPage::addMessageListeners(const QStringList& messageNamesList)
{
    d->addMessageListeners(messageNamesList);
}

void QOpenGLWebPage::loadFrameScript(const QString& name)
{
    d->loadFrameScript(name);
}

void QOpenGLWebPage::newWindow(const QString& url)
{
    LOGT("New Window: %s", url.toUtf8().data());
}

quint32 QOpenGLWebPage::uniqueID() const
{
    return d->mView ? d->mView->GetUniqueID() : 0;
}

void QOpenGLWebPage::setParentID(unsigned aParentID)
{
    if (aParentID != mParentID) {
        mParentID = aParentID;
        Q_EMIT parentIdChanged();
    }
}

void QOpenGLWebPage::synthTouchBegin(const QVariant& touches)
{
    d->synthTouchBegin(touches);
}

void QOpenGLWebPage::synthTouchMove(const QVariant& touches)
{
    d->synthTouchMove(touches);
}

void QOpenGLWebPage::synthTouchEnd(const QVariant& touches)
{
    d->synthTouchEnd(touches);
}

void QOpenGLWebPage::suspendView()
{
    if (!d->mViewInitialized) {
        return;
    }
    setActive(false);
    d->mView->SuspendTimeouts();
}

void QOpenGLWebPage::resumeView()
{
    if (!d->mViewInitialized) {
        return;
    }
    setActive(true);
    d->mView->ResumeTimeouts();
}

void QOpenGLWebPage::recvMouseMove(int posX, int posY)
{
    d->recvMouseMove(posX, posY);
}

void QOpenGLWebPage::recvMousePress(int posX, int posY)
{
    d->recvMousePress(posX, posY);
}

void QOpenGLWebPage::recvMouseRelease(int posX, int posY)
{
    d->recvMouseRelease(posX, posY);
}

/*!
    \fn void QOpenGLWebPage::touchEvent(QTouchEvent *event)

    Touch events need to be in correctly mapped coordination
    system.
*/
void QOpenGLWebPage::touchEvent(QTouchEvent *event)
{
    d->touchEvent(event);
    event->accept();
}

void QOpenGLWebPage::timerEvent(QTimerEvent *event)
{
    d->timerEvent(event);
}
