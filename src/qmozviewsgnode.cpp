/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozviewsgnode.h"

#include <QtGui/QPolygonF>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGSimpleRectNode>
#include <QMutex>
#include <QThread>
#include <private/qsgrendernode_p.h>
#include "qgraphicsmozview_p.h"
#include "quickmozview.h"
#include <assert.h>

static const char *vertShader =
    "attribute vec4 vPosition;\n"
    "attribute vec2 vTexCoord;\n"
    "varying   vec2 tex_coord;\n"
    "void main()\n"
    "{\n"
    "  gl_Position = vPosition;\n"
    "  tex_coord = vTexCoord;\n"
    "}\n";

static const char *fragShader =
    "precision mediump float;\n"
    "varying vec2      tex_coord;\n"
    "uniform sampler2D tx;\n"
    "void main()\n"
    "{\n"
    "  gl_FragColor = texture2D (tx, tex_coord);\n"
    "}\n";

static GLfloat quad_vrt[] =
{
    /* front */
    -1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,

    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
};

static GLfloat quad_txc[] =
{
    /* front */
     0.0f,  0.0f,
     1.0f,  1.0f,
     0.0f,  1.0f,

     0.0f,  0.0f,
     1.0f,  0.0f,
     1.0f,  1.0f,
};

class MozContentSGNode : public QSGRenderNode {
public:
    MozContentSGNode(QuickMozView* aView)
        : mView(aView)
        , m_id(0)
        , m_size(0, 0)
        , m_texture(0)
        , m_window(aView->window())
        , m_program(0)
    {
        mView->SetIsActive(true);
        if (QThread::currentThread() != aView->thread()) {
            m_texture = m_window->createTextureFromId(0, QSize(1, 1));
            initializeShaders();
        }
    }

    void initializeShaders()
    {
        m_program = new QOpenGLShaderProgram;
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertShader);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragShader);
        m_program->bindAttributeLocation("vPosition", 0);
        m_program->bindAttributeLocation("vTexCoord", 1);
        m_program->link();
    }

    virtual StateFlags changedStates()
    {
        return StateFlags(StencilState) | ColorState | BlendState;
    }

    virtual void render(const RenderState& state)
    {
        QMatrix affine = matrix() ? (*matrix()).toAffine() : QMatrix();
        mView->RenderToCurrentContext(affine);

        if (m_program && m_texture) {
            m_program->bind();
            glBindTexture(GL_TEXTURE_2D, m_texture->textureId());
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, quad_vrt);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quad_txc);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            m_program->release();
        }
    }

    ~MozContentSGNode()
    {
        delete m_texture;
        delete m_program;
    }

    const QMozViewSGNode* pageNode() const
    {
        const QMozViewSGNode* parent = static_cast<QMozViewSGNode*>(this->parent());
        assert(parent);
        return parent;
    }

    QuickMozView* getView() { return mView; }

    void newTexture(int id, const QSize &size)
    {
        m_mutex.lock();
        m_id = id;
        m_size = size;
        m_mutex.unlock();
    }

    // Before the scene graph starts to render, we update to the pending texture
    void prepareNode()
    {
        m_mutex.lock();
        int newId = m_id;
        QSize size = m_size;
        m_id = 0;
        m_mutex.unlock();
        if (newId) {
            delete m_texture;
            m_texture = m_window->createTextureFromId(newId, size);
        }
    }

private:
    QuickMozView* mView;

    int m_id;
    QSize m_size;
    QMutex m_mutex;
    QSGTexture *m_texture;
    QQuickWindow *m_window;
    QOpenGLShaderProgram *m_program;
};

QMozViewSGNode::QMozViewSGNode()
    : m_contentsNode(0)
{
}

void QMozViewSGNode::setRenderer(QuickMozView* aView)
{
    if (m_contentsNode && m_contentsNode->getView() == aView)
        return;

    if (m_contentsNode) {
        removeChildNode(m_contentsNode);
        delete m_contentsNode;
    }
    m_contentsNode = new MozContentSGNode(aView);
    // This sets the parent node of the content to QMozViewSGNode.
    appendChildNode(m_contentsNode);
}

void
QMozViewSGNode::newTexture(int id, const QSize &size)
{
    m_contentsNode->newTexture(id, size);
}

// Before the scene graph starts to render, we update to the pending texture
void
QMozViewSGNode::prepareNode()
{
    m_contentsNode->prepareNode();
}
