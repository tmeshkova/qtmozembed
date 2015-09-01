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

class QMozViewPrivate;
class QMozWindow;

class QuickMozView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int parentId READ parentId WRITE setParentID NOTIFY parentIdChanged FINAL)
    Q_PROPERTY(bool privateMode READ privateMode WRITE setPrivateMode NOTIFY privateModeChanged FINAL)
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
    void compositingFinished();

// INTERNAL
protected:
    void itemChange(ItemChange change, const ItemChangeData &) override;
    void geometryChanged(const QRectF & newGeometry, const QRectF & oldGeometry) override;
    QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery property) const override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void touchEvent(QTouchEvent*) override;
    void timerEvent(QTimerEvent*) override;
    void componentComplete() override;

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

    QMozViewPrivate* d;
    friend class QMozViewPrivate;
    unsigned mParentID;
    bool mPrivateMode;
    bool mUseQmlMouse;
    bool mActive;
    bool mBackground;
    bool mLoaded;
    GLuint mConsTex;
    QMutex mRenderMutex;
};

#endif // QuickMozView_H
