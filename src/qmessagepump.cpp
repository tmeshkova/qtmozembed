/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "MessagePumpQt"

#include <QTimer>
#include <QEvent>
#include <QThread>
#include <QAbstractEventDispatcher>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QCoreApplication>
#else
#include <QGuiApplication>
#endif

#include "qmessagepump.h"

#include "mozilla/embedlite/EmbedLiteMessagePump.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

using namespace mozilla::embedlite;

namespace {
// Cached QEvent user type, registered for our event system
static int sPokeEvent;
}  // namespace

MessagePumpQt::MessagePumpQt(EmbedLiteApp* aApp)
  : mApp(aApp), mTimer(new QTimer(this)), state_(0), mLastDelayedWorkTime(-1), mThread(0)
{
  mThread = QThread::currentThreadId();
  mEventLoopPrivate = mApp->CreateEmbedLiteMessagePump(this);
  // Register our custom event type, to use in qApp event loop
  sPokeEvent = QEvent::registerEventType();
  connect(mTimer, SIGNAL(timeout()), this, SLOT(dispatchDelayed()));
  mTimer->setSingleShot(true);
}

MessagePumpQt::~MessagePumpQt()
{
//  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
  mTimer->stop();
  delete mTimer;
  delete state_;
  delete mEventLoopPrivate;
}

bool
MessagePumpQt::event(QEvent *e)
{
//  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, etype:%i\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), e->type());
  if (e->type() == sPokeEvent) {
//    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    HandleDispatch();
    return true;
  }
  return QObject::event(e);
}

void MessagePumpQt::HandleDispatchStatic(void* aPump)
{
  static_cast<MessagePumpQt*>(aPump)->HandleDispatch();
}

void MessagePumpQt::HandleDispatch()
{
  if (!state_ || state_->should_quit) {
//    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    return;
  }

//  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
  if (mEventLoopPrivate->DoWork(state_->delegate)) {
//    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    // there might be more, see more_work_is_plausible 
    // variable above, that's why we ScheduleWork() to keep going.
    ScheduleWorkLocal();
  }

  if (state_->should_quit) {
//    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    return;
  }

//  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
  bool doIdleWork = !mEventLoopPrivate->DoDelayedWork(state_->delegate);
  scheduleDelayedIfNeeded();
//  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());

  if (doIdleWork) {
//    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    if (mEventLoopPrivate->DoIdleWork(state_->delegate)) {
//      printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
      ScheduleWorkLocal();
    }
  }
}

void MessagePumpQt::ScheduleWorkLocal()
{
//  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
  QCoreApplication::postEvent(this,
                              new QEvent((QEvent::Type)sPokeEvent));
//  QCoreApplication::processEvents();
  // Process any "deleteLater" objects...
//  QCoreApplication::sendPostedEvents(0, (QEvent::Type)sPokeEvent);
}

void
MessagePumpQt::scheduleDelayedIfNeeded()
{
  if (mLastDelayedWorkTime == -1) {
    printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
    return;
  }

  if (mTimer->isActive()) {
    mTimer->stop();
  }

  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
  mTimer->start(mLastDelayedWorkTime >= 0 ? mLastDelayedWorkTime : 0);
}

void
MessagePumpQt::dispatchDelayed()
{
  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
  HandleDispatch();
}

void MessagePumpQt::Run(void* delegate)
{
  RunState* state = new RunState();
  state->delegate = delegate;
  state->should_quit = false;
  state->run_depth = state_ ? state_->run_depth + 1 : 1;
  state_ = state;
  mThread = QThread::currentThreadId();
//  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
  HandleDispatch();
}

void MessagePumpQt::Quit()
{
//  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
  if (state_) {
    state_->should_quit = true;
    state_->delegate = NULL;
  }
}

void MessagePumpQt::ScheduleWork()
{
  // PlatformThreadHandle thread_handle(), PlatformThreadId thread_id   PlatformThread::YieldCurrentThread()  PlatformThread::Join() pthread_self()
//  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId());
  ScheduleWorkLocal();
}

void MessagePumpQt::ScheduleDelayedWork(const int aDelay)
{
  printf(">>>>>>Func:%s::%d curThread:%p, curThrId:%p, aDelay:%i\n", __PRETTY_FUNCTION__, __LINE__, QThread::currentThread(), (void*)QThread::currentThreadId(), aDelay);
  mLastDelayedWorkTime = aDelay;
  scheduleDelayedIfNeeded();
}
