/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-*/
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmozviewcreator_h
#define qmozviewcreator_h

#include <QObject>
#include <QString>

/*
 * QMozViewCreator abstract class defines interface for a view creator implemented in UI code.
 */
class QMozViewCreator : public QObject
{
    Q_OBJECT

public:
    explicit QMozViewCreator(QObject* parent = 0) : QObject(parent) {};
    virtual ~QMozViewCreator() {};

    /*
     * @returns ID of created web view
     */
    virtual quint32 createView(const QString &url, const quint32 &parentId) = 0;
};

#endif
