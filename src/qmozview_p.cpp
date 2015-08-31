/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QMozViewPrivate"

#include <QTouchEvent>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonParseError>

#include "qmozview_p.h"
#include "qmozcontext.h"
#include "qmozwindow.h"
#include "EmbedQtKeyUtils.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/gfx/Tools.h"
#include "mozilla/WidgetUtils.h"
#include "qmozembedlog.h"
#include <sys/time.h>
#include "mozilla/TimeStamp.h"

#ifndef MOZVIEW_FLICK_THRESHOLD
#define MOZVIEW_FLICK_THRESHOLD 200
#endif

#ifndef MOZVIEW_FLICK_STOP_TIMEOUT
#define MOZVIEW_FLICK_STOP_TIMEOUT 500
#endif

#define SCROLL_EPSILON 0.001

using namespace mozilla;
using namespace mozilla::embedlite;

qint64 current_timestamp(QTouchEvent* aEvent)
{
    if (aEvent) {
        return aEvent->timestamp();
    }

    struct timeval te;
    gettimeofday(&te, NULL);
    qint64 milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

QMozViewPrivate::QMozViewPrivate(IMozQViewIface* aViewIface, QObject *publicPtr)
    : mViewIface(aViewIface)
    , q(publicPtr)
    , mMozWindow(NULL)
    , mContext(NULL)
    , mView(NULL)
    , mViewInitialized(false)
    , mBgColor(Qt::white)
    , mTempTexture(NULL)
    , mEnabled(true)
    , mChromeGestureEnabled(true)
    , mChromeGestureThreshold(0.0)
    , mChrome(true)
    , mMoveDelta(0.0)
    , mDragStartY(0.0)
    , mMoving(false)
    , mPinching(false)
    , mLastTimestamp(0)
    , mLastStationaryTimestamp(0)
    , mCanFlick(false)
    , mPendingTouchEvent(false)
    , mProgress(0)
    , mCanGoBack(false)
    , mCanGoForward(false)
    , mIsLoading(false)
    , mLastIsGoodRotation(true)
    , mIsPasswordField(false)
    , mGraphicsViewAssigned(false)
    , mContentRect(0.0, 0.0, 0.0, 0.0)
    , mScrollableSize(0.0, 0.0)
    , mScrollableOffset(0,0)
    , mContentResolution(0.0)
    , mIsPainted(false)
    , mInputMethodHints(0)
    , mIsInputFieldFocused(false)
    , mPreedit(false)
    , mViewIsFocused(false)
    , mHasContext(false)
    , mGLSurfaceSize(0,0)
    , mOrientation(Qt::PrimaryOrientation)
    , mOrientationDirty(false)
    , mPressed(false)
    , mDragging(false)
    , mFlicking(false)
    , mMovingTimerId(0)
    , mOffsetX(0.0)
    , mOffsetY(0.0)
{
}

QMozViewPrivate::~QMozViewPrivate()
{
    delete mViewIface;
}

void QMozViewPrivate::CompositorCreated()
{
    mViewIface->createGeckoGLContext();
}

void QMozViewPrivate::DrawUnderlay()
{
    mViewIface->drawUnderlay();
}

void QMozViewPrivate::DrawOverlay(const nsIntRect& aRect)
{
    QRect rect(aRect.x, aRect.y, aRect.width, aRect.height);
    mViewIface->drawOverlay(rect);
}

void QMozViewPrivate::UpdateScrollArea(unsigned int aWidth, unsigned int aHeight, float aPosX, float aPosY)
{
    bool widthChanged = false;
    bool heightChanged = false;
    // Emit changes only after both values have been updated.
    if (mScrollableSize.width() != aWidth) {
        mScrollableSize.setWidth(aWidth);
        widthChanged = true;
    }

    if (mScrollableSize.height() != aHeight) {
        mScrollableSize.setHeight(aHeight);
        heightChanged = true;
    }

    if (!gfx::FuzzyEqual(mScrollableOffset.x(), aPosX, SCROLL_EPSILON) ||
        !gfx::FuzzyEqual(mScrollableOffset.y(), aPosY, SCROLL_EPSILON)) {

        mScrollableOffset.setX(aPosX);
        mScrollableOffset.setY(aPosY);
        mViewIface->scrollableOffsetChanged();

        if (mEnabled) {
            // We could add moving timers for both of these and check them separately.
            // Currently we have only one timer event for content.
            mVerticalScrollDecorator.setMoving(true);
            mHorizontalScrollDecorator.setMoving(true);

            // Update vertical scroll decorator
            qreal ySizeRatio = mContentRect.height() * mContentResolution / mScrollableSize.height();
            qreal tmpValue = mMozWindow->size().height() * ySizeRatio;
            mVerticalScrollDecorator.setSize(tmpValue);
            tmpValue = mScrollableOffset.y() * ySizeRatio;
            mVerticalScrollDecorator.setPosition(tmpValue);

            // Update horizontal scroll decorator
            qreal xSizeRatio = mContentRect.width() * mContentResolution / mScrollableSize.width();
            tmpValue = mMozWindow->size().width() * xSizeRatio;
            mHorizontalScrollDecorator.setSize(tmpValue);
            tmpValue = mScrollableOffset.x() * xSizeRatio;
            mHorizontalScrollDecorator.setPosition(tmpValue);
        }
    }

    if (widthChanged) {
        mViewIface->contentWidthChanged();
    }

    if (heightChanged) {
        mViewIface->contentHeightChanged();
    }
}

void QMozViewPrivate::TestFlickingMode(QTouchEvent *event)
{
    QPointF touchPoint = event->touchPoints().size() == 1 ? event->touchPoints().at(0).pos() : QPointF();
    // Only for single press point
    if (!touchPoint.isNull()) {
        if (event->type() == QEvent::TouchBegin) {
            mLastTimestamp = mLastStationaryTimestamp = current_timestamp(event);
            mCanFlick = true;
        } else if (event->type() == QEvent::TouchUpdate && !mLastPos.isNull()) {
            QRectF pressArea = event->touchPoints().at(0).rect();
            qreal touchHorizontalThreshold = pressArea.width() * 2;
            qreal touchVerticalThreshold = pressArea.height() * 2;
            if (!mLastStationaryPos.isNull() && (qAbs(mLastStationaryPos.x() - touchPoint.x()) > touchHorizontalThreshold
                                             || qAbs(mLastStationaryPos.y() - touchPoint.y()) > touchVerticalThreshold)) {
                // Threshold exceeded. Reset stationary position and time.
                mLastStationaryTimestamp = current_timestamp(event);
                mLastStationaryPos = touchPoint;
            } else if (qAbs(mLastPos.x() - touchPoint.x()) <= touchHorizontalThreshold && qAbs(mLastPos.y() - touchPoint.y()) <= touchVerticalThreshold) {
                // Handle stationary position when panning stops and continues. Eventually mCanFlick is based on timestamps between events, see touch end block.
                if (mCanFlick) {
                    mLastStationaryTimestamp = current_timestamp(event);
                    mLastStationaryPos = touchPoint;
                }
                mCanFlick = false;
            }
            else {
                mCanFlick = true;
            }
            mLastTimestamp = current_timestamp(event);
        } else if (event->type() == QEvent::TouchEnd) {
            mCanFlick =(qint64(current_timestamp(event) - mLastTimestamp) < MOZVIEW_FLICK_THRESHOLD) &&
                    (qint64(current_timestamp(event) - mLastStationaryTimestamp) < MOZVIEW_FLICK_THRESHOLD);
            mLastStationaryPos = QPointF();
        }
    }
    mLastPos = touchPoint;
}

void QMozViewPrivate::HandleTouchEnd(bool &draggingChanged, bool &pinchingChanged)
{
    if (mDragging) {
        mDragging = false;
        draggingChanged = true;
    }

    // Currently change from 2> fingers to 1 finger does not
    // allow moving content. Hence, keep pinching enabled
    // also when there is one finger left when releasing
    // fingers and only stop pinching when touch ends.
    // You can continue pinching by adding second finger.
    if (mPinching) {
        mPinching = false;
        pinchingChanged = true;
    }
}

void QMozViewPrivate::ResetState()
{
    // Invalid initial drag start Y.
    mDragStartY = -1.0;
    mMoveDelta = 0.0;

    mFlicking = false;
    UpdateMoving(false);
    mVerticalScrollDecorator.setMoving(false);
    mHorizontalScrollDecorator.setMoving(false);
}

void QMozViewPrivate::UpdateMoving(bool moving)
{
    if (mMoving != moving) {
        mMoving = moving;

        if (mMoving && q) {
            startMoveMonitor();
        }
        mViewIface->movingChanged();
    }
}

void QMozViewPrivate::ResetPainted()
{
    if (mIsPainted) {
        mIsPainted = false;
        mViewIface->firstPaint(-1, -1);
    }
}

void QMozViewPrivate::load(const QString &url)
{
    if (url.isEmpty())
        return;

    if (!mViewInitialized) {
        mPendingUrl = url;
        return;
    }
    LOGT("url: %s", url.toUtf8().data());
    mProgress = 0;
    ResetPainted();
    mView->LoadURL(url.toUtf8().data());
}

void QMozViewPrivate::loadFrameScript(const QString &frameScript)
{
    if (!mViewInitialized) {
        mPendingFrameScripts.append(frameScript);
    } else {
        mView->LoadFrameScript(frameScript.toUtf8().data());
    }
}

void QMozViewPrivate::addMessageListener(const QString &name)
{
    if (!mViewInitialized) {
        mPendingMessageListeners.append(name);
        return;
    }

    mView->AddMessageListener(name.toUtf8().data());
}

void QMozViewPrivate::addMessageListeners(const QStringList &messageNamesList)
{
    if (!mViewInitialized) {
        mPendingMessageListeners.append(messageNamesList);
        return;
    }

    nsTArray<nsString> messages;
    for (int i = 0; i < messageNamesList.size(); i++) {
        messages.AppendElement((char16_t*)messageNamesList.at(i).data());
    }
    mView->AddMessageListeners(messages);
}

void QMozViewPrivate::timerEvent(QTimerEvent *event)
{
    Q_ASSERT(q);
    if (event->timerId() == mMovingTimerId) {
        qreal offsetY = mScrollableOffset.y();
        qreal offsetX = mScrollableOffset.x();
        if (offsetX == mOffsetX && offsetY == mOffsetY) {
            ResetState();
            q->killTimer(mMovingTimerId);
            mMovingTimerId = 0;
        }
        mOffsetX = offsetX;
        mOffsetY = offsetY;
        event->accept();
    }
}

void QMozViewPrivate::startMoveMonitor()
{
    Q_ASSERT(q);
    // Kill running move monitor.
    if (mMovingTimerId > 0) {
        q->killTimer(mMovingTimerId);;
        mMovingTimerId = 0;
    }
    mMovingTimerId = q->startTimer(MOZVIEW_FLICK_STOP_TIMEOUT);
    mFlicking = true;
}

QVariant QMozViewPrivate::inputMethodQuery(Qt::InputMethodQuery property) const
{
    switch (property) {
    case Qt::ImEnabled:
        return QVariant((bool) mIsInputFieldFocused);
    case Qt::ImHints:
        return QVariant((int) mInputMethodHints);
    default:
        return QVariant();
    }
}

void QMozViewPrivate::inputMethodEvent(QInputMethodEvent *event)
{
    LOGT("cStr:%s, preStr:%s, replLen:%i, replSt:%i", event->commitString().toUtf8().data(), event->preeditString().toUtf8().data(), event->replacementLength(), event->replacementStart());
    mPreedit = !event->preeditString().isEmpty();
    if (mViewInitialized) {
        if (mInputMethodHints & Qt::ImhFormattedNumbersOnly || mInputMethodHints & Qt::ImhDialableCharactersOnly) {
            bool ok;
            int asciiNumber = event->commitString().toInt(&ok) + Qt::Key_0;

            if (ok) {
                int32_t domKeyCode = MozKey::QtKeyCodeToDOMKeyCode(asciiNumber, Qt::NoModifier);
                int32_t charCode = 0;

                if (event->commitString().length() && event->commitString()[0].isPrint()) {
                    charCode = (int32_t)event->commitString()[0].unicode();
                }
                mView->SendKeyPress(domKeyCode, 0, charCode);
                mView->SendKeyRelease(domKeyCode, 0, charCode);
                qGuiApp->inputMethod()->reset();
            } else {
                mView->SendTextEvent(event->commitString().toUtf8().data(), event->preeditString().toUtf8().data());
            }
        } else {
            if (event->commitString().isEmpty()) {
                mView->SendTextEvent(event->commitString().toUtf8().data(), event->preeditString().toUtf8().data());
            } else {
                mView->SendTextEvent(event->commitString().toUtf8().data(), event->preeditString().toUtf8().data());
                // After commiting pre-edit, we send "dummy" keypress.
                // Workaround for sites that enable "submit" button based on keypress events like
                // comment fields in FB, and m.linkedin.com
                // Chrome on Android does the same, but it does it also after each pre-edit change
                // We cannot do exectly the same here since sending keyevent with active pre-edit would commit gecko's
                // internal Input Engine's pre-edit
                mView->SendKeyPress(0, 0, 0);
                mView->SendKeyRelease(0, 0, 0);
            }
        }
    }
}

void QMozViewPrivate::keyPressEvent(QKeyEvent *event)
{
    if (!mViewInitialized)
        return;

    int32_t gmodifiers = MozKey::QtModifierToDOMModifier(event->modifiers());
    int32_t domKeyCode = MozKey::QtKeyCodeToDOMKeyCode(event->key(), event->modifiers());
    int32_t charCode = 0;
    if (event->text().length() && event->text()[0].isPrint()) {
        charCode = (int32_t)event->text()[0].unicode();
        if (getenv("USE_TEXT_EVENTS")) {
            return;
        }
    }
    mView->SendKeyPress(domKeyCode, gmodifiers, charCode);
}

void QMozViewPrivate::keyReleaseEvent(QKeyEvent *event)
{
    if (!mViewInitialized)
        return;

    int32_t gmodifiers = MozKey::QtModifierToDOMModifier(event->modifiers());
    int32_t domKeyCode = MozKey::QtKeyCodeToDOMKeyCode(event->key(), event->modifiers());
    int32_t charCode = 0;
    if (event->text().length() && event->text()[0].isPrint()) {
        charCode = (int32_t)event->text()[0].unicode();
        if (getenv("USE_TEXT_EVENTS")) {
            mView->SendTextEvent(event->text().toUtf8().data(), "");
            return;
        }
    }
    mView->SendKeyRelease(domKeyCode, gmodifiers, charCode);
}

void QMozViewPrivate::sendAsyncMessage(const QString &name, const QVariant &variant)
{
    if (!mViewInitialized)
        return;

    QJsonDocument doc = QJsonDocument::fromVariant(variant);
    QByteArray array = doc.toJson();

    mView->SendAsyncMessage((const char16_t*)name.constData(), NS_ConvertUTF8toUTF16(array.constData()).get());
}

void QMozViewPrivate::UpdateViewSize()
{
    mSize = mMozWindow->size();
}

bool QMozViewPrivate::RequestCurrentGLContext()
{
    QSize unused;
    return RequestCurrentGLContext(unused);
}

bool QMozViewPrivate::RequestCurrentGLContext(QSize& aViewPortSize)
{
    bool hasContext = false;
    mViewIface->requestGLContext(hasContext, aViewPortSize);
    return hasContext;
}

void QMozViewPrivate::ViewInitialized()
{
    mViewInitialized = true;

    Q_FOREACH (const QString &listener, mPendingMessageListeners) {
        addMessageListener(listener);
    }
    mPendingMessageListeners.clear();

    Q_FOREACH (const QString &frameScript, mPendingFrameScripts) {
        loadFrameScript(frameScript);
    }
    mPendingFrameScripts.clear();

    if (!mPendingUrl.isEmpty()) {
        load(mPendingUrl);
        mPendingUrl.clear();
    }

    UpdateViewSize();
    // This is currently part of official API, so let's subscribe to these messages by default
    mViewIface->viewInitialized();
    mViewIface->canGoBackChanged();
    mViewIface->canGoForwardChanged();
}

void QMozViewPrivate::SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    QMutexLocker locker(&mBgColorMutex);
    mBgColor = QColor(r, g, b, a);
    mViewIface->bgColorChanged();
}

