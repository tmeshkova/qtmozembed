#ifndef QuickMozView_H
#define QuickMozView_H

#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLShaderProgram>

class QuickMozViewPrivate;
class QuickMozView : public QQuickItem
{
    Q_OBJECT

public:
    QuickMozView(QQuickItem *parent = 0);
    ~QuickMozView();

protected:
    void itemChange(ItemChange change, const ItemChangeData &);
    virtual void geometryChanged(const QRectF & newGeometry, const QRectF & oldGeometry);

    virtual QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data);

public Q_SLOTS:
    void paint();
    void cleanup();

private Q_SLOTS:
    void onInitialized();

private:
    QuickMozViewPrivate* d;
    friend class QuickMozViewPrivate;
};

#endif // QuickMozView_H
