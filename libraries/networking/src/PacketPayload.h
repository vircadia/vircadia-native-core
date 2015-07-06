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
    PacketPayload(char* data, qint64 maxBytes);

    qint64 write(const char* src, qint64 srcBytes, qint64 index = 0);
    qint64 append(const char* src, qint64 srcBytes);

    qint64 read(char* dest, qint64 maxSize, qint64 index = 0);
    qint64 readNext(char* dest, qint64 maxSize);

protected:
    virtual qint64 writeData(const char* data, qint64 maxSize) { return append(data, maxSize) };
    virtual qint64 readData(const char* data, qint64 maxSize) { return readNext(data, maxSize) };

private:
    char* _data;

    qint64 _writeIndex = 0;
    qint64 _readIndex = 0;

    qint64 _maxBytes = 0;
};

#endif // hifi_PacketByteArray_h