void QMozViewPrivate::SetMargins(const QMargins& margins)
{
  if (margins != mMargins) {
    mMargins = margins;
    mView->SetMargins(margins.top(), margins.right(), margins.bottom(), margins.left());
    mViewIface->marginsChanged();
  }
}

// Can be read for instance from gecko compositor thread.
QColor QMozViewPrivate::GetBackgroundColor() const
{
    QMutexLocker locker(&mBgColorMutex);
    return mBgColor;
}

bool QMozViewPrivate::Invalidate()
{
    return mViewIface->Invalidate();
}

void QMozViewPrivate::CompositingFinished()
{
    mViewIface->CompositingFinished();
}

void QMozViewPrivate::OnLocationChanged(const char* aLocation, bool aCanGoBack, bool aCanGoForward)
{
    if (mCanGoBack != aCanGoBack) {
        mCanGoBack = aCanGoBack;
        mViewIface->canGoBackChanged();
    }

    if (mCanGoForward != aCanGoForward) {
        mCanGoForward = aCanGoForward;
        mViewIface->canGoForwardChanged();
    }

    if (mLocation != aLocation) {
        mLocation = QString(aLocation);
        mViewIface->urlChanged();
    }
}

void QMozViewPrivate::OnLoadProgress(int32_t aProgress, int32_t aCurTotal, int32_t aMaxTotal)
{
    if (mIsLoading) {
        mProgress = aProgress;
        mViewIface->loadProgressChanged();
    }
}

