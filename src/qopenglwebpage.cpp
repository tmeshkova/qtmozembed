/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qopenglwebpage.h"

#include "qmozcontext.h"
#include "qmozembedlog.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

#include <qglobal.h>
#include <qqmlinfo.h>
#include <QOpenGLContext>
#include <QOpenGLFunctions_ES2>
#include <QWindow>

#include "qmozgrabresult.h"
#include "qgraphicsmozview_p.h"
#include "qmozscrolldecorator.h"

#define LOG_COMPONENT "QOpenGLWebPage"

using namespace mozilla;
using namespace mozilla::embedlite;

/*!
    \fn void QOpenGLWebPage::QOpenGLWebPage(QObject *parent)

    In order to use this, MOZ_USE_EXTERNAL_WINDOW environment variable needs to be set. The
    MOZ_USE_EXTERNAL_WINDOW will take higher precedence than MOZ_LAYERS_PREFER_OFFSCREEN.

    \sa QOpenGLWebPage::requestGLContext()
*/
QOpenGLWebPage::QOpenGLWebPage(QObject *parent)
  : QObject(parent)
  , d(new QGraphicsMozViewPrivate(new IMozQView<QOpenGLWebPage>(*this), this))
  , mParentID(0)
  , mPrivateMode(false)
  , mActive(false)
  , mLoaded(false)
  , mCompleted(false)
  , mWindow(0)
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
        if (mActive) {
            d->mView->ClearContent(255, 255, 255, 0);
        }
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
        d->mView = d->mContext->GetApp()->CreateView(mParentID, mPrivateMode);
        d->mView->SetListener(d);
    }
}

void QOpenGLWebPage::updateSize()
{
    Q_ASSERT(mSizeUpdateScheduled);
    d->UpdateViewSize();
    mSizeUpdateScheduled = false;
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

void QOpenGLWebPage::createGeckoGLContext()
{
}

/*!
    \fn void QOpenGLWebPage::requestGLContext()

    With in the slot connected to the requestGLContext() prepare QOpenGLContext and
    make it current context of the gecko compositor thread,
    against the surface of the QWindow.

    \code
    m_context = new QOpenGLContext();
    m_context->setFormat(requestedFormat());
    m_context->create();
    m_context->makeCurrent(this);
    \endcode

    This signal is emitted from the gecko compositor thread, you must make sure that
    the connection is direct (see Qt::ConnectionType)

    \sa QOpenGLContext::makeCurrent(QSurface*)
*/
void QOpenGLWebPage::requestGLContext(bool& hasContext, QSize& viewPortSize)
{
    hasContext = true;
    viewPortSize = d->mGLSurfaceSize;
    Q_EMIT requestGLContext();
}

/*!
    \fn void QOpenGLWebPage::drawUnderlay()

    Called always from gecko compositor thread. Current context
    has been made as current by gecko compositor.
*/
void QOpenGLWebPage::drawUnderlay()
{
    // Current context used by gecko compositor thread.
    QOpenGLContext *glContext = QOpenGLContext::currentContext();

    if (!glContext) {
        return;
    }

    QOpenGLFunctions_ES2* funcs = glContext->versionFunctions<QOpenGLFunctions_ES2>();
    if (funcs) {
        QColor bgColor = d->GetBackgroundColor();
        funcs->glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), 0.0);
        funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

/*!
    \fn void QOpenGLWebPage::afterRendering(const QRect &rect)

    This signal is emitted after web content has been rendered, before swapbuffers
    has been called.

    This signal can be used to paint using raw GL on top of the web content, or to do
    screen scraping of the current frame buffer.

    The GL context used for rendering is bound at this point.

    This signal is emitted from the gecko compositor thread, you must make sure that
    the connection is direct (see Qt::ConnectionType).
*/

/*!
    \fn void QOpenGLWebPage::drawOverlay(const QRect &rect)

    Called always from gecko compositor thread. Current context
    has been made as current by gecko compositor.
*/
void QOpenGLWebPage::drawOverlay(const QRect &rect)
{
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

    Q_EMIT afterRendering(rect);
}

void QOpenGLWebPage::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    LOGT("newGeometry size: [%g, %g] oldGeometry size: [%g,%g]", newGeometry.size().width(),
                                                                 newGeometry.size().height(),
                                                                 oldGeometry.size().width(),
                                                                 oldGeometry.size().height());
    setSize(newGeometry.size());
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
        mActive ? d->mView->ResumeRendering() : d->mView->SuspendRendering();
        Q_EMIT activeChanged();
    }
}

