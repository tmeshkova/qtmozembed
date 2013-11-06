/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QGraphicsMozViewPrivate"

#include <QTouchEvent>
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QApplication>
#include <QInputContext>
#include <qjson/serializer.h>
#include <qjson/parser.h>
#else
#include <QJsonDocument>
#include <QGuiApplication>
#endif

#include "qgraphicsmozview_p.h"
#include "qmozcontext.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/gfx/Tools.h"
#include "qmozembedlog.h"
#include <sys/time.h>

#ifndef MOZVIEW_FLICK_THRESHOLD
#define MOZVIEW_FLICK_THRESHOLD 200
#endif

#define SCROLL_EPSILON 0.001

using namespace mozilla;
using namespace mozilla::embedlite;

qint64 current_timestamp(QTouchEvent* aEvent)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    if (aEvent) {
        return aEvent->timestamp();
    }
#endif
    struct timeval te;
    gettimeofday(&te, NULL);
    qint64 milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

QGraphicsMozViewPrivate::QGraphicsMozViewPrivate(IMozQViewIface* aViewIface)
    : mViewIface(aViewIface)
    , mContext(NULL)
    , mView(NULL)
    , mViewInitialized(false)
    , mBgColor(Qt::white)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    , mTempTexture(NULL)
#endif
    , mEnabled(true)
    , mChromeGestureEnabled(true)
    , mChromeGestureThreshold(0.0)
    , mChrome(true)
    , mMoveDelta(0.0)
    , mDragStartY(0)
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
    , mViewIsFocused(false)
    , mHasContext(false)
    , mGLSurfaceSize(0,0)
    , mPressed(false)
    , mDragging(false)
    , mFlicking(false)
{
}

QGraphicsMozViewPrivate::~QGraphicsMozViewPrivate()
{
    delete mViewIface;
}

void QGraphicsMozViewPrivate::CompositorCreated()
{
    mViewIface->createGeckoGLContext();
}

void QGraphicsMozViewPrivate::UpdateContentSize(unsigned int aWidth, unsigned int aHeight)
{
    // Get mContentResolution at startup via context's pixelRatio
    if (mContentResolution == 0.0) {
        mContentResolution = mContext->pixelRatio();
    }

    bool widthChanged = false;
    bool heightChanged = false;
    // Emit changes only after both values have been updated.
    if (mScrollableSize.width() != aWidth * mContentResolution) {
        mScrollableSize.setWidth(aWidth * mContentResolution);
        widthChanged = true;
    }

    if (mScrollableSize.height() != aHeight * mContentResolution) {
        mScrollableSize.setHeight(aHeight * mContentResolution);
        heightChanged = true;
    }

    if (widthChanged) {
        mViewIface->contentWidthChanged();
    }

    if (heightChanged) {
        mViewIface->contentHeightChanged();
    }
}

void QGraphicsMozViewPrivate::TestFlickingMode(QTouchEvent *event)
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

void QGraphicsMozViewPrivate::HandleTouchEnd(bool &draggingChanged, bool &pinchingChanged)
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

void QGraphicsMozViewPrivate::ResetState()
{
    // Invalid initial drag start Y.
    mDragStartY = -1;
    mMoveDelta = 0;

    mFlicking = false;
    if (mMoving) {
        mMoving = false;
        mViewIface->movingChanged();
    }
}

void QGraphicsMozViewPrivate::UpdateViewSize()
{
    if (!mViewInitialized) {
        return;
    }

    if (mContext->GetApp()->IsAccelerated() && mHasContext) {
        mView->SetGLViewPortSize(mGLSurfaceSize.width(), mGLSurfaceSize.height());
    }
    mView->SetViewSize(mSize.width(), mSize.height());
}

bool QGraphicsMozViewPrivate::RequestCurrentGLContext()
{
    QSize unused;
    return RequestCurrentGLContext(unused);
}

bool QGraphicsMozViewPrivate::RequestCurrentGLContext(QSize& aViewPortSize)
{
    bool hasContext = false;
    mViewIface->requestGLContext(hasContext, aViewPortSize);
    return hasContext;
}

