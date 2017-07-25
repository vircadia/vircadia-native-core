//
//  Created by Bradley Austin Davis on 2017/06/06
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ThreadHelpers.h"

#include <QtCore/QDebug>

// Support for viewing the thread name in the debugger.  
// Note, Qt actually does this for you but only in debug builds
// Code from https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
// and matches logic in `qt_set_thread_name` in qthread_win.cpp
#ifdef Q_OS_WIN
#include <qt_windows.h>
#pragma pack(push,8)  
struct THREADNAME_INFO {
    DWORD dwType; // Must be 0x1000.  
    LPCSTR szName; // Pointer to name (in user addr space).  
    DWORD dwThreadID; // Thread ID (-1=caller thread).  
    DWORD dwFlags; // Reserved for future use, must be zero.  
};
#pragma pack(pop)  
#endif

void setThreadName(const std::string& name) {
#ifdef Q_OS_WIN
    static const DWORD MS_VC_EXCEPTION = 0x406D1388;
    THREADNAME_INFO info{ 0x1000, name.c_str(), (DWORD)-1, 0 };
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    } __except (EXCEPTION_EXECUTE_HANDLER) { }
#endif
}

void moveToNewNamedThread(QObject* object, const QString& name, std::function<void(QThread*)> preStartCallback, std::function<void()> startCallback, QThread::Priority priority) {
    Q_ASSERT(QThread::currentThread() == object->thread());
    // setup a thread for the NodeList and its PacketReceiver
    QThread* thread = new QThread();
    thread->setObjectName(name);

    // Execute any additional work to do before the thread starts (like moving members to the target thread
    preStartCallback(thread);

    // Link the in-thread initialization code
    QObject::connect(thread, &QThread::started, [name, startCallback] { 
        if (!name.isEmpty()) {
            // Make it easy to spot our thread processes inside the debugger
            setThreadName("Hifi_" + name.toStdString());
        }
        startCallback();
    });

    // Make sure the thread will be destroyed and cleaned up
    QObject::connect(object, &QObject::destroyed, thread, &QThread::quit);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    // put the object on the thread
    object->moveToThread(thread);
    thread->start();
    if (priority != QThread::InheritPriority) {
        thread->setPriority(priority);
    }
}

void moveToNewNamedThread(QObject* object, const QString& name, std::function<void()> startCallback, QThread::Priority priority) {
    moveToNewNamedThread(object, name, [](QThread*){}, startCallback, priority);
}

void moveToNewNamedThread(QObject* object, const QString& name, QThread::Priority priority) {
    moveToNewNamedThread(object, name, [](QThread*){}, []{}, priority);
}
