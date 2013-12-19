/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QMozContext"

#include <QVariant>
#include <QThread>
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QApplication>
#include <qjson/serializer.h>
#include <qjson/parser.h>
#include <QtDeclarative/qdeclarative.h>
#else
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QtQml/QtQml>
#endif

#include "qmozembedlog.h"
#include "qmozcontext.h"
#include "geckoworker.h"
#include "qmessagepump.h"
#include "qmozviewcreator.h"

#include "nsDebug.h"
#include "mozilla/embedlite/EmbedLiteMessagePump.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedInitGlue.h"
#include "mozilla/embedlite/EmbedLiteView.h"

using namespace mozilla::embedlite;

static QMozContext* protectSingleton = nullptr;

class QMozContextPrivate : public EmbedLiteAppListener {
public:
    QMozContextPrivate(QMozContext* qq)
    : q(qq)
    , mApp(NULL)
    , mInitialized(false)
    , mPixelRatio(1.0)
    , mThread(new QThread())
    , mEmbedStarted(false)
    , mQtPump(NULL)
    , mViewCreator(NULL)
    , mAsyncContext(getenv("USE_ASYNC"))
    {
        LOGT("Create new Context: %p, parent:%p", (void*)this, (void*)qq);
        setenv("BUILD_GRE_HOME", BUILD_GRE_HOME, 1);
        LoadEmbedLite();
        mApp = XRE_GetEmbedLite();
        mApp->SetListener(this);
        if (mAsyncContext) {
            mQtPump = new MessagePumpQt(mApp);
        }
    }

    virtual ~QMozContextPrivate() {
        // deleting a running thread may result in a crash
        if (!mThread->isFinished()) {
            mThread->exit(0);
            mThread->wait();
        }
        delete mThread;
    }