void QMozViewPrivate::OnLoadStarted(const char* aLocation)
{
    Q_UNUSED(aLocation);

    ResetPainted();

    if (!mIsLoading) {
        mIsLoading = true;
        mProgress = 1;
        mViewIface->loadingChanged();
    }
}

void QMozViewPrivate::OnLoadFinished(void)
{
    if (mIsLoading) {
        mProgress = 100;
        mIsLoading = false;
        mViewIface->loadingChanged();
    }
}

void QMozViewPrivate::OnWindowCloseRequested()
{
    mViewIface->windowCloseRequested();
}

// View finally destroyed and deleted
void QMozViewPrivate::ViewDestroyed()
{
    LOGT();
    mView = NULL;
    mViewInitialized = false;
    mViewIface->viewDestroyed();
}

void QMozViewPrivate::RecvAsyncMessage(const char16_t* aMessage, const char16_t* aData)
{
    NS_ConvertUTF16toUTF8 message(aMessage);
    NS_ConvertUTF16toUTF8 data(aData);

    bool ok = false;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(data.get()), &error);
    ok = error.error == QJsonParseError::NoError;
    QVariant vdata = doc.toVariant();

    if (ok) {
        LOGT("mesg:%s, data:%s", message.get(), data.get());
        mViewIface->recvAsyncMessage(message.get(), vdata);
    } else {
        LOGT("parse: err:%s, errLine:%i", error.errorString().toUtf8().data(), error.offset);
    }
}

