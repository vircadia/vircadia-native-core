//
//  Created by Bradley Austin Davis on 2018/11/23
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TaskQueue.h"

using namespace ovr;

void TaskQueue::submitTaskBlocking(Lock& lock, const Task& newTask) {
    _task = newTask;
    _taskPending = true;
    _taskCondition.wait(lock, [=]() -> bool { return !_taskPending; });
}

void TaskQueue::submitTaskBlocking(const Task& task) {
    Lock lock(_mutex);
    submitTaskBlocking(lock, task);
}

void TaskQueue::pollTask() {
    Lock lock(_mutex);
    if (_taskPending) {
        _task();
        _taskPending = false;
        _taskCondition.notify_one();
    }
}

void TaskQueue::withLock(const Task& task) {
    Lock lock(_mutex);
    task();
}

void TaskQueue::withLockConditional(const LockTask& task) {
    Lock lock(_mutex);
    task(lock);
}
