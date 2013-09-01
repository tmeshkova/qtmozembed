/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "quickmozview.h"

#include "mozilla-config.h"
#include "qmozcontext.h"
#include "qmozembedlog.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedLiteRenderTarget.h"

#include <QTimer>
#include <QThread>
#include <QtOpenGL/QGLContext>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QtQuick/qquickwindow.h>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QSGSimpleRectNode>
#include <QSGSimpleTextureNode>
#include "qgraphicsmozview_p.h"
#include "EmbedQtKeyUtils.h"
#include "qmozviewsgnode.h"
#include "qsgthreadobject.h"
#include "qmcthreadobject.h"
#include "assert.h"

using namespace mozilla;
using namespace mozilla::embedlite;

QuickMozView::QuickMozView(QQuickItem *parent)
  : QQuickItem(parent)
  , d(new QGraphicsMozViewPrivate(new IMozQView<QuickMozView>(*this)))
  , mParentID(0)
  , mUseQmlMouse(false)
  , mSGRenderer(NULL)
  , mMCRenderer(NULL)
{
    static bool Initialized = false;
    if (!Initialized) {
        qmlRegisterType<QMozReturnValue>("QtMozilla", 1, 0, "QMozReturnValue");
        Initialized = true;
    }

    setFlag(ItemHasContents, true);

    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setFlag(ItemClipsChildrenToShape, true);
    setFlag(ItemIsFocusScope, true);
    setFlag(ItemAcceptsDrops, true);
    setFlag(ItemAcceptsInputMethod, true);

    d->mContext = QMozContext::GetInstance();
    connect(this, SIGNAL(setIsActive(bool)), this, SLOT(SetIsActive(bool)));
    connect(this, SIGNAL(updateThreaded()), this, SLOT(update()));
    if (!d->mContext->initialized()) {
        connect(d->mContext, SIGNAL(onInitialized()), this, SLOT(onInitialized()));
    } else {
        QTimer::singleShot(0, this, SLOT(onInitialized()));
    }
}

QuickMozView::~QuickMozView()
{
    if (mMCRenderer) {
        mMCRenderer->setSGNode(nullptr);
        mMCRenderer->setView(nullptr);
    }
    if (d->mView) {
        d->mView->SetListener(NULL);
        d->mContext->GetApp()->DestroyView(d->mView);
    }
    delete mSGRenderer;
    delete mMCRenderer;
    delete d;
}

void
QuickMozView::SetIsActive(bool aIsActive)
{
    if (QThread::currentThread() == thread()) {
        d->mView->SetIsActive(aIsActive);
    } else {
        Q_EMIT setIsActive(aIsActive);
    }
}

void
QuickMozView::onInitialized()
{
    LOGT("QuickMozView");
    if (!d->mView) {
        // We really don't care about SW rendering on Qt5 anymore
        d->mContext->GetApp()->SetIsAccelerated(true);
        d->mView = d->mContext->GetApp()->CreateView();
        d->mView->SetListener(d);
    }
}

void QuickMozView::createGeckoGLContext()
{
    if (!mMCRenderer && mSGRenderer) {
        mMCRenderer = new QMCThreadObject(this, mSGRenderer, d->mGLSurfaceSize);
    }
}

void QuickMozView::requestGLContext(bool& hasContext, QSize& viewPortSize)
{
    hasContext = d->mHasContext;
    viewPortSize = d->mGLSurfaceSize;
}

void QuickMozView::updateGLContextInfo(bool hasContext, QSize viewPortSize)
{
    d->mHasContext = hasContext;
    d->mGLSurfaceSize = viewPortSize;
    QRectF r(0, 0, d->mGLSurfaceSize.width(), d->mGLSurfaceSize.height());
    r = mapRectToScene(r);
    d->mGLSurfaceSize = r.size().toSize();
}

void QuickMozView::itemChange(ItemChange change, const ItemChangeData &)
{
    if (change == ItemSceneChange) {
        QQuickWindow *win = window();
        if (!win)
            return;
        connect(win, SIGNAL(beforeRendering()), this, SLOT(beforeRendering()), Qt::DirectConnection);
        connect(win, SIGNAL(sceneGraphInitialized()), this, SLOT(sceneGraphInitialized()), Qt::DirectConnection);
        win->setClearBeforeRendering(false);
    }
}

void QuickMozView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    d->mSize = newGeometry.size().toSize();
    d->UpdateViewSize();
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

void QuickMozView::sceneGraphInitialized()
{
}

