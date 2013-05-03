/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QGraphicsMozView"

#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include <QtOpenGL/QGLContext>
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QInputContext>
#include <qjson/serializer.h>
#include <qjson/parser.h>
#else
#include <QJsonDocument>
#include <QJsonParseError>
#endif
#include "EmbedQtKeyUtils.h"

#include "qgraphicsmozview.h"
#include "qmozcontext.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLog.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

#include "qgraphicsmozview_p.h"

using namespace mozilla;
using namespace mozilla::embedlite;

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
    d->mContext->GetApp()->DestroyView(d->mView);
    if (d->mView) {
        d->mView->SetListener(NULL);
    }
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

void
QGraphicsMozView::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget*)
{
    if (!d->mGraphicsViewAssigned) {
        d->mGraphicsViewAssigned = true;
        // Disable for future gl context in case if we did not get it yet
        if (d->mViewInitialized &&
            d->mContext->GetApp()->IsAccelerated() &&
            !QGLContext::currentContext()) {
            LOGT("Gecko is setup for GL rendering but no context available on paint, disable it");
            d->mContext->setIsAccelerated(false);
        }
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
                painter->beginNativePainting();
                d->mView->RenderGL();
                painter->endNativePainting();
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
    load(url.toString());
}

void QGraphicsMozView::load(const QString& url)
{
    if (url.isEmpty())
        return;

    if (!d->mViewInitialized) {
        return;
    }
    LOGT("url: %s", url.toUtf8().data());
    d->mProgress = 0;
    d->mView->LoadURL(url.toUtf8().data());
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

QPointF QGraphicsMozView::scrollableOffset() const
{
    return d->mScrollableOffset;
}

bool QGraphicsMozView::isPainted() const
{
    return d->mIsPainted;
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

void
QGraphicsMozView::synthTouchBegin(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    d->mTouchTime.restart();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START, d->mTouchTime.elapsed());
    int ptId = 0;
    for(QList<QVariant>::iterator it = list.begin(); it != list.end(); it++)
    {
        const QPointF pt = (*it).toPointF();
        nsIntPoint nspt(pt.x(), pt.y());
        ptId++;
        meventStart.mTouches.AppendElement(SingleTouchData(ptId,
                                                           nspt,
                                                           nsIntPoint(1, 1),
                                                           180.0f,
                                                           1.0f));
    }
    d->mView->ReceiveInputEvent(meventStart);
}

void
QGraphicsMozView::synthTouchMove(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_MOVE, d->mTouchTime.elapsed());
    int ptId = 0;
    for(QList<QVariant>::iterator it = list.begin(); it != list.end(); it++)
    {
        const QPointF pt = (*it).toPointF();
        nsIntPoint nspt(pt.x(), pt.y());
        ptId++;
        meventStart.mTouches.AppendElement(SingleTouchData(ptId,
                                                           nspt,
                                                           nsIntPoint(1, 1),
                                                           180.0f,
                                                           1.0f));
    }
    d->mView->ReceiveInputEvent(meventStart);
}

void
QGraphicsMozView::synthTouchEnd(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_END, d->mTouchTime.elapsed());
    int ptId = 0;
    for(QList<QVariant>::iterator it = list.begin(); it != list.end(); it++)
    {
        const QPointF pt = (*it).toPointF();
        nsIntPoint nspt(pt.x(), pt.y());
        ptId++;
        meventStart.mTouches.AppendElement(SingleTouchData(ptId,
                                                           nspt,
                                                           nsIntPoint(1, 1),
                                                           180.0f,
                                                           1.0f));
    }
    d->mView->ReceiveInputEvent(meventStart);
}

void
QGraphicsMozView::scrollTo(const QPointF& position)
{
    LOGT("NOT IMPLEMENTED");
}
