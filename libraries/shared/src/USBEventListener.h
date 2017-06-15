//
//  USBEventListener.h
//  libraries/midi
//
//  Created by Burt Sloane
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_USBEventListener_h
#define hifi_USBEventListener_h

#include <QObject>
#include <QAbstractNativeEventFilter>

class USBEventListener : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
	static USBEventListener& getInstance();

    virtual bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;
private:
	USBEventListener(QObject* parent = 0);
};

#endif // hifi_USBEventListener_h
