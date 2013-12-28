/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmozviewsgnode_h
#define qmozviewsgnode_h

#include <QtQuick/QSGNode>

class MozContentSGNode;
class QGraphicsMozViewPrivate;
class QuickMozView;

class QMozViewSGNode : public QSGNode {
public:
    QMozViewSGNode();
    void setRenderer(QGraphicsMozViewPrivate* renderer, QuickMozView* aView);

private:
    MozContentSGNode* m_contentsNode;
};

#endif /* qmozviewsgnode_h */