void QuickMozView::beforeRendering()
{
    if (!mSGRenderer) {
        mSGRenderer = new QSGThreadObject();
        connect(mSGRenderer, SIGNAL(updateGLContextInfo(bool,QSize)), this, SLOT(updateGLContextInfo(bool,QSize)));
        mSGRenderer->setupCurrentGLContext();
    }

    if (!d->mGraphicsViewAssigned) {
        d->UpdateViewSize();
        d->mGraphicsViewAssigned = true;
        // Disable for future gl context in case if we did not get it yet
        if (d->mViewInitialized &&
            d->mContext->GetApp()->IsAccelerated() &&
            !QOpenGLContext::currentContext()) {
            LOGT("Gecko is setup for GL rendering but no context available on paint, disable it");
            d->mContext->setIsAccelerated(false);
        }
    }

    if (mMCRenderer) {
        mMCRenderer->prepareTexture();
    }
}

void QuickMozView::RenderToCurrentContext(QMatrix affine, EmbedLiteRenderTarget* renderTarget)
{
    if (mMCRenderer && mMCRenderer->thread() != QThread::currentThread()) {
        mMCRenderer->RenderToCurrentContext(affine);
        return;
    }
    gfxMatrix matr(affine.m11(), affine.m12(), affine.m21(), affine.m22(), affine.dx(), affine.dy());
    d->mView->SetGLViewTransform(matr);
    d->mView->SetViewClipping(0, 0, d->mSize.width(), d->mSize.height());
    d->mView->RenderGL(renderTarget);
}

mozilla::embedlite::EmbedLiteRenderTarget*
QuickMozView::CreateEmbedLiteRenderTarget(QSize size)
{
    return d->mView->CreateEmbedLiteRenderTarget(size.width(), size.height());
}

QSGNode*
QuickMozView::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data)
{
    if (!d->mViewInitialized)
        return oldNode;

    if (!d->mContext->GetApp()->IsAccelerated()) {
        QSGSimpleTextureNode *n = static_cast<QSGSimpleTextureNode*>(oldNode);
        if (!n) {
            n = new QSGSimpleTextureNode();
        }
        QRect r(boundingRect().toRect());
        if (d->mTempBufferImage.isNull() || d->mTempBufferImage.width() != r.width() || d->mTempBufferImage.height() != r.height()) {
            d->mTempBufferImage = QImage(r.size(), QImage::Format_RGB32);
        }
        if (mMCRenderer && mMCRenderer->thread() != QThread::currentThread()) {
            printf("FIXME: Cannot perform SW rendering across threads\n");
            d->mTempBufferImage.fill(Qt::white);
        } else {
            d->mView->RenderToImage(d->mTempBufferImage.bits(), d->mTempBufferImage.width(),
                                    d->mTempBufferImage.height(), d->mTempBufferImage.bytesPerLine(),
                                    d->mTempBufferImage.depth());
        }

        if (d->mTempTexture)
            delete d->mTempTexture;
        d->mTempTexture = window()->createTextureFromImage(d->mTempBufferImage);
        n->setTexture(d->mTempTexture);
        n->setRect(boundingRect());
        return n;
    }

    QMozViewSGNode* node = static_cast<QMozViewSGNode*>(oldNode);

    if (!node) {
        node = new QMozViewSGNode;
        if (mMCRenderer) {
            mMCRenderer->setSGNode(node);
        }
    }

    node->setRenderer(this);
    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}

void QuickMozView::cleanup()
{
}

void QuickMozView::Invalidate()
{
    if (QThread::currentThread() != thread()) {
        Q_EMIT updateThreaded();
    } else {
        update();
    }
}

void QuickMozView::mouseMoveEvent(QMouseEvent* e)
{
    if (!mUseQmlMouse) {
        const bool accepted = e->isAccepted();
        recvMouseMove(e->pos().x(), e->pos().y());
        e->setAccepted(accepted);
    }
    else {
        QQuickItem::mouseMoveEvent(e);
    }
}

void QuickMozView::mousePressEvent(QMouseEvent* e)
{
    if (!mUseQmlMouse) {
        const bool accepted = e->isAccepted();
        recvMousePress(e->pos().x(), e->pos().y());
        e->setAccepted(accepted);
    }
    else {
        QQuickItem::mousePressEvent(e);
    }
}

