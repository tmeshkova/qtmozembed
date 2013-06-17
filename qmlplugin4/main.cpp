#include <QDeclarativeExtensionPlugin>
#include <qdeclarative.h>

#include <QStringList>
#include <QVector>

#include "qmlmozcontext.h"
#include "qgraphicsmozview.h"
#include "qdeclarativemozview.h"

class QtMozEmbedPlugin : public QDeclarativeExtensionPlugin
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Q_PLUGIN_METADATA(IID "org.qt-project.foo" FILE "myplugindescription.json")
#endif
public:
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtMozilla"));

        qmlRegisterType<QmlMozContext>("QtMozilla", 1, 0, "QmlMozContext");
        qmlRegisterType<QGraphicsMozView>("QtMozilla", 1, 0, "QGraphicsMozView");
        qmlRegisterType<QDeclarativeMozView>("QtMozilla", 1, 0, "QmlMozView");

        setenv("EMBED_COMPONENTS_PATH", DEFAULT_COMPONENTS_PATH, 1);
    }
};

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
Q_EXPORT_PLUGIN2(qtmozembedplugin, QtMozEmbedPlugin);
#endif
