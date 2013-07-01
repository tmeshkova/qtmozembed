/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozviewsgnode.h"

#include <QtGui/QPolygonF>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGSimpleRectNode>
#include <private/qsgrendernode_p.h>
#include "qgraphicsmozview_p.h"
#include <assert.h>

class MozContentSGNode : public QSGRenderNode {
public:
    MozContentSGNode(QGraphicsMozViewPrivate* aPrivate)
        : mPrivate(aPrivate)
    {
        mPrivate->mView->SetIsActive(true);
    }

    virtual StateFlags changedStates()
    {
        return StateFlags(StencilState) | ColorState | BlendState;
    }

    virtual void render(const RenderState& state)
    {
        QMatrix affine = matrix() ? (*matrix()).toAffine() : QMatrix();
        gfxMatrix matr(affine.m11(), affine.m12(), affine.m21(), affine.m22(), affine.dx(), affine.dy());
        mPrivate->mView->SetGLViewTransform(matr);
        if (state.scissorEnabled) {
            mPrivate->mView->SetViewClipping(state.scissorRect.x(),
                                               state.scissorRect.x(),
                                               state.scissorRect.width(),
                                               state.scissorRect.height());
        }
        mPrivate->mView->RenderGL();
    }

    ~MozContentSGNode()
    {
    }

    const QMozViewSGNode* pageNode() const
    {
        const QMozViewSGNode* parent = static_cast<QMozViewSGNode*>(this->parent());
        assert(parent);
        return parent;
    }

    QGraphicsMozViewPrivate* getPrivate() { return mPrivate; }

private:
    QRectF clipRect() const
    {
        // Start with an invalid rect.
        QRectF resultRect(0, 0, -1, -1);

        for (const QSGClipNode* clip = clipList(); clip; clip = clip->clipList()) {
            QMatrix4x4 clipMatrix = *clip->matrix();

            QRectF currentClip;

            if (clip->isRectangular())
                currentClip = clipMatrix.mapRect(clip->clipRect());
            else {
                const QSGGeometry* geometry = clip->geometry();
                // Assume here that clipNode has only coordinate data.
                const QSGGeometry::Point2D* geometryPoints = geometry->vertexDataAsPoint2D();

                // Clip region should be at least triangle to make valid clip.
                if (geometry->vertexCount() < 3)
                    continue;

                QPolygonF polygon;

                for (int i = 0; i < geometry->vertexCount(); i++)
                    polygon.append(clipMatrix.map(QPointF(geometryPoints[i].x, geometryPoints[i].y)));
                currentClip = polygon.boundingRect();
            }

            if (currentClip.isEmpty())
                continue;

            if (resultRect.isValid())
                resultRect &= currentClip;
            else
                resultRect = currentClip;
        }

        return resultRect;
    }

    QGraphicsMozViewPrivate* mPrivate;
};

QMozViewSGNode::QMozViewSGNode()
    : m_contentsNode(0)
    , m_backgroundNode(new QSGSimpleRectNode)
{
    appendChildNode(m_backgroundNode);
}

void QMozViewSGNode::setBackground(const QRectF& rect, const QColor& color)
{
    m_backgroundNode->setRect(rect);
    m_backgroundNode->setColor(color);
}

void QMozViewSGNode::setRenderer(QGraphicsMozViewPrivate* aPrivate)
{
    if (m_contentsNode && m_contentsNode->getPrivate() == aPrivate)
        return;

    delete m_contentsNode;
    m_contentsNode = new MozContentSGNode(aPrivate);
    // This sets the parent node of the content to QMozViewSGNode.
    appendChildNode(m_contentsNode);
}
