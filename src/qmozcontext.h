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

class QNewWindowResponse : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint32 windowID READ getNewWindowID WRITE setNewWindowID FINAL)

public:
    QNewWindowResponse(QObject* parent = 0) : QObject(parent) {}
    QNewWindowResponse(const QNewWindowResponse& aMsg) : QObject(NULL) { mNewWindowID = aMsg.mNewWindowID; }
    virtual ~QNewWindowResponse() {}

    quint32 getNewWindowID() const { return mNewWindowID; }
    void setNewWindowID(const quint32& newWindowID) { mNewWindowID = newWindowID; }

private:
    quint32 mNewWindowID;
};

Q_DECLARE_METATYPE(QNewWindowResponse)

class QMozContext : public QObject
{
    Q_OBJECT
public:
    virtual ~QMozContext();

    mozilla::embedlite::EmbedLiteApp* GetApp();
    void setPixelRatio(float ratio);
    float pixelRatio() const;

    static QMozContext* GetInstance();

Q_SIGNALS:
    void onInitialized();
    void newWindowRequested(const QString& url, const unsigned& parentId, QNewWindowResponse* newWinResponse);
    void recvObserve(const QString message, const QVariant data);

public Q_SLOTS:
    bool initialized();
    void setIsAccelerated(bool aIsAccelerated);
    bool isAccelerated();
    void addComponentManifest(const QString& manifestPath);
    void addObserver(const QString& aTopic);
    quint32 newWindow(const QString& url, const quint32& parentId = 0);
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
    void setCompositorInSeparateThread(bool aEnabled);

private:
    QMozContext(QObject* parent = 0);

    QMozContextPrivate* d;
    friend class QMozContextPrivate;
};

#endif /* qmozcontext_h */
