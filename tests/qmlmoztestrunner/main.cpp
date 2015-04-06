/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmozcontext.h"
#include "testviewcreator.h"
#include "qtestrunner.h"
#include <QGuiApplication>
#include <QtCore/qstring.h>
#include <QTimer>
#include <QtQml>
#include <stdio.h>
#include <QQuickView>

int main(int argc, char **argv)
{
    int retv = 0;
    {
        QGuiApplication app(argc, argv);
        {
            bool isOpenGL = false;
            for (int index = 1; index < argc; ++index) {
                if (strcmp(argv[index], "-opengl") == 0) {
                    isOpenGL = true;
                    break;
                }
            }

            qmlRegisterType<TestViewCreator>("qtmozembed.tests", 1, 0, "WebViewCreator");

            QTestRunner runn(isOpenGL, argc, argv);
            QTimer::singleShot(0, &runn, SLOT(DropInStartup()));
            // These components must be loaded before app start
            QString componentPath(DEFAULT_COMPONENTS_PATH);
            QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components") + QString("/EmbedLiteBinComponents.manifest"));
            QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome") + QString("/EmbedLiteJSScripts.manifest"));
            QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome") + QString("/EmbedLiteOverrides.manifest"));
            QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components") + QString("/EmbedLiteJSComponents.manifest"));
            QMozContext::GetInstance()->runEmbedding();

            retv = runn.GetResult();
        }
        app.quit();
    }
    return retv;
}
