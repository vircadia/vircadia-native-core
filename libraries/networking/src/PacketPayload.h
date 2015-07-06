//
//  PacketPayload.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/06/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketPayload_h
#define hifi_PacketPayload_h

#pragma once

#include <QtCore/QIODevice>

class PacketPayload : public QIODevice {
public:
    PacketPayload(char* data, qint64 capacity);

    qint64 getSizeUsed() const { return _sizeUsed; }
    void setSizeUsed(qint64 sizeUsed) { _sizeUsed = sizeUsed; }

    virtual qint64 size() const { return _capacity; }
    virtual bool isSequential() const  { return false; }

    const char* constData() const { return _data; }
protected:
    qint64 writeData(const char* data, qint64 maxSize);
    qint64 readData(char* data, qint64 maxSize);

private:
    char* _data;
    qint64 _sizeUsed = 0;
    qint64 _capacity = 0;
};

#endif // hifi_PacketPayload_h
