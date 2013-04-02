#include "qmlmozcontext.h"

QObject* QmlMozContext::instance() const
{
    return QMozContext::GetInstance();
}
