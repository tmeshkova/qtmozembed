/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GECKOWORKER_H
#define GECKOWORKER_H

#include <QObject>

namespace mozilla {
namespace embedlite {
class EmbedLiteApp;
}}

/*!
 * An instance of this class runs in its own thread/event loop and is used to
 * host EmbedLiteApp's event loop.
 */
class GeckoWorker : public QObject
{
    Q_OBJECT

public:
    explicit GeckoWorker(mozilla::embedlite::EmbedLiteApp* aApp, QObject* parent = 0);

public Q_SLOTS:
    void doWork();
    void quit();

private:
    mozilla::embedlite::EmbedLiteApp* mApp;
};

#endif