void QGraphicsMozViewPrivate::ViewInitialized()
{
    mViewInitialized = true;
    UpdateViewSize();
    // This is currently part of official API, so let's subscribe to these messages by default
    mViewIface->viewInitialized();
    mViewIface->navigationHistoryChanged();
}

void QGraphicsMozViewPrivate::SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    mBgColor = QColor(r, g, b, a);
    mViewIface->bgColorChanged();
}

bool QGraphicsMozViewPrivate::Invalidate()
{
    mViewIface->Invalidate();
    return true;
}

void QGraphicsMozViewPrivate::OnLocationChanged(const char* aLocation, bool aCanGoBack, bool aCanGoForward)
{
    if (mCanGoBack != aCanGoBack || mCanGoForward != aCanGoForward) {
        mCanGoBack = aCanGoBack;
        mCanGoForward = aCanGoForward;
        mViewIface->navigationHistoryChanged();
    }

    if (mLocation != aLocation) {
        mLocation = QString(aLocation);
        mViewIface->urlChanged();
    }
}

void QGraphicsMozViewPrivate::OnLoadProgress(int32_t aProgress, int32_t aCurTotal, int32_t aMaxTotal)
{
    if (mIsLoading) {
        mProgress = aProgress;
        mViewIface->loadProgressChanged();
    }
}

void QGraphicsMozViewPrivate::OnLoadStarted(const char* aLocation)
{
    if (mIsPainted) {
        mIsPainted = false;
        mViewIface->firstPaint(-1, -1);
    }

    if (mLocation != aLocation) {
        mLocation = QString(aLocation);
        mViewIface->urlChanged();
    }
    if (!mIsLoading) {
        mIsLoading = true;
        mProgress = 1;
        mViewIface->loadingChanged();
    }
}

void QGraphicsMozViewPrivate::OnLoadFinished(void)
{
    if (mIsLoading) {
        mProgress = 100;
        mIsLoading = false;
        mViewIface->loadingChanged();
    }
}

// View finally destroyed and deleted
void QGraphicsMozViewPrivate::ViewDestroyed()
{
    LOGT();
    mView = NULL;
    mViewInitialized = false;
    mViewIface->viewDestroyed();
}

void QGraphicsMozViewPrivate::RecvAsyncMessage(const PRUnichar* aMessage, const PRUnichar* aData)
{
    NS_ConvertUTF16toUTF8 message(aMessage);
    NS_ConvertUTF16toUTF8 data(aData);

    bool ok = false;
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QJson::Parser parser;
    QVariant vdata = parser.parse(QByteArray(data.get()), &ok);
#else
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(data.get()), &error);
    ok = error.error == QJsonParseError::NoError;
    QVariant vdata = doc.toVariant();
#endif

    if (ok) {
        LOGT("mesg:%s, data:%s", message.get(), data.get());
        mViewIface->recvAsyncMessage(message.get(), vdata);
    } else {
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
        LOGT("parse: err:%s, errLine:%i", parser.errorString().toUtf8().data(), parser.errorLine());
#else
        LOGT("parse: err:%s, errLine:%i", error.errorString().toUtf8().data(), error.offset);
#endif
    }
}

char* QGraphicsMozViewPrivate::RecvSyncMessage(const PRUnichar* aMessage, const PRUnichar*  aData)
{
    QMozReturnValue response;
    NS_ConvertUTF16toUTF8 message(aMessage);
    NS_ConvertUTF16toUTF8 data(aData);

    bool ok = false;
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QJson::Parser parser;
    QVariant vdata = parser.parse(QByteArray(data.get()), &ok);
#else
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(data.get()), &error);
    ok = error.error == QJsonParseError::NoError;
    QVariant vdata = doc.toVariant();
#endif
    mViewIface->recvSyncMessage(message.get(), vdata, &response);

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QJson::Serializer serializer;
    QByteArray array = serializer.serialize(response.getMessage());
#else
    QJsonDocument respdoc = QJsonDocument::fromVariant(response.getMessage());
    QByteArray array = respdoc.toJson();
#endif
    LOGT("msg:%s, response:%s", message.get(), array.constData());
    return strdup(array.constData());
}

void QGraphicsMozViewPrivate::OnLoadRedirect(void)
{
    LOGT();
    mViewIface->loadRedirect();
}

