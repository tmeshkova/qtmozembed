/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QGraphicsMozView"

#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QTime>
#include <QtOpenGL/QGLContext>
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QInputContext>
#else
#include <QJsonDocument>
#endif
#include <QApplication>
#include <QVariantMap>
#include "EmbedQtKeyUtils.h"

#include "mozilla-config.h"
#include "qgraphicsmozview.h"
#include "qmozcontext.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLog.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

#if (QT_VERSION <= QT_VERSION_CHECK(5, 0, 0))
#include <qjson/serializer.h>
#include <qjson/parser.h>
#else
#include <QJsonParseError>
#include <QJsonDocument>
#endif

using namespace mozilla;
using namespace mozilla::embedlite;

class QGraphicsMozViewPrivate : public EmbedLiteViewListener {
public:
    QGraphicsMozViewPrivate(QGraphicsMozView* view)
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
    {
    }
    virtual ~QGraphicsMozViewPrivate() {}

    QGraphicsView* GetViewWidget()
    {
        if (!q->scene())
            return nullptr;

        NS_ASSERTION(q->scene()->views().size() == 1, "Not exactly one view for our scene!");
        return q->scene()->views()[0];
    }

    void ReceiveInputEvent(const mozilla::InputData& event);
    void touchEvent(QTouchEvent* event);
    void UpdateViewSize()
    {
        if (mViewInitialized) {
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
    }
    virtual bool RequestCurrentGLContext()
    {
        QGraphicsView* view = GetViewWidget();
        if (view) {
            QGLWidget* qglwidget = qobject_cast<QGLWidget*>(view->viewport());
            if (qglwidget) {
                qglwidget->makeCurrent();
                QGLContext* context = const_cast<QGLContext*>(QGLContext::currentContext());
                if (context)
                    return true;
            }
        }
        return false;
    }
    virtual void ViewInitialized() {
        mViewInitialized = true;
        UpdateViewSize();
        Q_EMIT q->viewInitialized();
        Q_EMIT q->navigationHistoryChanged();
    }
    virtual void SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        mBgColor = QColor(r, g, b, a);
    }
    virtual bool Invalidate() {
        q->update();
        return true;
    }
    virtual void OnLocationChanged(const char* aLocation, bool aCanGoBack, bool aCanGoForward) {
        mLocation = QString(aLocation);
        if (mCanGoBack != aCanGoBack || mCanGoForward != aCanGoForward) {
            mCanGoBack = aCanGoBack;
            mCanGoForward = aCanGoForward;
            Q_EMIT q->navigationHistoryChanged();
        }
        Q_EMIT q->urlChanged();
    }
    virtual void OnLoadProgress(int32_t aProgress, int32_t aCurTotal, int32_t aMaxTotal) {
        mProgress = aProgress;
        Q_EMIT q->loadProgressChanged();
    }
    virtual void OnLoadStarted(const char* aLocation) {
        if (mLocation != aLocation) {
            mLocation = aLocation;
            Q_EMIT q->urlChanged();
        }
        if (!mIsLoading) {
            mIsLoading = true;
            Q_EMIT q->loadingChanged();
        }
    }
    virtual void OnLoadFinished(void) {
        if (mIsLoading) {
            mIsLoading = false;
            Q_EMIT q->loadingChanged();
        }
    }

    // View finally destroyed and deleted
    virtual void ViewDestroyed() {
        LOGT();
        mView = NULL;
        mViewInitialized = false;
        Q_EMIT q->viewDestroyed();
    }
    virtual void RecvAsyncMessage(const PRUnichar* aMessage, const PRUnichar* aData) {
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

        if (!strncmp(message.get(), "embed:", 6) || !strncmp(message.get(), "chrome:", 7)) {
            if (ok) {
                if (!strcmp(message.get(), "embed:alert")) {
                    Q_EMIT q->alert(vdata);
                    return;
                } else if (!strcmp(message.get(), "embed:confirm")) {
                    Q_EMIT q->confirm(vdata);
                    return;
                } else if (!strcmp(message.get(), "embed:prompt")) {
                    Q_EMIT q->prompt(vdata);
                    return;
                } else if (!strcmp(message.get(), "embed:auth")) {
                    Q_EMIT q->authRequired(vdata);
                    return;
                } else if (!strcmp(message.get(), "chrome:title")) {
                    QMap<QString, QVariant> map = vdata.toMap();
                    mTitle = map["title"].toString();
                    Q_EMIT q->titleChanged();
                    return;
                }
            } else {
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
                LOGT("parse: err:%s, errLine:%i", parser.errorString().toUtf8().data(), parser.errorLine());
#else
                LOGT("parse: err:%s, errLine:%i", error.errorString().toUtf8().data(), error.offset);
#endif
            }
        }
        LOGT("mesg:%s, data:%s", message.get(), data.get());
        Q_EMIT q->recvAsyncMessage(message.get(), vdata);
    }
    virtual char* RecvSyncMessage(const PRUnichar* aMessage, const PRUnichar*  aData) {
        QSyncMessageResponse response;
        NS_ConvertUTF16toUTF8 message(aMessage);
        NS_ConvertUTF16toUTF8 data(aData);
        Q_EMIT q->recvSyncMessage(message.get(), data.get(), &response);

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
        QJson::Serializer serializer;
        QByteArray array = serializer.serialize(response.getMessage());
#else
        QJsonDocument doc = QJsonDocument::fromVariant(response.getMessage());
        QByteArray array = doc.toJson();
#endif
        LOGT("msg:%s, response:%s", message.get(), array.constData());
        return strdup(array.constData());
    }

