/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmozcontext_h
#define qmozcontext_h

#include <QObject>
#include <QThread>
#include <QClipboard>

class QMozContextPrivate;

namespace mozilla {
namespace embedlite {
class EmbedLiteApp;
}}

class QEventLoop;
class QMozContext;
class GeckoThread : public QThread
{
    Q_OBJECT
public:
    GeckoThread(QMozContext* aContext) : mContext(aContext), mEventLoop(NULL) {}
    void run();

public Q_SLOTS:
    void Quit();

public:
    QMozContext* mContext;
    QEventLoop* mEventLoop;
};

class QMozContext : public QObject
{
    Q_OBJECT
public:
    QMozContext(QObject* parent = 0);
    virtual ~QMozContext();

    bool initialized();
    mozilla::embedlite::EmbedLiteApp* GetApp();

    static QMozContext* GetInstance();

Q_SIGNALS:
    void onInitialized();
    void newWindowRequested(const QString& url);

public Q_SLOTS:
    void addComponentManifest(const QString& manifestPath);
    void addObserver(const QString& aTopic);
    void setClipboard(QString text);
    QString getClipboard();
    void newWindow(const QString& url);

private Q_SLOTS:
    void runEmbedding();
    void onLastWindowClosed();

private:
    QMozContextPrivate* d;
    friend class QMozContextPrivate;
    QClipboard* clipboard;
};

class QmlMozContext : public QObject
{
    Q_OBJECT
public:
    QmlMozContext(QObject* parent = 0) : QObject(parent) {}
    virtual ~QmlMozContext() {}
public Q_SLOTS:
    void newWindow(const QString& url = "about:mozilla");
};

#endif /* qmozcontext_h */
