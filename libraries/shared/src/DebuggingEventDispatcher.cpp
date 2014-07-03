//
//  DebuggingEventDispatcher.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 4/1/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#include <QThread>

#include "DebuggingEventDispatcher.h"

DebuggingEventDispatcher::DebuggingEventDispatcher(QThread* thread) {
    _realEventDispatcher = thread->eventDispatcher();
    thread->setEventDispatcher(this);
}

void DebuggingEventDispatcher::flush() {
    _realEventDispatcher->flush();
}

bool DebuggingEventDispatcher::hasPendingEvents() {
    return _realEventDispatcher->hasPendingEvents();
}

void DebuggingEventDispatcher::interrupt() {
    _realEventDispatcher->interrupt();
}

bool DebuggingEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags) {
    return _realEventDispatcher->processEvents(flags);
}

/*
bool DebuggingEventDispatcher::registerEventNotifier(QWinEventNotifier* notifier) {
    return _realEventDispatcher->registerEventNotifier(notifier);
}
*/

void DebuggingEventDispatcher::registerSocketNotifier(QSocketNotifier* notifier) {
    _realEventDispatcher->registerSocketNotifier(notifier);
}

void DebuggingEventDispatcher::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject* object) {
    _realEventDispatcher->registerTimer(timerId, interval, timerType, object);
}

QList<DebuggingEventDispatcher::TimerInfo> DebuggingEventDispatcher::registeredTimers(QObject* object) const {
    return _realEventDispatcher->registeredTimers(object);
}

int	DebuggingEventDispatcher::remainingTime(int timerId) {
    return _realEventDispatcher->remainingTime(timerId);
}

/*
void DebuggingEventDispatcher::unregisterEventNotifier(QWinEventNotifier* notifier) {
    _realEventDispatcher->unregisterEventNotifier(notifier);
}
*/

void DebuggingEventDispatcher::unregisterSocketNotifier(QSocketNotifier* notifier) {
    _realEventDispatcher->unregisterSocketNotifier(notifier);
}

bool DebuggingEventDispatcher::unregisterTimer(int timerId) {
    return _realEventDispatcher->unregisterTimer(timerId);
}

bool DebuggingEventDispatcher::unregisterTimers(QObject* object) {
    return _realEventDispatcher->unregisterTimers(object);
}

void DebuggingEventDispatcher::wakeUp() {
    _realEventDispatcher->wakeUp();
}

/*
void installNativeEventFilter(QAbstractNativeEventFilter* filterObj);
int	registerTimer(int interval, Qt::TimerType timerType, QObject* object);
void removeNativeEventFilter(QAbstractNativeEventFilter* filter);
*/