    virtual bool ExecuteChildThread() {
        if (!getenv("GECKO_THREAD")) {
            LOGT("Execute in child Native thread: %p", (void*)mThread);
            GeckoWorker *worker = new GeckoWorker(mApp);

            QObject::connect(mThread, SIGNAL(started()), worker, SLOT(doWork()));
            QObject::connect(mThread, SIGNAL(finished()), worker, SLOT(quit()));
            worker->moveToThread(mThread);

            mThread->start(QThread::LowPriority);
            return true;
        }
        return false;
    }
    // Native thread must be stopped here
    virtual bool StopChildThread() {
        if (mThread) {
            LOGT("Stop Native thread: %p", (void*)mThread);
            mThread->exit(0);
            mThread->wait();
            return true;
        }
        return false;
    }
    // App Initialized and ready to API call
    virtual void Initialized() {
        mInitialized = true;
#if defined(GL_PROVIDER_EGL) || defined(GL_PROVIDER_GLX)
        if (mApp->GetRenderType() == EmbedLiteApp::RENDER_AUTO) {
            mApp->SetIsAccelerated(true);
        }
#endif
        setDefaultPrefs();
        mApp->LoadGlobalStyleSheet("chrome://global/content/embedScrollStyles.css", true);
        Q_EMIT q->onInitialized();
        QListIterator<QString> i(mObserversList);
        while (i.hasNext()) {
            const QString& str = i.next();
            mApp->AddObserver(str.toUtf8().data());
        }
        mObserversList.clear();
    }
    // App Destroyed, and ready to delete and program exit
    virtual void Destroyed() {
        LOGT("");
        if (mAsyncContext) {
            mQtPump->deleteLater();
        }
    }
    virtual void OnObserve(const char* aTopic, const PRUnichar* aData) {
        // LOGT("aTopic: %s, data: %s", aTopic, NS_ConvertUTF16toUTF8(aData).get());
        QString data((QChar*)aData);
        if (!data.startsWith('{') && !data.startsWith('[') && !data.startsWith('"')) {
            QVariant vdata = QVariant::fromValue(data);
            Q_EMIT q->recvObserve(aTopic, vdata);
            return;
        }
        bool ok = true;
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
        QJson::Parser parser;
        QVariant vdata = parser.parse(data.toUtf8(), &ok);
#else
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &error);
        ok = error.error == QJsonParseError::NoError;
        QVariant vdata = doc.toVariant();
#endif
        if (ok) {
            // LOGT("mesg:%s, data:%s", aTopic, data.toUtf8().data());
            Q_EMIT q->recvObserve(aTopic, vdata);
        } else {
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
            LOGT("parse: s:'%s', err:%s, errLine:%i", data.toUtf8().data(), parser.errorString().toUtf8().data(), parser.errorLine());
#else
            LOGT("parse: s:'%s', err:%s, errLine:%i", data.toUtf8().data(), error.errorString().toUtf8().data(), error.offset);
#endif
        }
    }
    void setDefaultPrefs()
    {
        if (getenv("DS_UA")) {
            mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (X11; Linux x86_64; rv:20.0) Gecko/20130124 Firefox/20.0");
        } else if (getenv("MT_UA")) {
            mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (Android; Tablet; rv:20.0) Gecko/20.0 Firefox/20.0");
        } else if (getenv("MP_UA")) {
            mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (Android; Mobile; rv:20.0) Gecko/20.0 Firefox/20.0");
        } else if (getenv("CT_UA")) {
            mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (Linux; Android 4.0.3; Transformer Prime TF201 Build/IML74K) AppleWebKit/535.19 (KHTML, like Gecko) Tablet Chrome/18.0.1025.166 Safari/535.19");
        } else if (getenv("GB_UA")) {
            mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (Meego; NokiaN9) AppleWebKit/534.13 (KHTML, like Gecko) NokiaBrowser/8.5.0 Mobile Safari/534.13");
        } else {
            const char* customUA = getenv("CUSTOM_UA");
            if (customUA) {
                mApp->SetCharPref("general.useragent.override", customUA);
            }
        }
    }
    bool IsInitialized() { return mApp && mInitialized; }

    virtual uint32_t CreateNewWindowRequested(const uint32_t& chromeFlags, const char* uri, const uint32_t& contextFlags, EmbedLiteView* aParentView)
    {
        LOGT("QtMozEmbedContext new Window requested: parent:%p", (void*)aParentView);
        uint32_t viewId = QMozContext::GetInstance()->createView(QString(uri), aParentView ? aParentView->GetUniqueID() : 0);
        return viewId;
    }

    EmbedLiteMessagePump* EmbedLoop() { return mQtPump->EmbedLoop(); }

    QList<QString> mObserversList;
private:
    QMozContext* q;
    EmbedLiteApp* mApp;
    bool mInitialized;
    float mPixelRatio;
    friend class QMozContext;
    QThread* mThread;
    bool mEmbedStarted;
    EmbedLiteMessagePump* mEventLoopPrivate;
    MessagePumpQt* mQtPump;
    bool mAsyncContext;
    QMozViewCreator *mViewCreator;
};

QMozContext::QMozContext(QObject* parent)
    : QObject(parent)
    , d(new QMozContextPrivate(this))
{
    Q_ASSERT(protectSingleton == nullptr);
    protectSingleton = this;
}

void QMozContext::setCompositorInSeparateThread(bool aEnabled)
{
    d->mApp->SetCompositorInSeparateThread(true);
}

void QMozContext::setProfile(const QString profilePath)
{
    d->mApp->SetProfilePath(!profilePath.isEmpty() ? profilePath.toUtf8().data() : NULL);
}

QMozContext::~QMozContext()
{
    protectSingleton = nullptr;
    if (d->mApp) {
        d->mApp->SetListener(NULL);
    }
    delete d;
}

void
QMozContext::sendObserve(const QString& aTopic, const QVariant& variant)
{
    if (!d->mApp)
        return;

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QJson::Serializer serializer;
    QByteArray array = serializer.serialize(variant);
#else
    QJsonDocument doc = QJsonDocument::fromVariant(variant);
    QByteArray array = doc.toJson();
#endif

    d->mApp->SendObserve(aTopic.toUtf8().data(), (const PRUnichar*)QString(array).constData());
}

void
QMozContext::sendObserve(const QString& aTopic, const QString& string)
{
    if (!d->mApp)
        return;

    d->mApp->SendObserve(aTopic.toUtf8().data(), (const PRUnichar*)string.constData());
}

