/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmozviewsgnode_h
#define qmozviewsgnode_h

#include <QtQuick/QSGTransformNode>

QT_BEGIN_NAMESPACE
class QQuickItem;
class QSGSimpleRectNode;
QT_END_NAMESPACE

class MozContentSGNode;
class QGraphicsMozViewPrivate;
class QuickMozView;

class QMozViewSGNode : public QSGNode
{
public:
    QMozViewSGNode();
    void setRenderer(QuickMozView* aView);

private:
    MozContentSGNode* m_contentsNode;

Q_SIGNALS:
    void textureInUse();
    void pendingNewTexture();

public Q_SLOTS:
    // This function gets called on the FBO rendering thread and will store the
    // texture id and size and schedule an update on the window.
    void newTexture(int id, const QSize &size);

    // Before the scene graph starts to render, we update to the pending texture
    void prepareNode();
};

#endif /* qmozviewsgnode_h */
