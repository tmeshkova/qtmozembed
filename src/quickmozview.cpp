#include "quickmozview.h"

#include "mozilla-config.h"
#include "qmozcontext.h"
#include "qmozembedlog.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

#include <QTimer>
#include <QtOpenGL/QGLContext>
#include <QApplication>
#include <QtQuick/qquickwindow.h>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QSGSimpleRectNode>
#include <QSGSimpleTextureNode>

using namespace mozilla;
using namespace mozilla::embedlite;

class QuickMozViewPrivate : public EmbedLiteViewListener {
public:
    QuickMozViewPrivate(QuickMozView* view)
      : q(view)
      , mContext(NULL)
      , mView(NULL)
      , mViewInitialized(false)
      , mViewGLSized(false)
      , mTempTexture(NULL)
    {
    }
    virtual ~QuickMozViewPrivate() {}

    virtual bool RequestCurrentGLContext()
    {
        // Need really check what is what and switch to common implementation
        const QOpenGLContext* ctx = QOpenGLContext::currentContext();
        return ctx && ctx->surface();
    }

    void UpdateViewSize(bool updateSize = true)
    {
        if (mViewInitialized) {
            if (mContext->GetApp()->IsAccelerated()) {
                const QOpenGLContext* ctx = QOpenGLContext::currentContext();
                if (ctx && ctx->surface()) {
                    QRectF r(0, 0, ctx->surface()->size().width(), ctx->surface()->size().height());
                    r = q->mapRectToScene(r);
                    mView->SetGLViewPortSize(r.width(), r.height());
                }
            }
            mView->SetViewSize(q->boundingRect().width(), q->boundingRect().height());
        }
    }
    virtual void ViewInitialized() {
        mViewInitialized = true;
        UpdateViewSize();
        mView->LoadURL("about:mozilla");
    }
    virtual bool Invalidate() {
        q->update();
        return true;
    }

    QuickMozView* q;
    QMozContext* mContext;
    EmbedLiteView* mView;
    bool mViewInitialized;
    bool mViewGLSized;
    QImage mTempBufferImage;
    QSGTexture* mTempTexture;
};

QuickMozView::QuickMozView(QQuickItem *parent)
  : QQuickItem(parent)
  , d(new QuickMozViewPrivate(this))
{
    static bool Initialized = false;
    if (!Initialized) {
        qmlRegisterType<QMozReturnValue>("QtMozilla", 1, 0, "QMozReturnValue");
        Initialized = true;
    }

    setFlag(ItemHasContents, true);
    d->mContext = QMozContext::GetInstance();
    QTimer::singleShot(0, QMozContext::GetInstance(), SLOT(runEmbedding()));
    if (!d->mContext->initialized()) {
        connect(d->mContext, SIGNAL(onInitialized()), this, SLOT(onInitialized()));
    } else {
        QTimer::singleShot(0, this, SLOT(onInitialized()));
    }
}

QuickMozView::~QuickMozView()
{
    delete d;
}

void
QuickMozView::onInitialized()
{
    LOGT("QuickMozView");
    const QOpenGLContext* ctx = QOpenGLContext::currentContext();
    d->mContext->GetApp()->SetIsAccelerated(ctx && ctx->surface() && !getenv("SWRENDER"));
    d->mView = d->mContext->GetApp()->CreateView();
    d->mView->SetListener(d);
}

void QuickMozView::itemChange(ItemChange change, const ItemChangeData &)
{
    if (change == ItemSceneChange) {
        QQuickWindow *win = window();
        if (!win)
            return;
        connect(win, SIGNAL(beforeRendering()), this, SLOT(paint()), Qt::DirectConnection);
        connect(win, SIGNAL(sceneGraphInitialized()), this, SLOT(sceneGraphInitialized()), Qt::DirectConnection);
        win->setClearBeforeRendering(false);
    }
}

void QuickMozView::geometryChanged(const QRectF & newGeometry, const QRectF&)
{
    d->UpdateViewSize();
}

void QuickMozView::sceneGraphInitialized()
{
}

