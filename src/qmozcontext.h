/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmozcontext_h
#define qmozcontext_h

#include <QObject>
#include <QVariant>
#include <QStringList>

class QMozContextPrivate;

namespace mozilla {
namespace embedlite {
class EmbedLiteApp;
}}
class QMozViewCreator;

class QMozContext : public QObject
{
    Q_OBJECT
public:
    typedef void (*TaskCallback)(void* data);
    typedef void* TaskHandle;

    virtual ~QMozContext();

    mozilla::embedlite::EmbedLiteApp* GetApp();
    void setPixelRatio(float ratio);
    float pixelRatio() const;
    Q_INVOKABLE bool initialized() const;
    Q_INVOKABLE bool isAccelerated() const;

    static QMozContext* GetInstance();

    TaskHandle PostUITask(TaskCallback, void* data, int timeout = 0);
    TaskHandle PostCompositorTask(TaskCallback, void* data, int timeout = 0);
    void CancelTask(TaskHandle);

Q_SIGNALS:
    void onInitialized();
    void destroyed();
    void lastViewDestroyed();
    void lastWindowDestroyed();
    void recvObserve(const QString message, const QVariant data);

public Q_SLOTS:
    void setIsAccelerated(bool aIsAccelerated);
    void addComponentManifest(const QString& manifestPath);
    void addObserver(const QString& aTopic);
    void sendObserve(const QString& aTopic, const QString& string);
    void sendObserve(const QString& aTopic, const QVariant& variant);
    // running this without delay specified will execute Gecko/Qt nested main loop
    // and block this call until stopEmbedding called
    void runEmbedding(int aDelay = -1);
    void stopEmbedding();
    void setPref(const QString& aName, const QVariant& aPref);
    void notifyFirstUIInitialized();
    void setProfile(const QString);
    void addObservers(const QStringList& aObserversList);
    void setViewCreator(QMozViewCreator* viewCreator);
    quint32 createView(const QString& url, const quint32& parentId = 0);

private:
    QMozContext(QObject* parent = 0);

    QMozContextPrivate* d;
    friend class QMozContextPrivate;
};

#endif /* qmozcontext_h */
