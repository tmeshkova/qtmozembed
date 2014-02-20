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
#include "qmozscrolldecorator.h"
#include "qmoztexturenode.h"
#include "qsgthreadobject.h"
#include "assert.h"
#ifndef NO_PRIVATE_API
#include "qmozviewsgnode.h"
#endif

using namespace mozilla;
using namespace mozilla::embedlite;

#ifndef MOZVIEW_FLICK_STOP_TIMEOUT
#define MOZVIEW_FLICK_STOP_TIMEOUT 500
#endif

QuickMozView::QuickMozView(QQuickItem *parent)
  : QQuickItem(parent)
  , d(new QGraphicsMozViewPrivate(new IMozQView<QuickMozView>(*this)))
  , mParentID(0)
  , mUseQmlMouse(false)
  , mSGRenderer(NULL)
  , mTimerId(0)
  , mOffsetX(0.0)
  , mOffsetY(0.0)
#ifndef NO_PRIVATE_API
  , mInThreadRendering(false)
#endif
  , mPreedit(false)
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
    connect(this, SIGNAL(enabledChanged()), this, SLOT(updateEnabled()));
    connect(this, SIGNAL(dispatchItemUpdate()), this, SLOT(update()));
    updateEnabled();
    if (!d->mContext->initialized()) {
        connect(d->mContext, SIGNAL(onInitialized()), this, SLOT(onInitialized()));
    } else {
        // QQuickWindow::sceneGraphInitialized is emitted only once for a QQuickWindow.
        init();
        onInitialized();
    }
}

QuickMozView::~QuickMozView()
{
    if (d->mView) {
        d->mView->SetListener(NULL);
        d->mContext->GetApp()->DestroyView(d->mView);
    }
    delete mSGRenderer;
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
#ifndef NO_PRIVATE_API
    if (mInThreadRendering) {
        onRenderThreadReady();
    }
    else
#endif
    {
        d->mContext->setCompositorInSeparateThread(true);
        Q_EMIT wrapRenderThreadGLContext();
        update();
    }
}

void
QuickMozView::onRenderThreadReady()
{
    if (!d->mView) {
        // We really don't care about SW rendering on Qt5 anymore
        d->mContext->GetApp()->SetIsAccelerated(true);
        d->mView = d->mContext->GetApp()->CreateView();
        d->mView->SetListener(d);
    }
}

void QuickMozView::updateEnabled()
{
    d->mEnabled = QQuickItem::isEnabled();
}

void QuickMozView::createGeckoGLContext()
{
#warning "Unused method, destroy?"
}

void QuickMozView::requestGLContext(bool& hasContext, QSize& viewPortSize)
{
    hasContext = d->mHasContext;
#ifndef NO_PRIVATE_API
    hasContext = hasContext && mInThreadRendering;
#endif
    viewPortSize = d->mGLSurfaceSize;
}

void QuickMozView::updateGLContextInfo(QOpenGLContext* ctx)
{
    d->mHasContext = true;
    d->mGLSurfaceSize = ctx->surface()->size();
}

/**
 *  Updates gl surface size based on current content orientation.
 *  This does not do anything if QQuickItem::window() is null.
 */
void QuickMozView::updateGLContextInfo()
{
    if (window()) {
        Qt::ScreenOrientation orientation = window()->contentOrientation();
        QSize viewPortSize;
        int minValue = qMin(window()->width(), window()->height());
        int maxValue = qMax(window()->width(), window()->height());

        switch (orientation) {
        case Qt::LandscapeOrientation:
        case Qt::InvertedLandscapeOrientation:
            viewPortSize.setWidth(maxValue);
            viewPortSize.setHeight(minValue);
            LOGT("Update landscape viewPortSize: [%d,%d]", viewPortSize.width(), viewPortSize.height());
            break;
        default:
            viewPortSize.setWidth(minValue);
            viewPortSize.setHeight(maxValue);
            LOGT("Update portrait viewPortSize: [%d,%d]", viewPortSize.width(), viewPortSize.height());
            break;
        }

        d->mGLSurfaceSize = viewPortSize;
    }
}