    virtual void OnLoadRedirect(void) {
        LOGT();
        Q_EMIT q->loadRedirect();
    }

    virtual void OnSecurityChanged(const char* aStatus, unsigned int aState) {
        LOGT();
        Q_EMIT q->securityChanged(aStatus, aState);
    }
    virtual void OnFirstPaint(int32_t aX, int32_t aY) {
        LOGT();
        Q_EMIT q->firstPaint(aX, aY);
    }

    virtual void IMENotification(int aIstate, bool aOpen, int aCause, int aFocusChange)
    {
        LOGT("imeState:%i", aIstate);
        q->setInputMethodHints(aIstate == 2 ? Qt::ImhHiddenText : Qt::ImhPreferLowercase);
        QWidget* focusWidget = qApp->focusWidget();
        if (focusWidget && aFocusChange) {
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
            QInputContext *inputContext = qApp->inputContext();
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
    }

    virtual void OnScrolledAreaChanged(unsigned int aWidth, unsigned int aHeight) { LOGT(); }
    virtual void OnScrollChanged(int32_t offSetX, int32_t offSetY) { }
    virtual void SetFirstPaintViewport(const nsIntPoint& aOffset, float aZoom,
                                       const nsIntRect& aPageRect, const gfxRect& aCssPageRect) { LOGT(); }
    virtual void SyncViewportInfo(const nsIntRect& aDisplayPort,
                                  float aDisplayResolution, bool aLayersUpdated,
                                  nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY) { LOGT(); }
    virtual void SetPageRect(const gfxRect& aCssPageRect) { LOGT(); }
    virtual bool SendAsyncScrollDOMEvent(const gfxRect& aContentRect, const gfxSize& aScrollableSize) {
        mContentRect = QRect(aContentRect.x, aContentRect.y, aContentRect.width, aContentRect.height);
        mScrollableSize = QSize(aScrollableSize.width, aScrollableSize.height);
        Q_EMIT q->viewAreaChanged();
        return false;
    }
    virtual bool ScrollUpdate(const gfxPoint& aPosition, const float aResolution)
    {
        mScrollableOffset = QPointF(aPosition.x, aPosition.y);
        mContentResolution = aResolution;
        Q_EMIT q->viewAreaChanged();
        return false;
    }
    virtual bool HandleLongTap(const nsIntPoint& aPoint) {
        Q_EMIT q->handleLongTap(QPoint(aPoint.x, aPoint.y));
        return false;
    }
    virtual bool HandleSingleTap(const nsIntPoint& aPoint) {
        Q_EMIT q->handleSingleTap(QPoint(aPoint.x, aPoint.y));
        return false;
    }
    virtual bool HandleDoubleTap(const nsIntPoint& aPoint) {
        Q_EMIT q->handleDoubleTap(QPoint(aPoint.x, aPoint.y));
        return false;
    }

    QGraphicsMozView* q;
    QMozContext* mContext;
    EmbedLiteView* mView;
    bool mViewInitialized;
    QColor mBgColor;
    QImage mTempBufferImage;
    QSize mSize;
    QTime mTouchTime;
    bool mPendingTouchEvent;
    QTime mPanningTime;
    QString mLocation;
    QString mTitle;
    int mProgress;
    bool mCanGoBack;
    bool mCanGoForward;
    bool mIsLoading;
    bool mLastIsGoodRotation;
    bool mIsPasswordField;
    bool mGraphicsViewAssigned;
    QRect mContentRect;
    QSize mScrollableSize;
    QPointF mScrollableOffset;
    float mContentResolution;
};

QGraphicsMozView::QGraphicsMozView(QGraphicsItem* parent)
    : QGraphicsWidget(parent)
    , d(new QGraphicsMozViewPrivate(this))
    , mParentID(0)
{
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
    setAcceptDrops(true);
    setAcceptTouchEvents(true);
    setFocusPolicy(Qt::StrongFocus);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);

    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setFlag(QGraphicsItem::ItemIsFocusScope, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setInputMethodHints(Qt::ImhPreferLowercase);

    d->mContext = QMozContext::GetInstance();
    if (!d->mContext->initialized()) {
        connect(d->mContext, SIGNAL(onInitialized()), this, SLOT(onInitialized()));
    } else {
        QTimer::singleShot(0, this, SLOT(onInitialized()));
    }
}

void
QGraphicsMozView::setParentID(unsigned aParentID)
{
    LOGT("mParentID:%u", aParentID);
    mParentID = aParentID;
    if (mParentID) {
        onInitialized();
    }
}

QGraphicsMozView::~QGraphicsMozView()
{
    delete d;
}

void
QGraphicsMozView::onInitialized()
{
    LOGT("mParentID:%u", mParentID);
    if (!d->mView) {
        d->mView = d->mContext->GetApp()->CreateView(mParentID);
        d->mView->SetListener(d);
    }
}

quint32
QGraphicsMozView::uniqueID() const
{
    return d->mView ? d->mView->GetUniqueID() : 0;
}

void QGraphicsMozView::EraseBackgroundGL(QPainter* painter, const QRect& r)
{
#ifdef GL_PROVIDER_EGL
    glEnable(GL_SCISSOR_TEST);
    glScissor(r.x(), r.y(), r.width(), r.height());
    glClearColor((GLfloat)d->mBgColor.red() / 255.0,
                 (GLfloat)d->mBgColor.green() / 255.0,
                 (GLfloat)d->mBgColor.blue() / 255.0,
                 (GLfloat)d->mBgColor.alpha() / 255.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
#else
    painter->fillRect(r, d->mBgColor);
#endif
}

void
QGraphicsMozView::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget*)
{
    if (!d->mGraphicsViewAssigned) {
        d->mGraphicsViewAssigned = true;
        QGraphicsView* view = d->GetViewWidget();
        if (view) {
            connect(view, SIGNAL(displayEntered()), this, SLOT(onDisplayEntered()));
            connect(view, SIGNAL(displayExited()), this, SLOT(onDisplayExited()));
        }
    }

    QRect r = opt ? opt->exposedRect.toRect() : boundingRect().toRect();
    if (d->mViewInitialized) {
        QMatrix affine = painter->transform().toAffine();
        gfxMatrix matr(affine.m11(), affine.m12(), affine.m21(), affine.m22(), affine.dx(), affine.dy());
        bool changedState = d->mLastIsGoodRotation != matr.PreservesAxisAlignedRectangles();
        d->mLastIsGoodRotation = matr.PreservesAxisAlignedRectangles();
        if (d->mContext->GetApp()->IsAccelerated()) {
            d->mView->SetGLViewTransform(matr);
            d->mView->SetViewClipping(0, 0, d->mSize.width(), d->mSize.height());
            if (changedState) {
                d->UpdateViewSize();
            }
            if (d->mLastIsGoodRotation) {
                // FIXME need to find proper rect using proper transform chain
                QRect eraseRect = painter->transform().isRotating() ? affine.mapRect(r) : r;
                painter->beginNativePainting();
                EraseBackgroundGL(painter, eraseRect);
                bool retval = d->mView->RenderGL();
                painter->endNativePainting();
                if (!retval) {
                    EraseBackgroundGL(painter, eraseRect);
                }
            }
        } else {
            if (d->mTempBufferImage.isNull() || d->mTempBufferImage.width() != r.width() || d->mTempBufferImage.height() != r.height()) {
                d->mTempBufferImage = QImage(r.size(), QImage::Format_RGB16);
            }
            {
                QPainter imgPainter(&d->mTempBufferImage);
                imgPainter.fillRect(r, d->mBgColor);
            }
            d->mView->RenderToImage(d->mTempBufferImage.bits(), d->mTempBufferImage.width(),
                                    d->mTempBufferImage.height(), d->mTempBufferImage.bytesPerLine(),
                                    d->mTempBufferImage.depth());
            painter->drawImage(QPoint(0, 0), d->mTempBufferImage);
        }
    } else {
        painter->fillRect(r, Qt::white);
    }
}

/*! \reimp
*/
QSizeF QGraphicsMozView::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
{
    if (which == Qt::PreferredSize)
        return QSizeF(800, 600); // ###
    return QGraphicsWidget::sizeHint(which, constraint);
}

/*! \reimp
*/
void QGraphicsMozView::setGeometry(const QRectF& rect)
{
    QGraphicsWidget::setGeometry(rect);

    // NOTE: call geometry() as setGeometry ensures that
    // the geometry is within legal bounds (minimumSize, maximumSize)
    d->mSize = geometry().size().toSize();
    d->UpdateViewSize();
}

QUrl QGraphicsMozView::url() const
{
    return QUrl(d->mLocation);
}

void QGraphicsMozView::setUrl(const QUrl& url)
{
    if (url.isEmpty())
        return;

    if (!d->mViewInitialized) {
        return;
    }
    LOGT("url: %s", url.toString().toUtf8().data());
    d->mView->LoadURL(url.toString().toUtf8().data());
}

void QGraphicsMozView::load(const QString& url)
{
    if (url.isEmpty())
        return;

    if (!d->mViewInitialized) {
        return;
    }
    LOGT("url: %s", url.toUtf8().data());
    d->mView->LoadURL(QUrl::fromUserInput(url).toString().toUtf8().data());
}

void QGraphicsMozView::loadFrameScript(const QString& name)
{
    LOGT("script:%s", name.toUtf8().data());
    d->mView->LoadFrameScript(name.toUtf8().data());
}

void QGraphicsMozView::addMessageListener(const QString& name)
{
    LOGT("name:%s", name.toUtf8().data());
    d->mView->AddMessageListener(name.toUtf8().data());
}

void QGraphicsMozView::sendAsyncMessage(const QString& name, const QVariant& variant)
{
    if (!d->mViewInitialized)
        return;

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QJson::Serializer serializer;
    QByteArray array = serializer.serialize(variant);
#else
    QJsonDocument doc = QJsonDocument::fromVariant(variant);
    QByteArray array = doc.toJson();
#endif

    d->mView->SendAsyncMessage((const PRUnichar*)name.constData(), NS_ConvertUTF8toUTF16(array.constData()).get());
}

void
QGraphicsMozView::sendAsyncMessage(const QString& name, const QString& message)
{
    if (!d->mViewInitialized)
        return;
    d->mView->SendAsyncMessage((const PRUnichar*)name.constData(), (const PRUnichar*)message.constData());
}

QPointF QGraphicsMozView::scrollableOffset() const
{
    return d->mScrollableOffset;
}

float QGraphicsMozView::resolution() const
{
    return d->mContentResolution;
}

QRect QGraphicsMozView::contentRect() const
{
    return d->mContentRect;
}

QSize QGraphicsMozView::scrollableSize() const
{
    return d->mScrollableSize;
}

QString QGraphicsMozView::title() const
{
    return d->mTitle;
}

int QGraphicsMozView::loadProgress() const
{
    return d->mProgress;
}

bool QGraphicsMozView::canGoBack() const
{
    return d->mCanGoBack;
}

bool QGraphicsMozView::canGoForward() const
{
    return d->mCanGoForward;
}

bool QGraphicsMozView::loading() const
{
    return d->mIsLoading;
}

void QGraphicsMozView::loadHtml(const QString& html, const QUrl& baseUrl)
{
    LOGT();
}

void QGraphicsMozView::goBack()
{
    LOGT();
    if (!d->mViewInitialized)
        return;
    d->mView->GoBack();
}

void QGraphicsMozView::goForward()
{
    LOGT();
    if (!d->mViewInitialized)
        return;
    d->mView->GoForward();
}

void QGraphicsMozView::stop()
{
    LOGT();
    if (!d->mViewInitialized)
        return;
    d->mView->StopLoad();
}

void QGraphicsMozView::reload()
{
    LOGT();
    if (!d->mViewInitialized)
        return;
    d->mView->Reload(false);
}

bool QGraphicsMozView::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        d->touchEvent(static_cast<QTouchEvent*>(event));
        return true;
    }
    case QEvent::Show: {
        LOGT("Event Show: curCtx:%p", QGLContext::currentContext());
        if (QGLContext::currentContext() && !getenv("SWRENDER")) {
            d->mContext->GetApp()->SetIsAccelerated(true);
        }
        break;
    }
    case QEvent::Hide: {
        LOGT("Event Hide");
        break;
    }
    default:
        break;
    }

    // Here so that it can be reimplemented without breaking ABI.
    return QGraphicsWidget::event(event);
}

