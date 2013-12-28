/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmoztexturenode_h
#define qmoztexturenode_h

#include <QtQuick/QSGSimpleTextureNode>
#include <QObject>
#include <QMutex>

class QuickMozView;

class MozTextureNode : public QObject, public QSGSimpleTextureNode
{
    Q_OBJECT
public:
    MozTextureNode(QuickMozView* aView);

    ~MozTextureNode()
    {
        delete m_texture;
    }

Q_SIGNALS:
    void pendingNewTexture();

public Q_SLOTS:

    // This function gets called on the FBO rendering thread and will store the
    // texture id and size and schedule an update on the window.
    void newTexture(int id, const QSize &size);

    // Before the scene graph starts to render, we update to the pending texture
    void prepareNode();

private:

    int m_id;
    QSize m_size;
    QMutex m_mutex;
    QSGTexture *m_texture;
    QuickMozView *m_view;
    bool mIsConnected;
};

#endif /* qmoztexturenode_h */