void QGraphicsMozViewPrivate::OnSecurityChanged(const char* aStatus, unsigned int aState)
{
    LOGT();
    mViewIface->securityChanged(aStatus, aState);
}
void QGraphicsMozViewPrivate::OnFirstPaint(int32_t aX, int32_t aY)
{
    LOGT();
    mIsPainted = true;
    mViewIface->firstPaint(aX, aY);
}

void QGraphicsMozViewPrivate::SetIsFocused(bool aIsFocused)
{
    mViewIsFocused = aIsFocused;
    if (mViewInitialized) {
        mView->SetIsFocused(aIsFocused);
    }
}

void QGraphicsMozViewPrivate::IMENotification(int aIstate, bool aOpen, int aCause, int aFocusChange,
                                              const PRUnichar* inputType, const PRUnichar* inputMode)
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
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
            QWidget* focusWidget = qApp->focusWidget();
            if (focusWidget && aFocusChange) {
                QInputContext* inputContext = qApp->inputContext();
                if (!inputContext) {
                    LOGT("Requesting SIP: but no input context");
                    return;
                }
                if (aIstate) {
                    QEvent request(QEvent::RequestSoftwareInputPanel);
                    inputContext->filterEvent(&request);
                    focusWidget->setAttribute(Qt::WA_InputMethodEnabled, true);
                    inputContext->setFocusWidget(focusWidget);
                } else {
                    QEvent request(QEvent::CloseSoftwareInputPanel);
                    inputContext->filterEvent(&request);
                    inputContext->reset();
                }
            }
#else
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
#endif
        }

    }
    mViewIface->imeNotification(aIstate, aOpen, aCause, aFocusChange, imType);
}

void QGraphicsMozViewPrivate::GetIMEStatus(int32_t* aIMEEnabled, int32_t* aIMEOpen, intptr_t* aNativeIMEContext)
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    *aNativeIMEContext = (intptr_t)qApp->inputContext();
#else
    *aNativeIMEContext = (intptr_t)qApp->inputMethod();
#endif
}

void QGraphicsMozViewPrivate::OnScrolledAreaChanged(unsigned int aWidth, unsigned int aHeight)
{
    Q_UNUSED(aWidth)
    Q_UNUSED(aHeight)
}

void QGraphicsMozViewPrivate::OnScrollChanged(int32_t offSetX, int32_t offSetY)
{
}

void QGraphicsMozViewPrivate::OnTitleChanged(const PRUnichar* aTitle)
{
    mTitle = QString((QChar*)aTitle);
    mViewIface->titleChanged();
}

void QGraphicsMozViewPrivate::SetFirstPaintViewport(const nsIntPoint& aOffset, float aZoom,
                                                    const nsIntRect& aPageRect, const gfxRect& aCssPageRect)
{
    LOGT();
}

void QGraphicsMozViewPrivate::SyncViewportInfo(const nsIntRect& aDisplayPort,
                                               float aDisplayResolution, bool aLayersUpdated,
                                               nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY)
{
    LOGT();
}

void QGraphicsMozViewPrivate::SetPageRect(const gfxRect& aCssPageRect)
{
    LOGT();
}

bool QGraphicsMozViewPrivate::SendAsyncScrollDOMEvent(const gfxRect& aContentRect, const gfxSize& aScrollableSize)
{
    // Get mContentResolution at startup via context's pixelRatio
    if (mContentResolution == 0.0) {
        mContentResolution = mContext->pixelRatio();
    }

    if (mContentRect.x() != aContentRect.x * mContentResolution || mContentRect.y() != aContentRect.y * mContentResolution ||
            mContentRect.width() != aContentRect.width * mContentResolution ||
            mContentRect.height() != aContentRect.height * mContentResolution) {
        mContentRect.setRect(aContentRect.x * mContentResolution, aContentRect.y * mContentResolution,
                             aContentRect.width * mContentResolution, aContentRect.height * mContentResolution);
        mViewIface->viewAreaChanged();
        // chrome, chromeGestureEnabled, and chromeGestureThreshold can be used
        // to control chrome/chromeless mode.
        // When chromeGestureEnabled is false, no actions are taken
        // When chromeGestureThreshold is true, chrome is set false when chromeGestrureThreshold is exceeded (pan/flick)
        // and set to true when flicking/panning the same amount to the the opposite direction.
        // This do not have relationship to HTML5 fullscreen API.
        if (mEnabled && mChromeGestureEnabled && mDragStartY >= 0) {
            qreal offset = mScrollableOffset.y();
            qreal currentDelta = offset - mDragStartY;

            if (qAbs(currentDelta) < mMoveDelta) {
                mDragStartY = offset;
            }

            if (currentDelta > mChromeGestureThreshold) {
                if (mChrome) {
                    mChrome = false;
                    mViewIface->chromeChanged();
                }
            } else if (currentDelta < -mChromeGestureThreshold) {
                if (!mChrome) {
                    mChrome = true;
                    mViewIface->chromeChanged();
                }
            }
            mMoveDelta = qAbs(currentDelta);
        }
    }

    UpdateContentSize(aScrollableSize.width, aScrollableSize.height);
    return false;
}

