#include "quickmozview.h"

#include "mozilla-config.h"
#include "qmozcontext.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLog.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

#include <QTimer>
#include <QtOpenGL/QGLContext>
#include <QApplication>
#include <QtQuick/qquickwindow.h>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QSGSimpleRectNode>

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
    {
    }
    virtual ~QuickMozViewPrivate() {}

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
};

QuickMozView::QuickMozView(QQuickItem *parent)
  : QQuickItem(parent)
  , d(new QuickMozViewPrivate(this))
{
//    setFlag(ItemHasContents, true);
    d->mContext = QMozContext::GetInstance();
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
    d->mContext->GetApp()->SetIsAccelerated(true);
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
        win->setClearBeforeRendering(false);
    }
}


void QuickMozView::paint()
{
    if (d->mViewInitialized) {
        if (d->mContext->GetApp()->IsAccelerated()) {
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
}

void QuickMozView::geometryChanged(const QRectF & newGeometry, const QRectF&)
{
    d->UpdateViewSize();
}

QSGNode*
QuickMozView::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data)
{
    QSGSimpleRectNode *n = static_cast<QSGSimpleRectNode *>(oldNode);
    if (!n) {
        n = new QSGSimpleRectNode();
        n->setColor(Qt::green);
    }
    n->setRect(boundingRect());
    return n;
}

void QuickMozView::cleanup()
{
}
