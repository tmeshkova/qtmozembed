/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QGraphicsMozViewPrivate"

#include <QTouchEvent>
#include <QGLContext>
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QInputContext>
#include <qjson/serializer.h>
#include <qjson/parser.h>
#else
#include <QJsonDocument>
#endif
#include <QApplication>

#include "qgraphicsmozview_p.h"
#include "qgraphicsmozview.h"
#include "qmozcontext.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLog.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

using namespace mozilla;
using namespace mozilla::embedlite;

QGraphicsMozViewPrivate::QGraphicsMozViewPrivate(QGraphicsMozView* view)
    : q(view)
    , mContext(NULL)
    , mView(NULL)
    , mViewInitialized(false)
    , mBgColor(Qt::white)
    , mPendingTouchEvent(false)
    , mProgress(100)
    , mCanGoBack(false)
    , mCanGoForward(false)
    , mIsLoading(false)
    , mLastIsGoodRotation(true)
    , mIsPasswordField(false)
    , mGraphicsViewAssigned(false)
    , mContentRect(0,0,0,0)
    , mScrollableSize(0,0)
    , mScrollableOffset(0,0)
    , mContentResolution(1.0)
    , mIsPainted(false)
{
}

QGraphicsMozViewPrivate::~QGraphicsMozViewPrivate()
{
}

QGraphicsView* QGraphicsMozViewPrivate::GetViewWidget()
{
    if (!q->scene()) {
        return nullptr;
    }

    NS_ASSERTION(q->scene()->views().size() == 1, "Not exactly one view for our scene!");
    return q->scene()->views()[0];
}

void QGraphicsMozViewPrivate::UpdateViewSize()
{
    if (!mViewInitialized) {
        return;
    }

    if (mContext->GetApp()->IsAccelerated()) {
        const QGLContext* ctx = QGLContext::currentContext();
        if (ctx && ctx->device()) {
            QRectF r(0, 0, ctx->device()->width(), ctx->device()->height());
            r = q->mapRectToScene(r);
            mView->SetGLViewPortSize(r.width(), r.height());
        }
    }
    mView->SetViewSize(mSize.width(), mSize.height());
}

bool QGraphicsMozViewPrivate::RequestCurrentGLContext()
{
    QGraphicsView* view = GetViewWidget();
    if (!view) {
        return false;
    }

    QGLWidget* qglwidget = qobject_cast<QGLWidget*>(view->viewport());
    if (qglwidget) {
        qglwidget->makeCurrent();
        QGLContext* context = const_cast<QGLContext*>(QGLContext::currentContext());
        if (context) {
            return true;
        }
    }
    return false;
}

void QGraphicsMozViewPrivate::ViewInitialized()
{
    mViewInitialized = true;
    UpdateViewSize();
    // This is currently part of official API, so let's subscribe to these messages by default
    Q_EMIT q->viewInitialized();
    Q_EMIT q->navigationHistoryChanged();
}

void QGraphicsMozViewPrivate::SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    mBgColor = QColor(r, g, b, a);
}

bool QGraphicsMozViewPrivate::Invalidate()
{
    q->update();
    return true;
}

void QGraphicsMozViewPrivate::OnLocationChanged(const char* aLocation, bool aCanGoBack, bool aCanGoForward)
{
    mLocation = QString(aLocation);
    if (mCanGoBack != aCanGoBack || mCanGoForward != aCanGoForward) {
        mCanGoBack = aCanGoBack;
        mCanGoForward = aCanGoForward;
        Q_EMIT q->navigationHistoryChanged();
    }
    Q_EMIT q->urlChanged();
}

void QGraphicsMozViewPrivate::OnLoadProgress(int32_t aProgress, int32_t aCurTotal, int32_t aMaxTotal)
{
    mProgress = aProgress;
    Q_EMIT q->loadProgressChanged();
}

void QGraphicsMozViewPrivate::OnLoadStarted(const char* aLocation)
{
    if (mLocation != aLocation) {
        mLocation = aLocation;
        Q_EMIT q->urlChanged();
    }
    if (!mIsLoading) {
        mIsLoading = true;
        mProgress = 1;
        Q_EMIT q->loadingChanged();
    }
}

void QGraphicsMozViewPrivate::OnLoadFinished(void)
{
    if (mIsLoading) {
        mProgress = 100;
        mIsLoading = false;
        Q_EMIT q->loadingChanged();
    }
}

