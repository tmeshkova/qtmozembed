/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef testviewcreator_h
#define testviewcreator_h

#include "qmozviewcreator.h"

class TestViewCreator : public QMozViewCreator
{
    Q_OBJECT

public:
    explicit TestViewCreator(QObject* parent = 0);
    ~TestViewCreator() {}

    virtual quint32 createView(const QString &url, const quint32 &parentId);

signals:
    void newWindowRequested(const QString &url, const quint32 &parentId);
};

#endif