void
QMozContext::addComponentManifest(const QString& manifestPath)
{
    if (!d->mApp)
        return;
    d->mApp->AddManifestLocation(manifestPath.toUtf8().data());
}

void
QMozContext::addObserver(const QString& aTopic)
{
    if (!d->IsInitialized()) {
        d->mObserversList.append(aTopic);
        return;
    }

    d->mApp->AddObserver(aTopic.toUtf8().data());
}

void QMozContext::addObservers(const QStringList& aObserversList)
{
    if (!d->mApp)
        return;

    nsTArray<nsCString> observersList;
    for (int i = 0; i < aObserversList.size(); i++) {
        observersList.AppendElement(aObserversList.at(i).toUtf8().data());
    }
    d->mApp->AddObservers(observersList);
}

QMozContext*
QMozContext::GetInstance()
{
    static QMozContext* lsSingleton = nullptr;
    if (!lsSingleton) {
        lsSingleton = new QMozContext(0);
        NS_ASSERTION(lsSingleton, "not initialized");
    }
    return lsSingleton;
}

void QMozContext::runEmbedding(int aDelay)
{
    if (!d->mEmbedStarted) {
        d->mEmbedStarted = true;
        if (d->mAsyncContext) {
            d->mApp->StartWithCustomPump(EmbedLiteApp::EMBED_THREAD, d->EmbedLoop());
        } else {
            d->mApp->Start(EmbedLiteApp::EMBED_THREAD);
            d->mEmbedStarted = false;
        }
    }
}

bool
QMozContext::initialized()
{
    return d->mInitialized;
}

EmbedLiteApp*
QMozContext::GetApp()
{
    return d->mApp;
}

void QMozContext::setPixelRatio(float ratio)
{
    d->mPixelRatio = ratio;
    setPref(QString("layout.css.devPixelsPerPx"), QString("%1").arg(ratio));
}

float QMozContext::pixelRatio() const
{
    return d->mPixelRatio;
}

void QMozContext::stopEmbedding()
{
    GetApp()->Stop();
}

quint32
QMozContext::createView(const QString& url, const quint32& parentId)
{
    Q_EMIT newWindowRequested(url);
    return d->mViewCreator ? d->mViewCreator->createView(url, parentId) : 0;
}

void
QMozContext::setIsAccelerated(bool aIsAccelerated)
{
    if (!d->mApp)
        return;

    d->mApp->SetIsAccelerated(aIsAccelerated);
}

bool
QMozContext::isAccelerated()
{
    if (!d->mApp)
        return false;
    return d->mApp->IsAccelerated();
}

void
QMozContext::setPref(const QString& aName, const QVariant& aPref)
{
    LOGT("name:%s, type:%i", aName.toUtf8().data(), aPref.type());
    if (!d->mInitialized) {
        LOGT("Error: context not yet initialized");
        return;
    }
    switch (aPref.type()) {
    case QVariant::String:
        d->mApp->SetCharPref(aName.toUtf8().data(), aPref.toString().toUtf8().data());
        break;
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
        d->mApp->SetIntPref(aName.toUtf8().data(), aPref.toInt());
        break;
    case QVariant::Bool:
        d->mApp->SetBoolPref(aName.toUtf8().data(), aPref.toBool());
        break;
    case QMetaType::Float:
    case QMetaType::Double:
        if (aPref.canConvert<int>()) {
            d->mApp->SetIntPref(aName.toUtf8().data(), aPref.toInt());
        } else {
            d->mApp->SetCharPref(aName.toUtf8().data(), aPref.toString().toUtf8().data());
        }
        break;
    default:
        LOGT("Unknown pref type: %i", aPref.type());
    }
}

void
QMozContext::notifyFirstUIInitialized()
{
    static bool sCalledOnce = false;
    if (!sCalledOnce) {
        d->mApp->SendObserve("final-ui-startup", NULL);
        sCalledOnce = true;
    }
}

void QMozContext::setViewCreator(QMozViewCreator* viewCreator)
{
    d->mViewCreator = viewCreator;
}
