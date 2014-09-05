//
//  ShutdownEventListener.h
//  libraries/shared/src
//
//  Created by Ryan Huffman on 09/03/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShutdownEventListener_h
#define hifi_ShutdownEventListener_h

#include <QObject>
#include <QAbstractNativeEventFilter>

class ShutdownEventListener : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    ShutdownEventListener(QObject* parent = NULL);

    virtual bool nativeEventFilter(const QByteArray& eventType, void* message, long* result);

signals:
    void receivedCloseEvent();
};

#endif // hifi_ShutdownEventListener_h