void QuickMozView::paint()
{
    if (d->mViewInitialized && d->mContext->GetApp()->IsAccelerated()) {
        if (!d->mViewGLSized) {
            d->UpdateViewSize(false);
            d->mViewGLSized = true;
        }
        QMatrix qmatr;
        qmatr.translate(x(), y());
        qmatr.rotate(rotation());
        qmatr.scale(scale(), scale());
        gfxMatrix matr(qmatr.m11(), qmatr.m12(), qmatr.m21(), qmatr.m22(), qmatr.dx(), qmatr.dy());
        d->mView->SetGLViewTransform(matr);
        d->mView->SetViewClipping(0, 0, boundingRect().width(), boundingRect().height());
        d->mView->RenderGL();
    }
}

QSGNode*
QuickMozView::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data)
{
    if (d->mViewInitialized && !d->mContext->GetApp()->IsAccelerated()) {
        QSGSimpleTextureNode *n = static_cast<QSGSimpleTextureNode*>(oldNode);
        if (!n) {
            n = new QSGSimpleTextureNode();
        }
        QRect r(boundingRect().toRect());
        if (d->mTempBufferImage.isNull() || d->mTempBufferImage.width() != r.width() || d->mTempBufferImage.height() != r.height()) {
            d->mTempBufferImage = QImage(r.size(), QImage::Format_RGB32);
        }
        d->mView->RenderToImage(d->mTempBufferImage.bits(), d->mTempBufferImage.width(),
                                d->mTempBufferImage.height(), d->mTempBufferImage.bytesPerLine(),
                                d->mTempBufferImage.depth());

        if (d->mTempTexture)
            delete d->mTempTexture;
        d->mTempTexture = window()->createTextureFromImage(d->mTempBufferImage);
        n->setTexture(d->mTempTexture);
        n->setRect(boundingRect());
        return n;
    }
    return nullptr;
}

void QuickMozView::cleanup()
{
}

QUrl QuickMozView::url() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return QUrl();
}

void QuickMozView::setUrl(const QUrl&)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

QString QuickMozView::title() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return QString();
}

int QuickMozView::loadProgress() const
{
    return 0;
}

bool QuickMozView::canGoBack() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return false;
}

bool QuickMozView::canGoForward() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return false;
}

bool QuickMozView::loading() const
{
    return false;
}

QRect QuickMozView::contentRect() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return QRect();
}

QSize QuickMozView::scrollableSize() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return QSize();
}

QPointF QuickMozView::scrollableOffset() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return QPointF();
}

float QuickMozView::resolution() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return 0.0;
}

bool QuickMozView::isPainted() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return false;
}

QColor QuickMozView::bgcolor() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return QColor();
}

bool QuickMozView::getUseQmlMouse()
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return false;
}

void QuickMozView::setUseQmlMouse(bool value)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::forceActiveFocus()
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::loadHtml(const QString& html, const QUrl& baseUrl)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::goBack()
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::goForward()
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::stop()
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::reload()
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::load(const QString&)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::sendAsyncMessage(const QString& name, const QVariant& variant)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::addMessageListener(const QString& name)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::addMessageListeners(const QStringList& messageNamesList)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::loadFrameScript(const QString& name)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::newWindow(const QString& url)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

quint32 QuickMozView::uniqueID() const
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
    return 0;
}

void QuickMozView::setParentID(unsigned aParentID)
{
    printf("QuickMozView::%s FIXME Not implemented: id:%i\n", __FUNCTION__, aParentID);
}

void QuickMozView::synthTouchBegin(const QVariant& touches)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::synthTouchMove(const QVariant& touches)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::synthTouchEnd(const QVariant& touches)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::scrollTo(const QPointF& position)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::suspendView()
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::resumeView()
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::recvMouseMove(int posX, int posY)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::recvMousePress(int posX, int posY)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}

void QuickMozView::recvMouseRelease(int posX, int posY)
{
    printf("QuickMozView::%s FIXME Not implemented\n", __FUNCTION__);
}
