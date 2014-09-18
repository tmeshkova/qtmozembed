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
#include "mozilla/TimeStamp.h"

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
#include <QtOpenGLExtensions>

#include "qgraphicsmozview_p.h"
#include "EmbedQtKeyUtils.h"
#include "qmozscrolldecorator.h"
#include "qmoztexturenode.h"
#include "qmozextmaterialnode.h"
#include "qsgthreadobject.h"
#include "assert.h"

using namespace mozilla;
using namespace mozilla::embedlite;

#ifndef MOZVIEW_FLICK_STOP_TIMEOUT
#define MOZVIEW_FLICK_STOP_TIMEOUT 500
#endif

static QSGThreadObject* gSGRenderer = NULL;

QuickMozView::QuickMozView(QQuickItem *parent)
  : QQuickItem(parent)
  , d(new QGraphicsMozViewPrivate(new IMozQView<QuickMozView>(*this)))
  , mParentID(0)
  , mUseQmlMouse(false)
  , mMovingTimerId(0)
  , mBackgroundTimerId(0)
  , mOffsetX(0.0)
  , mOffsetY(0.0)
  , mPreedit(false)
  , mActive(false)
  , mBackground(false)
  , mWindowVisible(false)
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
    connect(this, SIGNAL(viewInitialized()), this, SLOT(processViewInitialization()));
    connect(this, SIGNAL(enabledChanged()), this, SLOT(updateEnabled()));
    connect(this, SIGNAL(dispatchItemUpdate()), this, SLOT(update()));

    updateEnabled();
}

QuickMozView::~QuickMozView()
{
    if (d->mView) {
        d->mView->SetListener(NULL);
        d->mContext->GetApp()->DestroyView(d->mView);
    }
    delete d;
    d = 0;
}

void
QuickMozView::SetIsActive(bool aIsActive)
{
    if (QThread::currentThread() == thread() && d->mView) {
        d->mView->SetIsActive(aIsActive);
        if (mActive) {
            updateGLContextInfo();
            d->UpdateViewSize();
        }
    } else {
        Q_EMIT setIsActive(aIsActive);
    }
}

void
QuickMozView::contextInitialized()
{
    LOGT("QuickMozView");
    d->mContext->setCompositorInSeparateThread(true);
    // We really don't care about SW rendering on Qt5 anymore
    d->mContext->GetApp()->SetIsAccelerated(true);
    createView();
}

void QuickMozView::processViewInitialization()
{
    // This is connected to view initialization. View must be initialized
    // over here.
    Q_ASSERT(d->mViewInitialized);
    SetIsActive(mActive);
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
    hasContext = false;
    viewPortSize = d->mGLSurfaceSize;
}

void QuickMozView::updateGLContextInfo(QOpenGLContext* ctx)
{
    d->mHasContext = ctx != nullptr && ctx->surface() != nullptr;
    if (!d->mHasContext) {
        printf("ERROR: QuickMozView not supposed to work without GL context\n");
        return;
    }
    updateGLContextInfo();
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
        // All of these signals are emitted from scene graph rendering thread.
        connect(win, SIGNAL(beforeRendering()), this, SLOT(refreshNodeTexture()), Qt::DirectConnection);
        connect(win, SIGNAL(beforeSynchronizing()), this, SLOT(createThreadRenderObject()), Qt::DirectConnection);
        connect(win, SIGNAL(sceneGraphInvalidated()), this, SLOT(clearThreadRenderObject()), Qt::DirectConnection);
        connect(win, SIGNAL(visibleChanged(bool)), this, SLOT(windowVisibleChanged(bool)));
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
        if (mActive) {
            updateGLContextInfo();
            d->UpdateViewSize();
        }
    }
}

void QuickMozView::createThreadRenderObject()
{
    updateGLContextInfo(QOpenGLContext::currentContext());
    if (!gSGRenderer) {
        gSGRenderer = new QSGThreadObject();
    }
    disconnect(window(), SIGNAL(beforeSynchronizing()), this, 0);
}

void QuickMozView::clearThreadRenderObject()
{
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    Q_ASSERT(ctx != NULL && ctx->makeCurrent(ctx->surface()));
    if (gSGRenderer != NULL) {
        delete gSGRenderer;
        gSGRenderer = NULL;
    }
    QQuickWindow *win = window();
    if (!win) return;
    connect(win, SIGNAL(beforeSynchronizing()), this, SLOT(createThreadRenderObject()), Qt::DirectConnection);
}

void QuickMozView::createView()
{
    if (!d->mView) {
        d->mView = d->mContext->GetApp()->CreateView(mParentID);
        d->mView->SetListener(d);
    }
}

QSGNode*
QuickMozView::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data)
{
#if defined(QT_OPENGL_ES_2)
#define TextureNodeType MozExtMaterialNode
#else
#define TextureNodeType MozTextureNode
#endif
    if (width() <= 0 || height() <= 0) {
        delete oldNode;
        return 0;
    }

    TextureNodeType* n = static_cast<TextureNodeType*>(oldNode);
    if (!n) {
        n = new TextureNodeType(this);
        connect(this, SIGNAL(textureReady(int,QSize)), n, SLOT(newTexture(int,QSize)), Qt::DirectConnection);
        connect(window(), SIGNAL(beforeRendering()), n, SLOT(prepareNode()), Qt::DirectConnection);
    }
    n->update();
    return n;
}