char* QMozViewPrivate::RecvSyncMessage(const char16_t* aMessage, const char16_t*  aData)
{
    QMozReturnValue response;
    NS_ConvertUTF16toUTF8 message(aMessage);
    NS_ConvertUTF16toUTF8 data(aData);

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(data.get()), &error);
    Q_ASSERT(error.error == QJsonParseError::NoError);
    QVariant vdata = doc.toVariant();

    mViewIface->recvSyncMessage(message.get(), vdata, &response);

    QJsonDocument respdoc = QJsonDocument::fromVariant(response.getMessage());
    QByteArray array = respdoc.toJson();

    LOGT("msg:%s, response:%s", message.get(), array.constData());
    return strdup(array.constData());
}

void QMozViewPrivate::OnLoadRedirect(void)
{
    LOGT();
    mViewIface->loadRedirect();
}

void QMozViewPrivate::OnSecurityChanged(const char* aStatus, unsigned int aState)
{
    LOGT();
    mViewIface->securityChanged(aStatus, aState);
}
void QMozViewPrivate::OnFirstPaint(int32_t aX, int32_t aY)
{
    LOGT();
    mIsPainted = true;
    mViewIface->firstPaint(aX, aY);
}

void QMozViewPrivate::SetIsFocused(bool aIsFocused)
{
    mViewIsFocused = aIsFocused;
    if (mViewInitialized) {
        mView->SetIsFocused(aIsFocused);
    }
}

