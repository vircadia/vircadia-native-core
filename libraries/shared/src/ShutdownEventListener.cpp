//
//  ShutdownEventListener.cpp
//  libraries/shared/src
//
//  Created by Ryan Huffman on 09/03/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCoreApplication>

#include "ShutdownEventListener.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#else
#include <csignal>
#endif

ShutdownEventListener& ShutdownEventListener::getInstance() {
    static ShutdownEventListener staticInstance;
    return staticInstance;
}

void signalHandler(int param) {
    // tell the qApp it should quit
    QMetaObject::invokeMethod(qApp, "quit");
}

ShutdownEventListener::ShutdownEventListener(QObject* parent) :
    QObject(parent)
{
#ifndef Q_OS_WIN
    // be a signal handler for SIGTERM so we can stop our children when we get it
    signal(SIGTERM, signalHandler);
#endif
}


bool ShutdownEventListener::nativeEventFilter(const QByteArray &eventType, void* msg, long* result) {
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG* message = (MSG*)msg;
        if (message->message == WM_CLOSE) {
            // tell our registered application to quit
            QMetaObject::invokeMethod(qApp, "quit");
        }
    }
#endif
    return false;
}