void QuickMozView::mouseReleaseEvent(QMouseEvent* e)
{
    if (!mUseQmlMouse) {
        const bool accepted = e->isAccepted();
        recvMouseRelease(e->pos().x(), e->pos().y());
        e->setAccepted(accepted);
    }
    else {
        QQuickItem::mouseReleaseEvent(e);
    }
}

void QuickMozView::setInputMethodHints(Qt::InputMethodHints hints)
{
    d->mInputMethodHints = hints;
}

void QuickMozView::inputMethodEvent(QInputMethodEvent* event)
{
    LOGT("cStr:%s, preStr:%s, replLen:%i, replSt:%i", event->commitString().toUtf8().data(), event->preeditString().toUtf8().data(), event->replacementLength(), event->replacementStart());
    if (d->mViewInitialized) {
        d->mView->SendTextEvent(event->commitString().toUtf8().data(), event->preeditString().toUtf8().data());
    }
}

void QuickMozView::keyPressEvent(QKeyEvent* event)
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

void QuickMozView::keyReleaseEvent(QKeyEvent* event)
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
QuickMozView::inputMethodQuery(Qt::InputMethodQuery property) const
{
    switch (property) {
    case Qt::ImEnabled:
        return QVariant((bool) d->mIsInputFieldFocused);
    case Qt::ImHints:
        return QVariant((int) d->mInputMethodHints);
    default:
        return QVariant();
    }
}

void QuickMozView::focusInEvent(QFocusEvent* event)
{
    d->SetIsFocused(true);
    QQuickItem::focusInEvent(event);
}

void QuickMozView::focusOutEvent(QFocusEvent* event)
{
    d->SetIsFocused(false);
    QQuickItem::focusOutEvent(event);
}

void QuickMozView::forceViewActiveFocus()
{
    forceActiveFocus();
    if (d->mViewInitialized) {
        d->mView->SetIsActive(true);
        d->mView->SetIsFocused(true);
    }
}

QUrl QuickMozView::url() const
{
    return QUrl(d->mLocation);
}

void QuickMozView::setUrl(const QUrl& url)
{
    load(url.toString());
}

QString QuickMozView::title() const
{
    return d->mTitle;
}

int QuickMozView::loadProgress() const
{
    return d->mProgress;
}

bool QuickMozView::canGoBack() const
{
    return d->mCanGoBack;
}

bool QuickMozView::canGoForward() const
{
    return d->mCanGoForward;
}

bool QuickMozView::loading() const
{
    return d->mIsLoading;
}

QRectF QuickMozView::contentRect() const
{
    return d->mContentRect;
}

QSizeF QuickMozView::scrollableSize() const
{
    return d->mScrollableSize;
}

QPointF QuickMozView::scrollableOffset() const
{
    return d->mScrollableOffset;
}

float QuickMozView::resolution() const
{
    return d->mContext->pixelRatio();
}

bool QuickMozView::isPainted() const
{
    return d->mIsPainted;
}

QColor QuickMozView::bgcolor() const
{
    return d->mBgColor;
}

bool QuickMozView::getUseQmlMouse()
{
    return mUseQmlMouse;
}

void QuickMozView::setUseQmlMouse(bool value)
{
    mUseQmlMouse = value;
}

bool QuickMozView::dragging() const
{
    return d->mDragging;
}

qreal QuickMozView::contentWidth() const
{
    return d->mScrollableSize.width();
}

qreal QuickMozView::contentHeight() const
{
    return d->mScrollableSize.height();
}


void QuickMozView::loadHtml(const QString& html, const QUrl& baseUrl)
{
    LOGT();
}

void QuickMozView::goBack()
{
    if (!d->mViewInitialized)
        return;
    d->mView->GoBack();
}

void QuickMozView::goForward()
{
    if (!d->mViewInitialized)
        return;
    d->mView->GoForward();
}

void QuickMozView::stop()
{
    if (!d->mViewInitialized)
        return;
    d->mView->StopLoad();
}

void QuickMozView::reload()
{
    if (!d->mViewInitialized)
        return;
    d->mView->Reload(false);
}

void QuickMozView::load(const QString& url)
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

void QuickMozView::sendAsyncMessage(const QString& name, const QVariant& variant)
{
    if (!d->mViewInitialized)
        return;

    QJsonDocument doc = QJsonDocument::fromVariant(variant);
    QByteArray array = doc.toJson();

    d->mView->SendAsyncMessage((const PRUnichar*)name.constData(), NS_ConvertUTF8toUTF16(array.constData()).get());
}

