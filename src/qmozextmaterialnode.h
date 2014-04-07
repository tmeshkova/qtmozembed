/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qMozExtMaterialNode_h
#define qMozExtMaterialNode_h

#include <QtQuick/QSGGeometryNode>
#include <QObject>
#include <QMutex>

class QuickMozView;

class MozExtMaterialNode : public QObject, public QSGGeometryNode
{
    Q_OBJECT
public:
    MozExtMaterialNode(QuickMozView* aView);

    ~MozExtMaterialNode()
    {
    }

Q_SIGNALS:
    void pendingNewTexture();

public Q_SLOTS:

    // This function gets called on the FBO rendering thread and will store the
    // texture id and size and schedule an update on the window.
    void newTexture(int id, const QSize &size);

    void setRect(const QRectF &rect);

    // Before the scene graph starts to render, we update to the pending texture
    void prepareNode();

private:

    int m_id;
    QMutex m_mutex;
    QuickMozView *m_view;
    bool mIsConnected;
};

#endif /* qMozExtMaterialNode_h */
