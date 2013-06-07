/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QMOZ_EMBED_LOG_H
#define QMOZ_EMBED_LOG_H

#include <stdio.h>
#include <QDebug>

#ifdef Q_DEBUG_LOG

#ifdef LOG_COMPONENT
#define LOGT(FMT, ...) fprintf(stderr, \
                               "EmbedLiteExt %s:%s:%d: " FMT "\n", LOG_COMPONENT, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else // LOG_COMPONENT
#define LOGT(FMT, ...) fprintf(stderr, \
                               "EmbedLiteExt %s:%d: " FMT "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif // LOG_COMPONENT

#else // Q_DEBUG_LOG

#define LOGT(...) do {} while (0)

#endif // Q_DEBUG_LOG

#endif // QMOZ_EMBED_LOG_H
