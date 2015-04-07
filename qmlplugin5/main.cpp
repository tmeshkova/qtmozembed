/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtQml/QQmlExtensionPlugin>
#include <QtQml/qqml.h>
#include "quickmozview.h"
#include "qmozcontext.h"
#include "qmozscrolldecorator.h"
#include "qmlmozcontext.h"

class QtMozEmbedPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("Qt5Mozilla"));
        qmlRegisterType<QuickMozView>("Qt5Mozilla", 1, 0, "QmlMozView");
        qmlRegisterType<QmlMozContext>("Qt5Mozilla", 1, 0, "QmlMozContext");
        qmlRegisterUncreatableType<QMozScrollDecorator>("Qt5Mozilla", 1, 0, "QmlMozScrollDecorator", "");
        qmlRegisterUncreatableType<QMozReturnValue>("Qt5Mozilla", 1, 0, "QMozReturnValue", "");
        setenv("EMBED_COMPONENTS_PATH", DEFAULT_COMPONENTS_PATH, 1);
    }
};

#include "main.moc"