void QGraphicsMozView::onDisplayEntered()
{
    if (!d->mView) {
        return;
    }
    d->mView->SetIsActive(true);
    d->mView->ResumeTimeouts();
}

void QGraphicsMozView::onDisplayExited()
{
    if (!d->mView) {
        return;
    }
    d->mView->SetIsActive(false);
    d->mView->SuspendTimeouts();
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
        switch (pt.state())
        {
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
    if (meventStart.mTouches.Length())
        ReceiveInputEvent(meventStart);
    if (meventMove.mTouches.Length())
        ReceiveInputEvent(meventMove);
    if (meventEnd.mTouches.Length())
        ReceiveInputEvent(meventEnd);
}

void QGraphicsMozViewPrivate::ReceiveInputEvent(const InputData& event)
{
    if (mViewInitialized) {
        mView->ReceiveInputEvent(event);
    }
}

void QGraphicsMozView::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        const bool accepted = e->isAccepted();
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_MOVE, d->mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(e->pos().x(), e->pos().y()),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
        e->setAccepted(accepted);
    }

    if (!e->isAccepted())
        QGraphicsItem::mouseMoveEvent(e);
}

void QGraphicsMozView::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    d->mPanningTime.restart();
    forceActiveFocus();
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        const bool accepted = e->isAccepted();
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_START, d->mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(e->pos().x(), e->pos().y()),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
        e->setAccepted(accepted);
    }

    if (!e->isAccepted())
        QGraphicsItem::mouseMoveEvent(e);
}

