#include <QDeclarativeExtensionPlugin>
#include <qdeclarative.h>

#include <QtDebug>
#include <QStringList>
#include <QVector>

#include "qmlmozcontext.h"
#include "qgraphicsmozview.h"
#include "qdeclarativemozview.h"

class QtMozEmbedPlugin : public QDeclarativeExtensionPlugin
{
public:
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtMozilla"));

        qmlRegisterType<QmlMozContext>("QtMozilla", 1, 0, "QmlMozContext");
        qmlRegisterType<QGraphicsMozView>("QtMozilla", 1, 0, "QGraphicsMozView");
        qmlRegisterType<QDeclarativeMozView>("QtMozilla", 1, 0, "QmlMozView");
    }
};

Q_EXPORT_PLUGIN2(qtmozembedplugin, QtMozEmbedPlugin);