bool QGraphicsMozViewPrivate::ScrollUpdate(const gfxPoint& aPosition, const float aResolution)
{
    mContentResolution = aResolution;
    float posX(aPosition.x * mContentResolution);
    float posY(aPosition.y * mContentResolution);

    if (!gfx::FuzzyEqual(mScrollableOffset.x(), posX, SCROLL_EPSILON) ||
        !gfx::FuzzyEqual(mScrollableOffset.y(), posY, SCROLL_EPSILON)) {

        mScrollableOffset.setX(posX);
        mScrollableOffset.setY(posY);
        mViewIface->scrollableOffsetChanged();

        if (mEnabled) {
            // Update vertical scroll decorator
            qreal ySizeRatio = mContentRect.height() / mScrollableSize.height();
            qreal tmpValue = mSize.height() * ySizeRatio;
            mVerticalScrollDecorator.setHeight(tmpValue);
            tmpValue = mScrollableOffset.y() * ySizeRatio;
            mVerticalScrollDecorator.setY(tmpValue);

            // Update horizontal scroll decorator
            qreal xSizeRatio = mContentRect.width() / mScrollableSize.width();
            tmpValue = mSize.width() * xSizeRatio;
            mHorizontalScrollDecorator.setWidth(tmpValue);
            tmpValue = mScrollableOffset.x() * xSizeRatio;
            mHorizontalScrollDecorator.setX(tmpValue);
        }
    }

    return false;
}

bool QGraphicsMozViewPrivate::HandleLongTap(const nsIntPoint& aPoint)
{
    QMozReturnValue retval;
    retval.setMessage(false);
    mViewIface->handleLongTap(QPoint(aPoint.x, aPoint.y), &retval);
    return retval.getMessage().toBool();
}

bool QGraphicsMozViewPrivate::HandleSingleTap(const nsIntPoint& aPoint)
{
    QMozReturnValue retval;
    retval.setMessage(false);
    mViewIface->handleSingleTap(QPoint(aPoint.x, aPoint.y), &retval);
    return retval.getMessage().toBool();
}

bool QGraphicsMozViewPrivate::HandleDoubleTap(const nsIntPoint& aPoint)
{
    QMozReturnValue retval;
    retval.setMessage(false);
    mViewIface->handleDoubleTap(QPoint(aPoint.x, aPoint.y), &retval);
    return retval.getMessage().toBool();
}

