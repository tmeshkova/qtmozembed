#ifndef QuickMozView_H
#define QuickMozView_H

#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLShaderProgram>

class QuickMozViewPrivate;
class QuickMozView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(unsigned parentid WRITE setParentID)
    Q_PROPERTY(QObject* child READ getChild NOTIFY childChanged)
public:
    QuickMozView(QQuickItem *parent = 0);
    ~QuickMozView();

private:
    QObject* getChild() { return this; }
    void setParentID(unsigned);

public Q_SLOTS:
    quint32 uniqueID() const;

Q_SIGNALS:
    void childChanged();

// INTERNAL
protected:
    void itemChange(ItemChange change, const ItemChangeData &);
    virtual void geometryChanged(const QRectF & newGeometry, const QRectF & oldGeometry);
    virtual QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data);

public Q_SLOTS:
    void paint();
    void sceneGraphInitialized();
    void cleanup();

private Q_SLOTS:
    void onInitialized();

private:
    QuickMozViewPrivate* d;
    friend class QuickMozViewPrivate;
};

#endif // QuickMozView_H