void QuickMozView::refreshNodeTexture()
{
    if (!d->mViewInitialized || !mActive)
        return;

    if (d && d->mView)
    {
        int width = 0, height = 0;
        static QOpenGLExtension_OES_EGL_image* extension = nullptr;
        if (!extension) {
            extension = new QOpenGLExtension_OES_EGL_image();
            extension->initializeOpenGLFunctions();
        }
        if (!mConsTex) {
          glGenTextures(1, &mConsTex);
        }
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, mConsTex);
        void* image = d->mView->GetPlatformImage(&width, &height);
        extension->glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
        Q_EMIT textureReady(mConsTex, QSize(width, height));
    }
}

void QuickMozView::windowVisibleChanged(bool visible)
{
    mWindowVisible = visible;
    if (mBackgroundTimerId == 0 && !mBackground) {
        mBackgroundTimerId = startTimer(1000);
    }
}

int QuickMozView::parentId() const
{
    return mParentID;
}

bool QuickMozView::active() const
{
    return mActive;
}

void QuickMozView::setActive(bool active)
{
    if (d->mViewInitialized) {
        if (mActive != active) {
            mActive = active;
            // Process pending paint request before final suspend (unblock possible content Compositor waiters Bug 1020350)
            SetIsActive(active);
            Q_EMIT activeChanged();
        }
    } else {
        // Will be processed once view is initialized.
        mActive = active;
    }
}

bool QuickMozView::background() const
{
    return mBackground;
}

void QuickMozView::CompositingFinished()
{
    Q_EMIT dispatchItemUpdate();
}

void QuickMozView::cleanup()
{
}

void QuickMozView::startMoveMonitoring()
{
    mMovingTimerId = startTimer(MOZVIEW_FLICK_STOP_TIMEOUT);
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
            if (event->commitString().isEmpty()) {
                d->mView->SendTextEvent(event->commitString().toUtf8().data(), event->preeditString().toUtf8().data());
            } else {
                d->mView->SendTextEvent(event->commitString().toUtf8().data(), event->preeditString().toUtf8().data());
                // After commiting pre-edit, we send "dummy" keypress.
                // Workaround for sites that enable "submit" button based on keypress events like
                // comment fields in FB, and m.linkedin.com
                // Chrome on Android does the same, but it does it also after each pre-edit change
                // We cannot do exectly the same here since sending keyevent with active pre-edit would commit gecko's
                // internal Input Engine's pre-edit
                d->mView->SendKeyPress(0, 0, 0);
                d->mView->SendKeyRelease(0, 0, 0);
            }
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
        setActive(true);
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
    return d->mContentResolution;
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
    if (aParentID != mParentID) {
        mParentID = aParentID;
        Q_EMIT parentIdChanged();
    }
}

void QuickMozView::synthTouchBegin(const QVariant& touches)
{
    QList<QVariant> list = touches.toList();
    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START,
                                QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
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
                                QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
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
                                QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
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
    setActive(false);
    d->mView->SuspendTimeouts();
}

void QuickMozView::resumeView()
{
    if (!d->mView) {
        return;
    }
    setActive(true);
    d->mView->ResumeTimeouts();
}

void QuickMozView::recvMouseMove(int posX, int posY)
{
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_MOVE,
                              QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
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
                              QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
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
                              QDateTime::currentMSecsSinceEpoch(), TimeStamp(), 0);
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
    if (event->timerId() == mMovingTimerId) {
        qreal offsetY = d->mScrollableOffset.y();
        qreal offsetX = d->mScrollableOffset.x();
        if (offsetX == mOffsetX && offsetY == mOffsetY) {
            d->ResetState();
            killTimer(mMovingTimerId);
            mMovingTimerId = 0;
        }
        mOffsetX = offsetX;
        mOffsetY = offsetY;
    } else if (event->timerId() == mBackgroundTimerId) {
        if (window()) {
            // Guard window visibility change was not cancelled after timer triggered.
            bool windowVisible = window()->isVisible();
            // mWindowVisible == mBackground visibility changed
            if (windowVisible == mWindowVisible && mWindowVisible == mBackground) {
                mBackground = !mWindowVisible;
                Q_EMIT backgroundChanged();
                if (mWindowVisible) {
                    killTimer(mBackgroundTimerId);
                    mBackgroundTimerId = 0;
                }
            }
        }
    }
}

void QuickMozView::componentComplete()
{
    QQuickItem::componentComplete();
    // The first created view gets always parentId of 0
    if (!d->mContext->initialized()) {
        connect(d->mContext, SIGNAL(onInitialized()), this, SLOT(contextInitialized()));
    } else {
        createView();
    }
}
