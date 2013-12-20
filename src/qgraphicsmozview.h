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

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY navigationHistoryChanged FINAL)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY navigationHistoryChanged FINAL)
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)
    Q_PROPERTY(QRectF contentRect READ contentRect NOTIFY viewAreaChanged FINAL)
    Q_PROPERTY(QSizeF scrollableSize READ scrollableSize)
    Q_PROPERTY(QPointF scrollableOffset READ scrollableOffset)
    Q_PROPERTY(float resolution READ resolution)
    Q_PROPERTY(bool painted READ isPainted NOTIFY firstPaint FINAL)
    Q_PROPERTY(QColor bgcolor READ bgcolor NOTIFY bgColorChanged FINAL)
    Q_PROPERTY(bool useQmlMouse READ getUseQmlMouse WRITE setUseQmlMouse)

public:
    QGraphicsMozView(QGraphicsItem* parent = 0);
    virtual ~QGraphicsMozView();
    void startMoveMonitoring();

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
    bool Invalidate();
    void CompositingFinished();
    void requestGLContext(bool& hasContext, QSize& viewPortSize);

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
    void useQmlMouse(bool value);
    void updateThreaded();
    void requestGLContextQGV(bool& hasContext, QSize& viewPortSize);
    void contentWidthChanged();
    void contentHeightChanged();
    void scrollableOffsetChanged();
    void draggingChanged();
    void movingChanged();
    void chromeChanged();
    void pinchingChanged();

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
    virtual void focusInEvent(QFocusEvent*);
    virtual void focusOutEvent(QFocusEvent*);

private Q_SLOTS:
    void onInitialized();
    void OnUpdateThreaded();

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
