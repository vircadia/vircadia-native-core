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

#include "ShutdownEventListener.h"

ShutdownEventListener::ShutdownEventListener(QObject* parent) : QObject(parent) {
}

bool ShutdownEventListener::nativeEventFilter(const QByteArray &eventType, void* msg, long* result) {
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG* message = (MSG*)msg;
        if (message->message == WM_CLOSE) {
            emit receivedCloseEvent();
        }
    }
#endif
    return true;
}
