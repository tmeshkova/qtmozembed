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
#include "qmozembedlog.h"

using namespace mozilla;
using namespace mozilla::embedlite;

QGraphicsMozViewPrivate::QGraphicsMozViewPrivate(IMozQViewIface* aViewIface)
    : mViewIface(aViewIface)
    , mContext(NULL)
    , mView(NULL)
    , mViewInitialized(false)
    , mBgColor(Qt::white)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    , mTempTexture(NULL)
#endif
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
    mLocation = QString(aLocation);
    if (mCanGoBack != aCanGoBack || mCanGoForward != aCanGoForward) {
        mCanGoBack = aCanGoBack;
        mCanGoForward = aCanGoForward;
        mViewIface->navigationHistoryChanged();
    }
    mViewIface->urlChanged();
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
    if (mLocation != aLocation) {
        mLocation = aLocation;
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
    UpdateContentSize(aWidth, aHeight);
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
    }

    UpdateContentSize(aScrollableSize.width, aScrollableSize.height);
    return false;
}

bool QGraphicsMozViewPrivate::ScrollUpdate(const gfxPoint& aPosition, const float aResolution)
{
    mContentResolution = aResolution;
    if (mScrollableOffset.x() != aPosition.x * mContentResolution || mScrollableOffset.y() != aPosition.y * mContentResolution) {
        mScrollableOffset.setX(aPosition.x * mContentResolution);
        mScrollableOffset.setY(aPosition.y * mContentResolution);
        mViewIface->scrollableOffsetChanged();
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
    if (event->type() == QEvent::TouchBegin) {
        mViewIface->forceViewActiveFocus();
        mTouchTime.restart();
    } else if (event->type() == QEvent::TouchUpdate && !mDragging) {
        mDragging = true;
        draggingChanged = true;
    } else if (event->type() == QEvent::TouchEnd) {
        mDragging = false;
        draggingChanged = true;
    }

    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START, mTouchTime.elapsed());
    MultiTouchInput meventMove(MultiTouchInput::MULTITOUCH_MOVE, mTouchTime.elapsed());
    MultiTouchInput meventEnd(MultiTouchInput::MULTITOUCH_END, mTouchTime.elapsed());
    for (int i = 0; i < event->touchPoints().size(); ++i) {
        const QTouchEvent::TouchPoint& pt = event->touchPoints().at(i);
        mozilla::ScreenIntPoint nspt(pt.pos().x(), pt.pos().y());
        switch (pt.state()) {
            case Qt::TouchPointPressed: {
                meventStart.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                                   nspt,
                                                                   mozilla::ScreenSize(1, 1),
                                                                   180.0f,
                                                                   1.0f));
                break;
            }
            case Qt::TouchPointReleased: {
                meventEnd.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                                 nspt,
                                                                 mozilla::ScreenSize(1, 1),
                                                                 180.0f,
                                                                 1.0f));
                break;
            }
            case Qt::TouchPointMoved: {
                meventMove.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                                  nspt,
                                                                  mozilla::ScreenSize(1, 1),
                                                                  180.0f,
                                                                  1.0f));
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
        ReceiveInputEvent(meventMove);
    }
    if (meventEnd.mTouches.Length()) {
        ReceiveInputEvent(meventEnd);
    }

    if (draggingChanged) {
        mViewIface->draggingChanged();
    }
}

void QGraphicsMozViewPrivate::ReceiveInputEvent(const InputData& event)
{
    if (mViewInitialized) {
        mView->ReceiveInputEvent(event);
    }
}