void QuickMozView::itemChange(ItemChange change, const ItemChangeData &)
{
    if (change == ItemSceneChange) {
        QQuickWindow *win = window();
        if (!win)
            return;
        connect(win, SIGNAL(beforeRendering()), this, SLOT(beforeRendering()), Qt::DirectConnection);
        connect(win, SIGNAL(sceneGraphInitialized()), this, SLOT(init()), Qt::DirectConnection);
        win->setClearBeforeRendering(false);
    }
}

void QuickMozView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    LOGT("newGeometry size: [%g, %g] oldGeometry size: [%g,%g]", newGeometry.size().width(),
                                                                 newGeometry.size().height(),
                                                                 oldGeometry.size().width(),
                                                                 oldGeometry.size().height());
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    // Width and height are updated separately. So we want to avoid cases where width and height
    // equals or size have not actually changed at all. This will trigger viewport
    // calculation update.
    if (newGeometry.width() != newGeometry.height() && d->mSize != newGeometry.size()) {
        d->mSize = newGeometry.size();
        updateGLContextInfo();
        d->UpdateViewSize();
    }
}

void QuickMozView::init()
{
#ifndef NO_PRIVATE_API
    if (thread() == QThread::currentThread()) {
        mInThreadRendering = true;
    }
    else
#endif
    {
        mSGRenderer = new QSGThreadObject();
        connect(mSGRenderer, SIGNAL(onRenderThreadReady()), this, SLOT(onRenderThreadReady()));
        connect(this, SIGNAL(wrapRenderThreadGLContext()), mSGRenderer, SLOT(onWrapRenderThreadGLContext()));
    }

    updateGLContextInfo(QOpenGLContext::currentContext());
}

void QuickMozView::beforeRendering()
{
    if (!d->mViewInitialized)
        return;

#ifndef NO_PRIVATE_API
    if (!mInThreadRendering)
#endif
    {
        RefreshNodeTexture();
    }
}

void QuickMozView::RenderToCurrentContext()
{
    QMatrix affine;
    gfxMatrix matr(affine.m11(), affine.m12(), affine.m21(), affine.m22(), affine.dx(), affine.dy());
    d->mView->SetGLViewTransform(matr);
    d->mView->SetViewClipping(0, 0, d->mSize.width(), d->mSize.height());
    d->mView->RenderGL();
}

QSGNode*
QuickMozView::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data)
{
#ifndef NO_PRIVATE_API
    if (mInThreadRendering) {
        if (!d->mViewInitialized) {
            return oldNode;
        }
        QMozViewSGNode* n = static_cast<QMozViewSGNode*>(oldNode);

        const QWindow* window = this->window();
        assert(window);

        if (!n)
            n = new QMozViewSGNode;

        n->setRenderer(d, this);

        return n;
    }
#endif

    MozTextureNode *n = static_cast<MozTextureNode*>(oldNode);
    if (!n) {
        n = new MozTextureNode(this);
        connect(this, SIGNAL(textureReady(int,QSize)), n, SLOT(newTexture(int,QSize)), Qt::DirectConnection);
        connect(window(), SIGNAL(beforeRendering()), n, SLOT(prepareNode()), Qt::DirectConnection);
    }
    n->setRect(boundingRect());
    n->markDirty(QSGNode::DirtyMaterial);
    return n;
}

void QuickMozView::RefreshNodeTexture()
{
    int texId = 0, width = 0, height = 0;
    if (d->mView->GetPendingTexture(mSGRenderer->GetTargetContextWrapper(), &texId, &width, &height)) {
       Q_EMIT textureReady(texId, QSize(width, height));
    }
}

bool QuickMozView::Invalidate()
{
#ifndef NO_PRIVATE_API
    if (mInThreadRendering) {
        update();
        return true;
    }
#endif
    QMatrix affine;
    gfxMatrix matr(affine.m11(), affine.m12(), affine.m21(), affine.m22(), affine.dx(), affine.dy());
    d->mView->SetGLViewTransform(matr);
    d->mView->SetViewClipping(0, 0, d->mSize.width(), d->mSize.height());
    return false;
}

void QuickMozView::CompositingFinished()
{
#ifndef NO_PRIVATE_API
    if (!mInThreadRendering)
#endif
    {
        Q_EMIT dispatchItemUpdate();
    }
}

