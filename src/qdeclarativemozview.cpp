/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QDeclarativeMozView"

#include <QSharedPointer>
#include <QGLContext>
#include <QtDeclarative/QDeclarativeView>

#include "mozilla-config.h"
#include "qdeclarativemozview.h"
#include "qgraphicsmozview.h"
#include "qmozembedlog.h"

class QDeclarativeMozViewPrivate {
public:
    QDeclarativeMozViewPrivate(QDeclarativeMozView* qq)
      : q(qq)
      , preferredwidth(0)
      , preferredheight(0)
    {
    }

    QDeclarativeMozView* q;
    QSharedPointer<GraphicsMozView> view;
    int preferredwidth, preferredheight;
};

QDeclarativeMozView::QDeclarativeMozView(QDeclarativeItem* parent)
    : QDeclarativeItem(parent)
    , d(new QDeclarativeMozViewPrivate(this))
{
    init();
}

QDeclarativeMozView::~QDeclarativeMozView()
{
    delete d;
}

void
QDeclarativeMozView::setParentID(unsigned aParentID)
{
    if (d->view) {
        d->view->setParentID(aParentID);
    }
}

GraphicsMozView::GraphicsMozView(QDeclarativeMozView* parent)
    : QGraphicsMozView(parent)
    , parent(parent)
{
}

void
QDeclarativeMozView::init()
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QGraphicsItem::ItemHasNoContents, true);
    setFlag(QGraphicsItem::ItemIsFocusScope, true);
    setClip(true);

    d->view = QSharedPointer<GraphicsMozView>(new GraphicsMozView(this));
    connect(d->view.data(), SIGNAL(requestGLContextQGV(bool&,QSize&)), this, SLOT(onRequestGLContext(bool&,QSize&)));
    d->view->setFocus();
    if (!preferredWidth())
        setPreferredWidth(d->view->preferredWidth());
    if (!preferredHeight())
        setPreferredHeight(d->view->preferredHeight());
}

static bool viewHasGLContext(QGraphicsView* view)
{
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

void
QDeclarativeMozView::onRequestGLContext(bool& hasContext, QSize& viewPortSize)
{
    hasContext = false;
    if (!scene()) {
        return;
    }
    QList<QGraphicsView *> views = scene()->views();
    for (int i = 0; i < views.length(); ++i) {
        QDeclarativeView *view = qobject_cast<QDeclarativeView *>(views[i]);
        if (view && viewHasGLContext(view)) {
            const QGLContext* ctx = QGLContext::currentContext();
            if (ctx && ctx->device()) {
                QRectF r(0, 0, ctx->device()->width(), ctx->device()->height());
                r = mapRectToScene(r);
                viewPortSize = r.size().toSize();
            }
            hasContext = true;
            break;
        }
    }
}

/*!
    \qmlproperty int MozView::preferredWidth
    This property holds the ideal width for displaying the current URL.
*/
int QDeclarativeMozView::preferredWidth() const
{
    return d->preferredwidth;
}

void QDeclarativeMozView::setPreferredWidth(int width)
{
    if (d->preferredwidth == width)
        return;
    d->preferredwidth = width;
    updateContentsSize();
    Q_EMIT preferredWidthChanged();
}

/*!
    \qmlproperty int MozView::preferredHeight
    This property holds the ideal height for displaying the current URL.
    This only affects the area zoomed by heuristicZoom().
*/
int QDeclarativeMozView::preferredHeight() const
{
    return d->preferredheight;
}

void QDeclarativeMozView::setPreferredHeight(int height)
{
    if (d->preferredheight == height)
        return;
    d->preferredheight = height;
    updateContentsSize();
    Q_EMIT preferredHeightChanged();
}

void QDeclarativeMozView::updateContentsSize()
{
    QSize sz(d->preferredwidth>0 ? d->preferredwidth : width(),
             d->preferredheight>0 ? d->preferredheight : height());
    LOGT("sz:[%i,%i]", sz.width(), sz.height());
}

void QDeclarativeMozView::updateDeclarativeMozViewSize()
{
    QSizeF size = d->view->geometry().size();
    setImplicitWidth(size.width());
    setImplicitHeight(size.height());
}

void QDeclarativeMozView::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    d->view->resize(newGeometry.size());
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);
}

QObject*
QDeclarativeMozView::getChild() const
{
    return d->view.data();
}

quint32
QDeclarativeMozView::uniqueID() const
{
    LOGT();
    return d->view->uniqueID();
}
