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

class QMozViewSGNode : public QSGTransformNode {
    public:
        QMozViewSGNode();
        void setBackground(const QRectF&, const QColor&);
        void setRenderer(QGraphicsMozViewPrivate* renderer);

    private:
        MozContentSGNode* m_contentsNode;
        QSGSimpleRectNode* m_backgroundNode;
};

#endif /* qmozviewsgnode_h */