void QuickMozView::addMessageListener(const QString& name)
{
    if (!d->mViewInitialized)
        return;

    d->mView->AddMessageListener(name.toUtf8().data());
}

void QuickMozView::addMessageListeners(const QStringList& messageNamesList)
{
    if (!d->mViewInitialized)
        return;

    nsTArray<nsString> messages;
    for (int i = 0; i < messageNamesList.size(); i++) {
        messages.AppendElement((PRUnichar*)messageNamesList.at(i).data());
    }
    d->mView->AddMessageListeners(messages);
}

void QuickMozView::loadFrameScript(const QString& name)
{
    d->mView->LoadFrameScript(name.toUtf8().data());
}

void QuickMozView::newWindow(const QString& url)
{
    LOGT("New Window: %s", url.toUtf8().data());
}

quint32 QuickMozView::uniqueID() const
{
    return d->mView ? d->mView->GetUniqueID() : 0;
}

void QuickMozView::setParentID(unsigned aParentID)
{
    mParentID = aParentID;
    if (mParentID) {
        onInitialized();
    }
}

void QuickMozView::synthTouchBegin(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    d->mTouchTime.restart();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START, d->mTouchTime.elapsed());
    int ptId = 0;
    for(QList<QVariant>::iterator it = list.begin(); it != list.end(); it++)
    {
        const QPointF pt = (*it).toPointF();
        mozilla::ScreenIntPoint nspt(pt.x(), pt.y());
        ptId++;
        meventStart.mTouches.AppendElement(SingleTouchData(ptId,
                                                           nspt,
                                                           mozilla::ScreenSize(1, 1),
                                                           180.0f,
                                                           1.0f));
    }
    d->mView->ReceiveInputEvent(meventStart);
}

void QuickMozView::synthTouchMove(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_MOVE, d->mTouchTime.elapsed());
    int ptId = 0;
    for(QList<QVariant>::iterator it = list.begin(); it != list.end(); it++)
    {
        const QPointF pt = (*it).toPointF();
        mozilla::ScreenIntPoint nspt(pt.x(), pt.y());
        ptId++;
        meventStart.mTouches.AppendElement(SingleTouchData(ptId,
                                                           nspt,
                                                           mozilla::ScreenSize(1, 1),
                                                           180.0f,
                                                           1.0f));
    }
    d->mView->ReceiveInputEvent(meventStart);
}

void QuickMozView::synthTouchEnd(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_END, d->mTouchTime.elapsed());
    int ptId = 0;
    for(QList<QVariant>::iterator it = list.begin(); it != list.end(); it++)
    {
        const QPointF pt = (*it).toPointF();
        mozilla::ScreenIntPoint nspt(pt.x(), pt.y());
        ptId++;
        meventStart.mTouches.AppendElement(SingleTouchData(ptId,
                                                           nspt,
                                                           mozilla::ScreenSize(1, 1),
                                                           180.0f,
                                                           1.0f));
    }
    d->mView->ReceiveInputEvent(meventStart);
}

void QuickMozView::scrollTo(const QPointF& position)
{
    LOGT("NOT IMPLEMENTED");
}

void QuickMozView::suspendView()
{
    if (!d->mView) {
        return;
    }
    d->mView->SetIsActive(false);
    d->mView->SuspendTimeouts();
}

void QuickMozView::resumeView()
{
    if (!d->mView) {
        return;
    }
    d->mView->SetIsActive(true);
    d->mView->ResumeTimeouts();
}

void QuickMozView::recvMouseMove(int posX, int posY)
{
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_MOVE, d->mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     mozilla::ScreenIntPoint(posX, posY),
                                     mozilla::ScreenSize(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
    }
}

void QuickMozView::recvMousePress(int posX, int posY)
{
    d->mPanningTime.restart();
    forceViewActiveFocus();
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_START, d->mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     mozilla::ScreenIntPoint(posX, posY),
                                     mozilla::ScreenSize(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
    }
}

void QuickMozView::recvMouseRelease(int posX, int posY)
{
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_END, d->mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     mozilla::ScreenIntPoint(posX, posY),
                                     mozilla::ScreenSize(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
    }
    if (d->mPendingTouchEvent) {
        d->mPendingTouchEvent = false;
    }
}

void QuickMozView::touchEvent(QTouchEvent *event)
{
    if (!mUseQmlMouse || event->touchPoints().count() > 1) {
        d->touchEvent(event);
    } else {
        QQuickItem::touchEvent(event);
    }
}