// View finally destroyed and deleted
void QGraphicsMozViewPrivate::ViewDestroyed()
{
    LOGT();
    mView = NULL;
    mViewInitialized = false;
    Q_EMIT q->viewDestroyed();
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
        Q_EMIT q->recvAsyncMessage(message.get(), vdata);
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
    QSyncMessageResponse response;
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
    Q_EMIT q->recvSyncMessage(message.get(), vdata, &response);

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
    Q_EMIT q->loadRedirect();
}

void QGraphicsMozViewPrivate::OnSecurityChanged(const char* aStatus, unsigned int aState)
{
    LOGT();
    Q_EMIT q->securityChanged(aStatus, aState);
}
void QGraphicsMozViewPrivate::OnFirstPaint(int32_t aX, int32_t aY)
{
    LOGT();
    mIsPainted = true;
    Q_EMIT q->firstPaint(aX, aY);
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
    q->setInputMethodHints(hints);

    QWidget* focusWidget = qApp->focusWidget();
    if (focusWidget && aFocusChange) {
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
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
#else
        LOGT("Fixme IME for Qt5");
#endif
    }
    Q_EMIT q->imeNotification(aIstate, aOpen, aCause, aFocusChange, imType);
}

void QGraphicsMozViewPrivate::OnScrolledAreaChanged(unsigned int aWidth, unsigned int aHeight)
{
    LOGT();
}

void QGraphicsMozViewPrivate::OnScrollChanged(int32_t offSetX, int32_t offSetY)
{
}

void QGraphicsMozViewPrivate::OnTitleChanged(const PRUnichar* aTitle)
{
    mTitle = QString((QChar*)aTitle);
    Q_EMIT q->titleChanged();
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
    mContentRect = QRect(aContentRect.x, aContentRect.y, aContentRect.width, aContentRect.height);
    mScrollableSize = QSize(aScrollableSize.width, aScrollableSize.height);
    Q_EMIT q->viewAreaChanged();
    return false;
}

bool QGraphicsMozViewPrivate::ScrollUpdate(const gfxPoint& aPosition, const float aResolution)
{
    mScrollableOffset = QPointF(aPosition.x, aPosition.y);
    mContentResolution = aResolution;
    Q_EMIT q->viewAreaChanged();
    return false;
}

bool QGraphicsMozViewPrivate::HandleLongTap(const nsIntPoint& aPoint)
{
    Q_EMIT q->handleLongTap(QPoint(aPoint.x, aPoint.y));
    return false;
}

bool QGraphicsMozViewPrivate::HandleSingleTap(const nsIntPoint& aPoint)
{
    Q_EMIT q->handleSingleTap(QPoint(aPoint.x, aPoint.y));
    return false;
}

bool QGraphicsMozViewPrivate::HandleDoubleTap(const nsIntPoint& aPoint)
{
    Q_EMIT q->handleDoubleTap(QPoint(aPoint.x, aPoint.y));
    return false;
}

void QGraphicsMozViewPrivate::touchEvent(QTouchEvent* event)
{
    // Always accept the QTouchEvent so that we'll receive also TouchUpdate and TouchEnd events
    mPendingTouchEvent = true;
    event->setAccepted(true);
    if (event->type() == QEvent::TouchBegin) {
        q->forceActiveFocus();
        mTouchTime.restart();
    }

    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START, mTouchTime.elapsed());
    MultiTouchInput meventMove(MultiTouchInput::MULTITOUCH_MOVE, mTouchTime.elapsed());
    MultiTouchInput meventEnd(MultiTouchInput::MULTITOUCH_END, mTouchTime.elapsed());
    for (int i = 0; i < event->touchPoints().size(); ++i) {
        const QTouchEvent::TouchPoint& pt = event->touchPoints().at(i);
        nsIntPoint nspt(pt.pos().x(), pt.pos().y());
        switch (pt.state()) {
            case Qt::TouchPointPressed: {
                meventStart.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                                   nspt,
                                                                   nsIntPoint(1, 1),
                                                                   180.0f,
                                                                   1.0f));
                break;
            }
            case Qt::TouchPointReleased: {
                meventEnd.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                                 nspt,
                                                                 nsIntPoint(1, 1),
                                                                 180.0f,
                                                                 1.0f));
                break;
            }
            case Qt::TouchPointMoved: {
                meventMove.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                                  nspt,
                                                                  nsIntPoint(1, 1),
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
}

void QGraphicsMozViewPrivate::ReceiveInputEvent(const InputData& event)
{
    if (mViewInitialized) {
        mView->ReceiveInputEvent(event);
    }
}
