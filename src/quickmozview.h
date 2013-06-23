#ifndef QuickMozView_H
#define QuickMozView_H

#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLShaderProgram>
#include "qmozview_defined_wrapper.h"

class QGraphicsMozViewPrivate;

class QuickMozView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(unsigned parentid WRITE setParentID)
    Q_PROPERTY(QObject* child READ getChild NOTIFY childChanged)

    Q_MOZ_VIEW_PRORERTIES

public:
    QuickMozView(QQuickItem *parent = 0);
    ~QuickMozView();

    Q_MOZ_VIEW_PUBLIC_METHODS

private:
    QObject* getChild() { return this; }

public Q_SLOTS:

    Q_MOZ_VIEW_PUBLIC_SLOTS

Q_SIGNALS:
    void childChanged();

    Q_MOZ_VIEW_SIGNALS

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

public Q_SLOTS:
    void paint();
    void sceneGraphInitialized();
    void cleanup();
    void setInputMethodHints(Qt::InputMethodHints hints);
    void onRequestGLContext(bool& hasContext, QSize& viewPortSize);

private Q_SLOTS:
    void onInitialized();

private:
    QGraphicsMozViewPrivate* d;
    friend class QGraphicsMozViewPrivate;
    unsigned mParentID;
    bool mUseQmlMouse;
};

#endif // QuickMozView_H
