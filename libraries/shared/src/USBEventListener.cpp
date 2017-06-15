//
//  USBEventListener.cpp
//  libraries/shared/src
//
//  Created by Ryan Huffman on 09/03/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "USBEventListener.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#else
#include <csignal>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

USBEventListener& USBEventListener::getInstance() {
	static USBEventListener staticInstance;
    return staticInstance;
}

void signalHandler(int param) {
    // tell the qApp it should quit
    QMetaObject::invokeMethod(qApp, "quit");
}

USBEventListener::USBEventListener(QObject* parent) : QObject(parent) {
#ifndef Q_OS_WIN
#endif
}


bool USBEventListener::nativeEventFilter(const QByteArray &eventType, void* msg, long* result) {
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG* message = (MSG*)msg;
        if (message->message == WM_DEVICECHANGE) {
        }
    }
#endif
    return false;
}