void QGraphicsMozView::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        const bool accepted = e->isAccepted();
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_END, d->mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(e->pos().x(), e->pos().y()),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
        e->setAccepted(accepted);
    }
    if (d->mPendingTouchEvent) {
        d->mPendingTouchEvent = false;
    }

    if (!e->isAccepted())
        QGraphicsItem::mouseMoveEvent(e);
}

void QGraphicsMozView::forceActiveFocus()
{
    QGraphicsItem *parent = parentItem();
    while (parent) {
        if (parent->flags() & QGraphicsItem::ItemIsFocusScope)
            parent->setFocus(Qt::OtherFocusReason);
        parent = parent->parentItem();
    }

    setFocus(Qt::OtherFocusReason);
    if (d->mViewInitialized) {
        d->mView->SetIsActive(true);
    }
}

void QGraphicsMozView::inputMethodEvent(QInputMethodEvent* event)
{
    LOGT("cStr:%s, preStr:%s, replLen:%i, replSt:%i", event->commitString().toUtf8().data(), event->preeditString().toUtf8().data(), event->replacementLength(), event->replacementStart());
    if (d->mViewInitialized) {
        d->mView->SendTextEvent(event->commitString().toUtf8().data(), event->preeditString().toUtf8().data());
    }
}

void QGraphicsMozView::keyPressEvent(QKeyEvent* event)
{
    if (!d->mViewInitialized)
        return;

    LOGT();
    int32_t gmodifiers = MozKey::QtModifierToDOMModifier(event->modifiers());
    int32_t domKeyCode = MozKey::QtKeyCodeToDOMKeyCode(event->key(), event->modifiers());
    int32_t charCode = 0;
    if (event->text().length() && event->text()[0].isPrint()) {
        charCode = (int32_t)event->text()[0].unicode();
        if (getenv("USE_TEXT_EVENTS")) {
            return;
        }
    }
    d->mView->SendKeyPress(domKeyCode, gmodifiers, charCode);
}

void QGraphicsMozView::keyReleaseEvent(QKeyEvent* event)
{
    if (!d->mViewInitialized)
        return;

    LOGT();
    int32_t gmodifiers = MozKey::QtModifierToDOMModifier(event->modifiers());
    int32_t domKeyCode = MozKey::QtKeyCodeToDOMKeyCode(event->key(), event->modifiers());
    int32_t charCode = 0;
    if (event->text().length() && event->text()[0].isPrint()) {
        charCode = (int32_t)event->text()[0].unicode();
        if (getenv("USE_TEXT_EVENTS")) {
            d->mView->SendTextEvent(event->text().toUtf8().data(), "");
            return;
        }
    }
    d->mView->SendKeyRelease(domKeyCode, gmodifiers, charCode);
}

QVariant
QGraphicsMozView::inputMethodQuery(Qt::InputMethodQuery aQuery) const
{
    static bool commitNow = getenv("DO_FAST_COMMIT") != 0;
    return commitNow ? QVariant(0) : QVariant();
}

void
QGraphicsMozView::newWindow(const QString& url)
{
    LOGT("New Window: %s", url.toUtf8().data());
}