void QMozViewPrivate::SetThrottlePainting(bool aThrottle)
{
    if (mViewInitialized) {
        mView->SetThrottlePainting(aThrottle);
    }
}

void QMozViewPrivate::IMENotification(int aIstate, bool aOpen, int aCause, int aFocusChange,
                                              const char16_t* inputType, const char16_t* inputMode)
{
    Qt::InputMethodHints hints = Qt::ImhNone;
    hints = aIstate == 2 ? Qt::ImhHiddenText : Qt::ImhPreferLowercase;

    QString imType((QChar*)inputType);
    if (imType.contains("number", Qt::CaseInsensitive)) {
        //hints |= Qt::ImhDigitsOnly;
        hints |= Qt::ImhFormattedNumbersOnly;
    }
    else if (imType.contains("tel", Qt::CaseInsensitive)) {
        hints |= Qt::ImhDialableCharactersOnly;
    }
    else if (imType.contains("email", Qt::CaseInsensitive)) {
        hints |= Qt::ImhEmailCharactersOnly;
    }
    else if (imType.contains("url", Qt::CaseInsensitive)) {
        hints |= Qt::ImhUrlCharactersOnly;
    }

    mViewIface->setInputMethodHints(hints);
    if (aFocusChange) {
        mIsInputFieldFocused = aIstate;
        if (mViewIsFocused) {
#ifndef QT_NO_IM
            QInputMethod* inputContext = qGuiApp->inputMethod();
            if (!inputContext) {
                LOGT("Requesting SIP: but no input context");
                return;
            }
            inputContext->update(Qt::ImEnabled);
            if (aIstate) {
                inputContext->show();
            } else {
                inputContext->hide();
            }
            inputContext->update(Qt::ImQueryAll);
#endif
        }

    }
    mViewIface->imeNotification(aIstate, aOpen, aCause, aFocusChange, imType);
}