qreal QOpenGLWebPage::width() const
{
    return d->mSize.width();
}

void QOpenGLWebPage::setWidth(qreal width)
{
    QSizeF newSize(width, d->mSize.height());
    setSize(newSize);
}

qreal QOpenGLWebPage::height() const
{
    return d->mSize.height();
}

void QOpenGLWebPage::setHeight(qreal height)
{
    QSizeF newSize(d->mSize.width(), height);
    setSize(newSize);
}

QSizeF QOpenGLWebPage::size() const
{
    return d->mSize;
}

void QOpenGLWebPage::setSize(const QSizeF &size)
{
    if (d->mSize == size) {
        return;
    }

    bool widthWillChanged = d->mSize.width() != size.width();
    bool heightWillChanged = d->mSize.height() != size.height();

    d->mSize = size;
    scheduleSizeUpdate();

    if (widthWillChanged) {
        Q_EMIT widthChanged();
    }

    if (heightWillChanged) {
        Q_EMIT heightChanged();
    }

    Q_EMIT sizeChanged();
}

bool QOpenGLWebPage::loaded() const
{
    return mLoaded;
}

QWindow *QOpenGLWebPage::window() const
{
    return mWindow;
}

void QOpenGLWebPage::setWindow(QWindow *window)
{
    if (mWindow == window) {
        return;
    }

    mWindow = window;
    Q_EMIT windowChanged();
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

bool QOpenGLWebPage::Invalidate()
{
    return true;
}

void QOpenGLWebPage::CompositingFinished()
{
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

    d->mView->ScheduleUpdate();
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

void QOpenGLWebPage::updateContentOrientation(Qt::ScreenOrientation orientation)
{
    if (!mWindow) {
        qDebug() << "No window set, cannot update content orientation.";
        return;
    }

    QSize surfaceSize;
    QSize windowSize = mWindow->size();

    int minValue = qMin(windowSize.width(), windowSize.height());
    int maxValue = qMax(windowSize.width(), windowSize.height());

    if (orientation == Qt::LandscapeOrientation || orientation == Qt::InvertedLandscapeOrientation) {
        surfaceSize.setWidth(maxValue);
        surfaceSize.setHeight(minValue);
    } else {
        surfaceSize.setWidth(minValue);
        surfaceSize.setHeight(maxValue);
    }

    setSize(surfaceSize);
    setSurfaceSize(surfaceSize, orientation);
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

    if (mParentID) {
        createView();
    }
}

void QOpenGLWebPage::synthTouchBegin(const QVariant& touches)
{
    Q_UNUSED(touches);
}

void QOpenGLWebPage::synthTouchMove(const QVariant& touches)
{
    Q_UNUSED(touches);
}

void QOpenGLWebPage::synthTouchEnd(const QVariant& touches)
{
    Q_UNUSED(touches);
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
    Q_ASSERT_X(false, "QOpenGLWebPage", "calling recvMouseMove not supported!");
}

void QOpenGLWebPage::recvMousePress(int posX, int posY)
{
    Q_ASSERT_X(false, "QOpenGLWebPage", "calling recvMousePress not supported!");
}

void QOpenGLWebPage::recvMouseRelease(int posX, int posY)
{
    Q_ASSERT_X(false, "QOpenGLWebPage", "calling recvMouseRelease not supported!");
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

void QOpenGLWebPage::scheduleSizeUpdate()
{
    if (!mSizeUpdateScheduled) {
        QMetaObject::invokeMethod(this, "updateSize", Qt::QueuedConnection);
        mSizeUpdateScheduled = true;
    }
}

/*!
    \fn void QOpenGLWebPage::setSurfaceSize()

    Sets surface size and orientation.

    Set surface size as soon as the page is created. The page cannot be
    shown until surface is given.
*/
void QOpenGLWebPage::setSurfaceSize(const QSize &surfaceSize, Qt::ScreenOrientation orientation)
{
    if ((d->mGLSurfaceSize != surfaceSize) || (d->mOrientation != orientation)) {
        d->mGLSurfaceSize = surfaceSize;
        d->mOrientation = orientation;
        d->mOrientationDirty = true;
        scheduleSizeUpdate();
    }
}
