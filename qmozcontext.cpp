/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QMozContext"
#include "mozilla/embedlite/EmbedLog.h"

#include <QTimer>
#include <QApplication>

#include "qmozcontext.h"

#include "nsDebug.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedInitGlue.h"

using namespace mozilla::embedlite;

static QMozContext* sSingleton = nullptr;

void
GeckoThread::Quit()
{
    if (mEventLoop)
        mEventLoop->quit();
    quit();
    wait();
}

void
GeckoThread::run()
{
    mContext->GetApp()->StartChildThread();
    mEventLoop = new QEventLoop();
    mEventLoop->exec();
    printf("Call Term StopChildThread\n");
    mContext->GetApp()->StopChildThread();
    delete mEventLoop;
    mEventLoop = 0;
}

class QMozContextPrivate : public EmbedLiteAppListener {
public:
    QMozContextPrivate(QMozContext* qq)
    : q(qq)
    , mApp(NULL)
    , mInitialized(false)
    , mThread(new GeckoThread(qq))
    {
    }
    virtual ~QMozContextPrivate() {
        delete mThread;
    }

    virtual bool ExecuteChildThread() {
        LOGT();
        if (!getenv("GECKO_THREAD")) {
            mThread->start();
            mThread->setPriority(QThread::LowPriority);
            return true;
        }
        return false;
    }
    // Native thread must be stopped here
    virtual bool StopChildThread() {
        LOGT();
        if (mThread) {
            mThread->Quit();
            return true;
        }
        return false;
    }
    // App Initialized and ready to API call
    virtual void Initialized() {
        mInitialized = true;
        setDefaultPrefs();
        mApp->LoadGlobalStyleSheet("chrome://global/content/embedScrollStyles.css", true);
        Q_EMIT q->onInitialized();
        // Listen history notifications
        // mApp->AddObserver("history:checkurivisited");
        // mApp->AddObserver("history:markurivisited");
    }
    // App Destroyed, and ready to delete and program exit
    virtual void Destroyed() {
        LOGT();
    }
    virtual void OnObserve(const char* aTopic, const PRUnichar* aData) {
        LOGT("aTopic: %s, data: %s", aTopic, NS_ConvertUTF16toUTF8(aData).get());
    }
    void setDefaultPrefs()
    {
        LOGT();
        if (getenv("DS_UA")) {
            mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (X11; Linux x86_64; rv:20.0) Gecko/20130124 Firefox/20.0");
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
        mApp->SetBoolPref("layout.build_layers_for_scrollable_views", getenv("USE_SCROLL_VIEWS") != 0);
    }

private:
    QMozContext* q;
    EmbedLiteApp* mApp;
    bool mInitialized;
    friend class QMozContext;
    friend class GeckoThread;
    GeckoThread* mThread;
};

QMozContext::QMozContext(QObject* parent)
    : QObject(parent)
    , d(new QMozContextPrivate(this))
{
    LoadEmbedLite();
    d->mApp = XRE_GetEmbedLite();
    d->mApp->SetListener(d);
    QObject::connect(qApp, SIGNAL(lastWindowClosed()), this, SLOT(onLastWindowClosed()));
    QTimer::singleShot(0, this, SLOT(runEmbedding()));
}

QMozContext::~QMozContext()
{
    delete d;
}

QMozContext*
QMozContext::GetInstance()
{
    if (!sSingleton) {
        sSingleton = new QMozContext();
        NS_ASSERTION(sSingleton, "not initialized");
    }
    return sSingleton;
}

void QMozContext::runEmbedding()
{
    d->mApp->Start(EmbedLiteApp::EMBED_THREAD);
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

void QMozContext::onLastWindowClosed()
{
    GetApp()->Stop();
}
