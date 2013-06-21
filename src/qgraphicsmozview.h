/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qgraphicsmozview_h
#define qgraphicsmozview_h

#include <QGraphicsWidget>
#include "qmozview_defined_wrapper.h"

class QMozContext;
class QSyncMessage;
class QGraphicsMozViewPrivate;


class QGraphicsMozView : public QGraphicsWidget
{
    Q_OBJECT

    Q_MOZ_VIEW_PRORERTIES

public:
    QGraphicsMozView(QGraphicsItem* parent = 0);
    virtual ~QGraphicsMozView();

    Q_MOZ_VIEW_PUBLIC_METHODS

public Q_SLOTS:

    Q_MOZ_VIEW_PUBLIC_SLOTS

Q_SIGNALS:

    Q_MOZ_VIEW_SIGNALS

protected:
    virtual void setGeometry(const QRectF& rect);
    virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint) const;

protected:
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* widget = 0);
    virtual bool event(QEvent*);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);
    virtual void inputMethodEvent(QInputMethodEvent*);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery aQuery) const;

private Q_SLOTS:
    void onInitialized();

private:
    QGraphicsMozViewPrivate* d;
    friend class QGraphicsMozViewPrivate;
    unsigned mParentID;

    bool mUseQmlMouse;
};

#endif /* qgraphicsmozview_h */