void QMozViewPrivate::GetIMEStatus(int32_t* aIMEEnabled, int32_t* aIMEOpen, intptr_t* aNativeIMEContext)
{
    *aNativeIMEContext = (intptr_t)qApp->inputMethod();
}

void QMozViewPrivate::OnScrolledAreaChanged(unsigned int aWidth, unsigned int aHeight)
{
    LOGT("sz[%u,%u]", aWidth, aHeight);
    Q_UNUSED(aWidth)
    Q_UNUSED(aHeight)
}

void QMozViewPrivate::OnScrollChanged(int32_t offSetX, int32_t offSetY)
{
}

void QMozViewPrivate::OnTitleChanged(const char16_t* aTitle)
{
    mTitle = QString((QChar*)aTitle);
    mViewIface->titleChanged();
}

void QMozViewPrivate::SetFirstPaintViewport(const nsIntPoint& aOffset, float aZoom,
                                                    const nsIntRect& aPageRect, const gfxRect& aCssPageRect)
{
    LOGT();
}

void QMozViewPrivate::SyncViewportInfo(const nsIntRect& aDisplayPort,
                                               float aDisplayResolution, bool aLayersUpdated,
                                               nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY)
{
    LOGT("viewport display port[%d,%d,%d,%d]", aDisplayPort.x, aDisplayPort.y, aDisplayPort.width, aDisplayPort.height);
}

void QMozViewPrivate::SetPageRect(const gfxRect& aCssPageRect)
{
    LOGT();
}

bool QMozViewPrivate::SendAsyncScrollDOMEvent(const gfxRect& aContentRect, const gfxSize& aScrollableSize)
{
    mContentResolution = mMozWindow->size().width() / aContentRect.width;

    if (mContentRect.x() != aContentRect.x || mContentRect.y() != aContentRect.y ||
            mContentRect.width() != aContentRect.width ||
            mContentRect.height() != aContentRect.height) {
        mContentRect.setRect(aContentRect.x, aContentRect.y, aContentRect.width, aContentRect.height);
        mViewIface->viewAreaChanged();
        // chrome, chromeGestureEnabled, and chromeGestureThreshold can be used
        // to control chrome/chromeless mode.
        // When chromeGestureEnabled is false, no actions are taken
        // When chromeGestureThreshold is true, chrome is set false when chromeGestrureThreshold is exceeded (pan/flick)
        // and set to true when flicking/panning the same amount to the the opposite direction.
        // This do not have relationship to HTML5 fullscreen API.
        if (mEnabled && mChromeGestureEnabled && mDragStartY >= 0.0) {
            // In MozView coordinates
            qreal offset = aContentRect.y * mContentResolution;
            qreal currentDelta = offset - mDragStartY;
            LOGT("dragStartY: %f, %f, %f, %f, %d", mDragStartY, offset, currentDelta, mMoveDelta, (qAbs(currentDelta) < mMoveDelta));

            if (qAbs(currentDelta) < mMoveDelta) {
                mDragStartY = offset;
            }

            if (currentDelta > mChromeGestureThreshold) {
                LOGT("currentDelta > mChromeGestureThreshold: %d", mChrome);
                if (mChrome) {
                    mChrome = false;
                    mViewIface->chromeChanged();
                }
            } else if (currentDelta < -mChromeGestureThreshold) {
                LOGT("currentDelta < -mChromeGestureThreshold: %d", mChrome);
                if (!mChrome) {
                    mChrome = true;
                    mViewIface->chromeChanged();
                }
            }
            mMoveDelta = qAbs(currentDelta);
        }
    }


    UpdateScrollArea(aScrollableSize.width * mContentResolution, aScrollableSize.height * mContentResolution,
                     aContentRect.x * mContentResolution, aContentRect.y * mContentResolution);
    return false;
}