void QGraphicsMozViewPrivate::touchEvent(QTouchEvent* event)
{
    // Always accept the QTouchEvent so that we'll receive also TouchUpdate and TouchEnd events
    mPendingTouchEvent = true;
    event->setAccepted(true);
    bool draggingChanged = false;
    bool pinchingChanged = false;
    bool testFlick = true;
    int touchPointsCount = event->touchPoints().size();

    if (event->type() == QEvent::TouchBegin) {
        mViewIface->forceViewActiveFocus();
        if (touchPointsCount > 1 && !mPinching) {
            mPinching = true;
            pinchingChanged = true;
        }
        ResetState();
    } else if (event->type() == QEvent::TouchUpdate) {
        if (!mDragging) {
            mDragging = true;
            mDragStartY = mScrollableOffset.y();
            mMoveDelta = 0;
            draggingChanged = true;
        }

        if (touchPointsCount > 1 && !mPinching) {
            mPinching = true;
            pinchingChanged = true;
        }
    } else if (event->type() == QEvent::TouchEnd) {
        HandleTouchEnd(draggingChanged, pinchingChanged);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    } else if (event->type() == QEvent::TouchCancel) {
        HandleTouchEnd(draggingChanged, pinchingChanged);
        testFlick = false;
        mCanFlick = false;
#endif
    }

    if (testFlick) {
        TestFlickingMode(event);
    }

    qint64 timeStamp = current_timestamp(event);
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START, timeStamp);
    MultiTouchInput meventMove(MultiTouchInput::MULTITOUCH_MOVE, timeStamp);
    MultiTouchInput meventEnd(mCanFlick ? MultiTouchInput::MULTITOUCH_END :
                              MultiTouchInput::MULTITOUCH_CANCEL, timeStamp);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    // Add active touch point to cancelled touch sequence.
    if (event->type() == QEvent::TouchCancel && touchPointsCount == 0) {
        QMapIterator<int, QPointF> i(mActiveTouchPoints);
        while (i.hasNext()) {
            i.next();
            QPointF pos = i.value();
            meventEnd.mTouches.AppendElement(SingleTouchData(i.key(),
                                                             mozilla::ScreenIntPoint(pos.x(), pos.y()),
                                                             mozilla::ScreenSize(1, 1),
                                                             180.0f,
                                                             0));
        }
        // All touch point should be cleared but let's clear active touch points anyways.
        mActiveTouchPoints.clear();
    }
#endif

    for (int i = 0; i < touchPointsCount; ++i) {
        const QTouchEvent::TouchPoint& pt = event->touchPoints().at(i);
        mozilla::ScreenIntPoint nspt(pt.pos().x(), pt.pos().y());
        switch (pt.state()) {
            case Qt::TouchPointPressed: {
                mActiveTouchPoints.insert(pt.id(), pt.pos());
                meventStart.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                                   nspt,
                                                                   mozilla::ScreenSize(1, 1),
                                                                   180.0f,
                                                                   pt.pressure()));
                break;
            }
            case Qt::TouchPointReleased: {
                mActiveTouchPoints.remove(pt.id());
                meventEnd.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                                 nspt,
                                                                 mozilla::ScreenSize(1, 1),
                                                                 180.0f,
                                                                 pt.pressure()));
                break;
            }
            case Qt::TouchPointMoved:
            case Qt::TouchPointStationary: {
                mActiveTouchPoints.insert(pt.id(), pt.pos());
                meventMove.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                                  nspt,
                                                                  mozilla::ScreenSize(1, 1),
                                                                  180.0f,
                                                                  pt.pressure()));
                break;
            }
            default:
                break;
        }
    }

    if (meventStart.mTouches.Length()) {
        // We should append previous touches to start event in order
        // to make Gecko recognize it as new added touches to existing session
        // and not evict it here http://hg.mozilla.org/mozilla-central/annotate/1d9c510b3742/layout/base/nsPresShell.cpp#l6135
        if (meventMove.mTouches.Length()) {
            meventStart.mTouches.AppendElements(meventMove.mTouches);
        }
        ReceiveInputEvent(meventStart);
    }
    if (meventMove.mTouches.Length()) {
        if (meventStart.mTouches.Length()) {
            meventMove.mTouches.AppendElements(meventStart.mTouches);
        }
        ReceiveInputEvent(meventMove);
    }
    if (meventEnd.mTouches.Length()) {
        ReceiveInputEvent(meventEnd);
    }

    if (draggingChanged) {
        mViewIface->draggingChanged();
    }

    if (pinchingChanged) {
        mViewIface->pinchingChanged();
    }

    if (mMoving != (mDragging || mCanFlick)) {
        mMoving = (mDragging || mCanFlick);
        mViewIface->movingChanged();
    }

     if (event->type() == QEvent::TouchEnd && mCanFlick) {
        mViewIface->startMoveMonitoring();
     }
}

void QGraphicsMozViewPrivate::ReceiveInputEvent(const InputData& event)
{
    if (mViewInitialized) {
        mView->ReceiveInputEvent(event);
    }
}
