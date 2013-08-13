/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozviewtexsgnode.h"

QMozViewTexSGNode::QMozViewTexSGNode(QuickMozView *aView)
  : m_id(0)
  , m_size(0, 0)
  , m_texture(0)
  , m_window(aView->window())
{
    // Our texture node must have a texture, so use the default 0 texture.
    m_texture = m_window->createTextureFromId(0, QSize(1, 1));
    setTexture(m_texture);
    setFiltering(QSGTexture::Linear);
}

QMozViewTexSGNode::~QMozViewTexSGNode()
{
    delete m_texture;
}

void
QMozViewTexSGNode::newTexture(int id, const QSize &size)
{
    m_mutex.lock();
    m_id = id;
    m_size = size;
    m_mutex.unlock();
}

// Before the scene graph starts to render, we update to the pending texture
void
QMozViewTexSGNode::prepareNode()
{
    m_mutex.lock();
    int newId = m_id;
    QSize size = m_size;
    m_id = 0;
    m_mutex.unlock();
    if (newId) {
        delete m_texture;
        m_texture = m_window->createTextureFromId(newId, size);
        setTexture(m_texture);
    }
}
