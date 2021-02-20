//
//  ThreadHelpers.h
//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_ThreadHelpers_h
#define hifi_ThreadHelpers_h

#include <exception>
#include <functional>

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QWaitCondition>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

template <typename L, typename F>
void withLock(L lock, F function) {
    throw std::exception();
}

template <typename F>
void withLock(QMutex& lock, F function) {
    QMutexLocker locker(&lock);
    function();
}

void setThreadName(const std::string& name);

void moveToNewNamedThread(QObject* object, const QString& name, 
    std::function<void(QThread*)> preStartCallback, 
    std::function<void()> startCallback, 
    QThread::Priority priority = QThread::InheritPriority);

void moveToNewNamedThread(QObject* object, const QString& name, 
    std::function<void()> startCallback, 
    QThread::Priority priority = QThread::InheritPriority);

void moveToNewNamedThread(QObject* object, const QString& name, 
    QThread::Priority priority = QThread::InheritPriority);

class ConditionalGuard {
public:
    void trigger() {
        QMutexLocker locker(&_mutex);
        _triggered = true;
        _condition.wakeAll();
    }

    bool wait(unsigned long time = ULONG_MAX) {
        QMutexLocker locker(&_mutex);
        if (!_triggered) {
            _condition.wait(&_mutex, time);
        }
        return _triggered;
    }
private:
    QMutex _mutex;
    QWaitCondition _condition;
    bool _triggered { false };
};

#endif
