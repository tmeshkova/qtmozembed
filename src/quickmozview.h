/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QuickMozView_H
#define QuickMozView_H

#include <QMatrix>
#include <QMutex>
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLShaderProgram>
#include "qmozview_defined_wrapper.h"

class QGraphicsMozViewPrivate;
class QuickMozView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int parentId READ parentId WRITE setParentID NOTIFY parentIdChanged FINAL)
    Q_PROPERTY(bool privateMode READ privateMode WRITE setPrivateMode NOTIFY privateModeChanged FINAL)
    Q_PROPERTY(unsigned parentid WRITE setParentID)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool background READ background NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(QObject* child READ getChild NOTIFY childChanged)

    Q_MOZ_VIEW_PRORERTIES

public:
    QuickMozView(QQuickItem *parent = 0);
    ~QuickMozView();

    Q_MOZ_VIEW_PUBLIC_METHODS
    void RenderToCurrentContext();

    int parentId() const;
    bool privateMode() const;

    bool active() const;
    void setActive(bool active);
    void setPrivateMode(bool);

    bool background() const;
    bool loaded() const;

private:
    QObject* getChild() { return this; }
    void updateGLContextInfo();

public Q_SLOTS:
    Q_MOZ_VIEW_PUBLIC_SLOTS

Q_SIGNALS:
    void childChanged();
    void setIsActive(bool);
    void dispatchItemUpdate();
    void textureReady(int id, const QSize &size);
    void parentIdChanged();
    void privateModeChanged();
    void activeChanged();
    void backgroundChanged();
    void loadedChanged();
    void updateViewSize();

    Q_MOZ_VIEW_SIGNALS

private Q_SLOTS:
    void processViewInitialization();
    void SetIsActive(bool aIsActive);
    void updateLoaded();
    void resumeRendering();

// INTERNAL
protected:
    void itemChange(ItemChange change, const ItemChangeData &);
    virtual void geometryChanged(const QRectF & newGeometry, const QRectF & oldGeometry);
    virtual QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery property) const;
    virtual void inputMethodEvent(QInputMethodEvent* event);
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);
    virtual void focusInEvent(QFocusEvent*);
    virtual void focusOutEvent(QFocusEvent*);
    virtual void touchEvent(QTouchEvent*);
    virtual void timerEvent(QTimerEvent*);
    virtual void componentComplete();

public Q_SLOTS:
    void cleanup();
    void setInputMethodHints(Qt::InputMethodHints hints);
    void updateGLContextInfo(QOpenGLContext*);

private Q_SLOTS:
    void createThreadRenderObject();
    void clearThreadRenderObject();
    void contextInitialized();
    void updateEnabled();
    void refreshNodeTexture();
    void windowVisibleChanged(bool visible);

private:
    void createView();

    QGraphicsMozViewPrivate* d;
    friend class QGraphicsMozViewPrivate;
    unsigned mParentID;
    bool mPrivateMode;
    bool mUseQmlMouse;
    bool mPreedit;
    bool mActive;
    bool mBackground;
    bool mLoaded;
    GLuint mConsTex;
    QMutex mRenderMutex;
};

#endif // QuickMozView_H
