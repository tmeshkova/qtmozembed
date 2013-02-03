/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qdeclarativemozview_h
#define qdeclarativemozview_h

#include <QtDeclarative/QDeclarativeItem>
#include "qgraphicsmozview.h"

class QMozContext;
class QDeclarativeMozViewPrivate;

class QDeclarativeMozView : public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY(int preferredWidth READ preferredWidth WRITE setPreferredWidth NOTIFY preferredWidthChanged)
    Q_PROPERTY(int preferredHeight READ preferredHeight WRITE setPreferredHeight NOTIFY preferredHeightChanged)
public:
    QDeclarativeMozView(QDeclarativeItem *parent = 0);

    virtual ~QDeclarativeMozView();

    int preferredWidth() const;
    void setPreferredWidth(int);
    int preferredHeight() const;
    void setPreferredHeight(int);
    Q_INVOKABLE QObject* child() const;

Q_SIGNALS:
    void preferredWidthChanged();
    void preferredHeightChanged();
    void childChanged();

private Q_SLOTS:
    void updateDeclarativeMozViewSize();
    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);

private:
    void init();
    void updateContentsSize();

private:
    QDeclarativeMozViewPrivate* d;
    friend class QDeclarativeMozViewPrivate;
};

class GraphicsMozView : public QGraphicsMozView {
    Q_OBJECT
public:
    GraphicsMozView(QDeclarativeMozView* parent = 0);

private:
    QDeclarativeMozView *parent;
    friend class QDeclarativeMozView;
    Q_DISABLE_COPY(GraphicsMozView)
};

QML_DECLARE_TYPE(QDeclarativeMozView)

#endif /* qdeclarativemozview_h */
