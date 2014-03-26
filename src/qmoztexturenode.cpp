/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmoztexturenode.h"
#include "quickmozview.h"
#include <QQuickWindow>
#include <QThread>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

QSGMozTexture::QSGMozTexture()
    : QSGTexture()
    , m_texture_id(0)
    , m_texture_target(GL_TEXTURE_2D)
{
}

QSGMozTexture::~QSGMozTexture()
{
}

void QSGMozTexture::setTexture(int id, int target)
{
    m_texture_id = id;
    m_texture_target = target;
}

void QSGMozTexture::bind()
{
    glBindTexture(m_texture_target, m_texture_id);
}

MozTextureNode::MozTextureNode(QuickMozView* aView)
  : m_id(0)
  , m_size(0, 0)
  , m_texture(0)
  , m_view(aView)
  , mIsConnected(false)
{
    // Our texture node must have a texture, so use the default 0 texture.
    m_texture = m_view->window()->createTextureFromId(0, QSize(1, 1));
    setTexture(m_texture);
    setFiltering(QSGTexture::Linear);
}

void
MozTextureNode::newTexture(int id, const QSize &size)
{
    m_mutex.lock();
    m_id = id;
    m_size = size;
    m_mutex.unlock();

    // We cannot call QQuickWindow::update directly here, as this is only allowed
    // from the rendering thread or GUI thread.
    Q_EMIT pendingNewTexture();
}

#define LOCAL_GL_TEXTURE_EXTERNAL 0x8D65
// Before the scene graph starts to render, we update to the pending texture
void
MozTextureNode::prepareNode()
{
    m_mutex.lock();
    int newId = m_id;
    QSize size = m_size;
    m_id = 0;
    m_mutex.unlock();
    if (newId) {
        delete m_texture;
        QSGMozTexture* texture = new QSGMozTexture();
        texture->setTexture(newId, LOCAL_GL_TEXTURE_EXTERNAL);
        texture->setTextureSize(size);
        m_texture = texture;
        setTexture(m_texture);
    }
}
