//
//  Created by Bradley Austin Davis on 2017/06/06
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ThreadHelpers.h"

#include <QtCore/QDebug>


void moveToNewNamedThread(QObject* object, const QString& name, std::function<void()> startCallback, QThread::Priority priority) {
     Q_ASSERT(QThread::currentThread() == object->thread());
     // setup a thread for the NodeList and its PacketReceiver
     QThread* thread = new QThread();
     thread->setObjectName(name);

     QString tempName = name;
     QObject::connect(thread, &QThread::started, [startCallback] {
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

void moveToNewNamedThread(QObject* object, const QString& name, QThread::Priority priority) {
    moveToNewNamedThread(object, name, [] {}, priority);
}