bool QMozViewPrivate::HandleLongTap(const nsIntPoint& aPoint)
{
    QMozReturnValue retval;
    retval.setMessage(false);
    mViewIface->handleLongTap(QPoint(aPoint.x, aPoint.y), &retval);
    return retval.getMessage().toBool();
}

bool QMozViewPrivate::HandleSingleTap(const nsIntPoint& aPoint)
{
    QMozReturnValue retval;
    retval.setMessage(false);
    mViewIface->handleSingleTap(QPoint(aPoint.x, aPoint.y), &retval);
    return retval.getMessage().toBool();
}

bool QMozViewPrivate::HandleDoubleTap(const nsIntPoint& aPoint)
{
    QMozReturnValue retval;
    retval.setMessage(false);
    mViewIface->handleDoubleTap(QPoint(aPoint.x, aPoint.y), &retval);
    return retval.getMessage().toBool();
}

void QMozViewPrivate::touchEvent(QTouchEvent* event)
{
    // QInputMethod sends the QInputMethodEvent. Thus, it will
    // be handled before this touch event. Problem is that
    // this also commits preedited text when moving web content.
    // This should be committed just before moving cursor position to
    // the old cursor position.
    if (mPreedit) {
        QInputMethod* inputContext = qGuiApp->inputMethod();
        if (inputContext) {
            inputContext->commit();
        }
        mPreedit = false;
    }

    // Always accept the QTouchEvent so that we'll receive also TouchUpdate and TouchEnd events
    mPendingTouchEvent = true;
    event->setAccepted(true);
    bool draggingChanged = false;
    bool pinchingChanged = false;
    bool testFlick = true;
    int touchPointsCount = event->touchPoints().size();

    if (event->type() == QEvent::TouchBegin) {
        Q_ASSERT(touchPointsCount > 0);
        mViewIface->forceViewActiveFocus();
        if (touchPointsCount > 1 && !mPinching) {
            mPinching = true;
            pinchingChanged = true;
        }
        ResetState();
    } else if (event->type() == QEvent::TouchUpdate) {
        Q_ASSERT(touchPointsCount > 0);
        if (!mDragging) {
            mDragging = true;
            mDragStartY = mContentRect.y() * mContentResolution;
            mMoveDelta = 0;
            draggingChanged = true;
        }

        if (touchPointsCount > 1 && !mPinching) {
            mPinching = true;
            pinchingChanged = true;
        }
    } else if (event->type() == QEvent::TouchEnd) {
        Q_ASSERT(touchPointsCount > 0);
        HandleTouchEnd(draggingChanged, pinchingChanged);
    } else if (event->type() == QEvent::TouchCancel) {
        HandleTouchEnd(draggingChanged, pinchingChanged);
        testFlick = false;
        mCanFlick = false;
    }

    if (testFlick) {
        TestFlickingMode(event);
    }

    if (draggingChanged) {
        mViewIface->draggingChanged();
    }

    if (pinchingChanged) {
        mViewIface->pinchingChanged();
    }

    if (event->type() == QEvent::TouchEnd) {
        if (mCanFlick) {
            UpdateMoving(mCanFlick);
        } else {
            // From dragging (panning) end to clean state
            ResetState();
        }
    } else {
        UpdateMoving(mDragging);
    }

    qint64 timeStamp = current_timestamp(event);

    // Add active touch point to cancelled touch sequence.
    if (event->type() == QEvent::TouchCancel && touchPointsCount == 0) {
        QMapIterator<int, QPointF> i(mActiveTouchPoints);
        MultiTouchInput multiTouchInputEnd(MultiTouchInput::MULTITOUCH_END, timeStamp, TimeStamp(), 0);
        while (i.hasNext()) {
            i.next();
            QPointF pos = i.value();
            multiTouchInputEnd.mTouches.AppendElement(SingleTouchData(i.key(),
                                                             mozilla::ScreenIntPoint(pos.x(), pos.y()),
                                                             mozilla::ScreenSize(1, 1),
                                                             180.0f,
                                                             0));
        }
        // All touch point should be cleared but let's clear active touch points anyways.
        mActiveTouchPoints.clear();
        ReceiveInputEvent(multiTouchInputEnd);
        // touch was canceled hence no need to generate touchstart or touchmove
        return;
    }

    QList<int> pressedIds, moveIds, endIds;
    QHash<int,int> idHash;
    for (int i = 0; i < touchPointsCount; ++i) {
        const QTouchEvent::TouchPoint& pt = event->touchPoints().at(i);
        idHash.insert(pt.id(), i);
        switch (pt.state()) {
            case Qt::TouchPointPressed: {
                mActiveTouchPoints.insert(pt.id(), pt.pos());
                pressedIds.append(pt.id());
                break;
            }
            case Qt::TouchPointReleased: {
                mActiveTouchPoints.remove(pt.id());
                endIds.append(pt.id());
                break;
            }
            case Qt::TouchPointMoved:
            case Qt::TouchPointStationary: {
                mActiveTouchPoints.insert(pt.id(), pt.pos());
                moveIds.append(pt.id());
                break;
            }
            default:
                break;
        }
    }

    // We should append previous touches to start event in order
    // to make Gecko recognize it as new added touches to existing session
    // and not evict it here http://hg.mozilla.org/mozilla-central/annotate/1d9c510b3742/layout/base/nsPresShell.cpp#l6135
    QList<int> startIds(moveIds);

    // Produce separate event for every pressed touch points
    Q_FOREACH (int id, pressedIds) {
        MultiTouchInput multiTouchInputStart(MultiTouchInput::MULTITOUCH_START, timeStamp, TimeStamp(), 0);
        startIds.append(id);
        std::sort(startIds.begin(), startIds.end(), std::less<int>());
        Q_FOREACH (int startId, startIds) {
            const QTouchEvent::TouchPoint& pt = event->touchPoints().at(idHash.value(startId));
            mozilla::ScreenIntPoint nspt(pt.pos().x(), pt.pos().y());
            multiTouchInputStart.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                               nspt,
                                                               mozilla::ScreenSize(1, 1),
                                                               180.0f,
                                                               pt.pressure()));
        }

        ReceiveInputEvent(multiTouchInputStart);
    }

    Q_FOREACH (int id, endIds) {
        const QTouchEvent::TouchPoint& pt = event->touchPoints().at(idHash.value(id));
        mozilla::ScreenIntPoint nspt(pt.pos().x(), pt.pos().y());
        MultiTouchInput multiTouchInputEnd(MultiTouchInput::MULTITOUCH_END, timeStamp, TimeStamp(), 0);
        multiTouchInputEnd.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                         nspt,
                                                         mozilla::ScreenSize(1, 1),
                                                         180.0f,
                                                         pt.pressure()));
        ReceiveInputEvent(multiTouchInputEnd);
    }

    if (!moveIds.empty()) {
        if (!pressedIds.empty()) {
            moveIds.append(pressedIds);
        }

        // Sort touch lists by IDs just in case JS code identifies touches
        // by their order rather than their IDs.
        std::sort(moveIds.begin(), moveIds.end(), std::less<int>());
        MultiTouchInput multiTouchInputMove(MultiTouchInput::MULTITOUCH_MOVE, timeStamp, TimeStamp(), 0);
        Q_FOREACH (int id, moveIds) {
            const QTouchEvent::TouchPoint& pt = event->touchPoints().at(idHash.value(id));
            mozilla::ScreenIntPoint nspt(pt.pos().x(), pt.pos().y());
            multiTouchInputMove.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                              nspt,
                                                              mozilla::ScreenSize(1, 1),
                                                              180.0f,
                                                              pt.pressure()));
        }
        ReceiveInputEvent(multiTouchInputMove);
    }
}

void QMozViewPrivate::ReceiveInputEvent(const InputData& event)
{
    if (mViewInitialized) {
        mView->ReceiveInputEvent(event);
    }
}
