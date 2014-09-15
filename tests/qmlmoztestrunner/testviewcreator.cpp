/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozcontext.h"
#include "testviewcreator.h"

TestViewCreator::TestViewCreator(QObject* parent)
  : QMozViewCreator(parent)
{
    QMozContext::GetInstance()->setViewCreator(this);
};

quint32 TestViewCreator::createView(const QString &url, const quint32 &parentId)
{
    emit newWindowRequested(url, parentId);
    // NB: actual new ID is supposed to be set synchronously in the slot handling newWindowRequested signal
    return 2;
};