void QuickMozView::cleanup()
{
}

void QuickMozView::startMoveMonitoring()
{
    mTimerId = startTimer(MOZVIEW_FLICK_STOP_TIMEOUT);
    d->mFlicking = true;
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
    mPreedit = !event->preeditString().isEmpty();
    if (d->mViewInitialized) {
        if (d->mInputMethodHints & Qt::ImhFormattedNumbersOnly || d->mInputMethodHints & Qt::ImhDialableCharactersOnly) {
            bool ok;
            int asciiNumber = event->commitString().toInt(&ok) + Qt::Key_0;

            if (ok) {
                int32_t domKeyCode = MozKey::QtKeyCodeToDOMKeyCode(asciiNumber, Qt::NoModifier);
                int32_t charCode = 0;

                if (event->commitString().length() && event->commitString()[0].isPrint()) {
                    charCode = (int32_t)event->commitString()[0].unicode();
                }
                d->mView->SendKeyPress(domKeyCode, 0, charCode);
                d->mView->SendKeyRelease(domKeyCode, 0, charCode);
                qGuiApp->inputMethod()->reset();
            } else {
                d->mView->SendTextEvent(event->commitString().toUtf8().data(), event->preeditString().toUtf8().data());
            }
        } else {
            d->mView->SendTextEvent(event->commitString().toUtf8().data(), event->preeditString().toUtf8().data());
        }
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

bool QuickMozView::moving() const
{
    return d->mMoving;
}

bool QuickMozView::pinching() const{
    return d->mPinching;
}

QMozScrollDecorator* QuickMozView::verticalScrollDecorator() const
{
    return &d->mVerticalScrollDecorator;
}

QMozScrollDecorator* QuickMozView::horizontalScrollDecorator() const
{
    return &d->mHorizontalScrollDecorator;
}

bool QuickMozView::chromeGestureEnabled() const
{
    return d->mChromeGestureEnabled;
}

void QuickMozView::setChromeGestureEnabled(bool value)
{
    if (value != d->mChromeGestureEnabled) {
        d->mChromeGestureEnabled = value;
        Q_EMIT chromeGestureEnabledChanged();
    }
}

qreal QuickMozView::chromeGestureThreshold() const
{
    return d->mChromeGestureThreshold;
}

void QuickMozView::setChromeGestureThreshold(qreal value)
{
    if (value != d->mChromeGestureThreshold) {
        d->mChromeGestureThreshold = value;
        Q_EMIT chromeGestureThresholdChanged();
    }
}

bool QuickMozView::chrome() const
{
    return d->mChrome;
}

void QuickMozView::setChrome(bool value)
{
    if (value != d->mChrome) {
        d->mChrome = value;
        Q_EMIT chromeChanged();
    }
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

    d->mView->SendAsyncMessage((const char16_t*)name.constData(), NS_ConvertUTF8toUTF16(array.constData()).get());
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
        messages.AppendElement((char16_t*)messageNamesList.at(i).data());
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
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START,
                                QDateTime::currentMSecsSinceEpoch(), 0);
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
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_MOVE,
                                QDateTime::currentMSecsSinceEpoch(), 0);
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
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_END,
                                QDateTime::currentMSecsSinceEpoch(), 0);
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
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_MOVE,
                              QDateTime::currentMSecsSinceEpoch(), 0);
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
    forceViewActiveFocus();
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_START,
                              QDateTime::currentMSecsSinceEpoch(), 0);
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
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_END,
                              QDateTime::currentMSecsSinceEpoch(), 0);
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

    if (!mUseQmlMouse || event->touchPoints().count() > 1) {
        d->touchEvent(event);
    } else {
        QQuickItem::touchEvent(event);
    }
}

void QuickMozView::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == mTimerId) {
        qreal offsetY = d->mScrollableOffset.y();
        qreal offsetX = d->mScrollableOffset.x();
        if (offsetX == mOffsetX && offsetY == mOffsetY) {
            d->ResetState();
            killTimer(mTimerId);
            mTimerId = 0;
        }
        mOffsetX = offsetX;
        mOffsetY = offsetY;
    }
}
