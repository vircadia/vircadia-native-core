//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <mutex>
#include <functional>

namespace ovr {

using Mutex = std::mutex;
using Condition = std::condition_variable;
using Lock = std::unique_lock<Mutex>;
using Task = std::function<void()>;
using LockTask = std::function<void(Lock& lock)>;

class TaskQueue {
public:
    // Execute a task on another thread
    void submitTaskBlocking(const Task& task);
    void submitTaskBlocking(Lock& lock, const Task& task);
    void pollTask();

    void withLock(const Task& task);
    void withLockConditional(const LockTask& task);
private:
    Mutex _mutex;
    Task _task;
    bool _taskPending{ false };
    Condition _taskCondition;
};

}





