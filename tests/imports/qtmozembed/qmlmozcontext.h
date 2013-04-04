#ifndef QMLMOZCONTEXT_H
#define QMLMOZCONTEXT_H

#include <QDeclarativeItem>
#include "qmozcontext.h"

/*!
 * Declarative wrapper for singleton.
 */
class QmlMozContext : public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY(QObject* instance READ instance CONSTANT)

public:
    QObject* instance() const;
public Q_SLOTS:
    void waitLoop(bool mayWait = true, int aTimeout = -1);
    void dumpTS(const QString& msg);
};

#endif
