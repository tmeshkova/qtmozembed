/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qgraphicsmozview_h
#define qgraphicsmozview_h

#include <QGraphicsWidget>
#include <QUrl>
#include "qmozview_defined_wrapper.h"

class QMozContext;
class QSyncMessage;
class QGraphicsMozViewPrivate;

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#undef Q_SIGNALS
#define Q_SIGNALS public
#endif

class QGraphicsMozView : public QGraphicsWidget
{
    Q_OBJECT

    Q_MOZ_VIEW_PRORERTIES

public:
    QGraphicsMozView(QGraphicsItem* parent = 0);
    virtual ~QGraphicsMozView();

    Q_MOZ_VIEW_PUBLIC_METHODS

public Q_SLOTS:
    void loadHtml(const QString& html, const QUrl& baseUrl = QUrl());
    void goBack();
    void goForward();
    void stop();
    void reload();
    void load(const QString&);
    void sendAsyncMessage(const QString& name, const QVariant& variant);
    void addMessageListener(const QString& name);
    void addMessageListeners(const QStringList& messageNamesList);
    void loadFrameScript(const QString& name);
    void newWindow(const QString& url = "about:blank");
    quint32 uniqueID() const;
    void setParentID(unsigned aParentID);
    void synthTouchBegin(const QVariant& touches);
    void synthTouchMove(const QVariant& touches);
    void synthTouchEnd(const QVariant& touches);
    void scrollTo(const QPointF& position);
    void suspendView();
    void resumeView();
    void recvMouseMove(int posX, int posY);
    void recvMousePress(int posX, int posY);
    void recvMouseRelease(int posX, int posY);

Q_SIGNALS:
    void viewInitialized();
    void urlChanged();
    void titleChanged();
    void loadProgressChanged();
    void navigationHistoryChanged();
    void loadingChanged();
    void viewDestroyed();
    void recvAsyncMessage(const QString message, const QVariant data);
    bool recvSyncMessage(const QString message, const QVariant data, QMozReturnValue* response);
    void loadRedirect();
    void securityChanged(QString status, uint state);
    void firstPaint(int offx, int offy);
    void contentLoaded(QString docuri);
    void viewAreaChanged();
    void handleLongTap(QPoint point, QMozReturnValue* retval);
    void handleSingleTap(QPoint point, QMozReturnValue* retval);
    void handleDoubleTap(QPoint point, QMozReturnValue* retval);
    void imeNotification(int state, bool open, int cause, int focusChange, const QString& type);
    void bgColorChanged();
    void requestGLContext(bool& hasContext, QSize& viewPortSize);
    void useQmlMouse(bool value);

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
    unsigned mParentID;

    bool mUseQmlMouse;
};

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#undef Q_SIGNALS
#define Q_SIGNALS protected
#endif

#endif /* qgraphicsmozview_h */
