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
    PacketPayload(char* data, qint64 size);

    virtual qint64 size() const { return _size; }
    virtual bool isSequential() const  { return false; }

protected:
    virtual qint64 writeData(const char* data, qint64 maxSize);
    virtual qint64 readData(const char* data, qint64 maxSize);

private:
    char* _data;
    qint64 _size = 0;
    qint64 _maxWritten = 0;
};

#endif // hifi_PacketPayload_h
