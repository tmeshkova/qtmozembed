#include "qmlmozcontext.h"
#include <QEventLoop>
#include <QAbstractEventDispatcher>
#include <QThread>
#include <QDateTime>
#include <QCoreApplication>

QObject* QmlMozContext::instance() const
{
    return QMozContext::GetInstance();
}

void
QmlMozContext::waitLoop(bool mayWait, int aTimeout)
{
    QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents;

    if (mayWait)
        flags |= QEventLoop::WaitForMoreEvents;

    QAbstractEventDispatcher *dispatcher =  QAbstractEventDispatcher::instance(QThread::currentThread());
    if (!dispatcher) {
        return;
    }

    if (aTimeout > -1) {
        QCoreApplication::processEvents(flags, aTimeout);
    }
    dispatcher->processEvents(flags);
}

void
QmlMozContext::dumpTS(const QString& msg)
{
    printf("TimeStamp: msg:\"%s\", Ts: %llu\n", msg.toUtf8().data(), QDateTime::currentMSecsSinceEpoch() / 1000);
}
