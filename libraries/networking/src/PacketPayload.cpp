//
//  PacketPayload.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/06/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketPayload.h"

PacketPayload::PacketPayload(char* data, qint64 maxBytes) :
    _data(data)
    _maxBytes(maxBytes)
{

}

qint64 PacketPayload::append(const char* src, qint64 srcBytes) {
    // this is a call to write at the current index
    qint64 numWrittenBytes = write(src, srcBytes, _index);

    if (numWrittenBytes > 0) {
        // we wrote some bytes, push the index
        _index += numWrittenBytes;
        return numWrittenBytes;
    } else {
        return numWrittenBytes;
    }
}

const int PACKET_READ_ERROR = -1;

qint64 PacketPayload::write(const char* src, qint64 srcBytes, qint64 index) {
    if (index >= _maxBytes) {
        // we were passed a bad index, return error
        return PACKET_WRITE_ERROR;
    }

    // make sure we have the space required to write this block
    qint64 bytesAvailable = _maxBytes - index;

    if (bytesAvailable < srcBytes) {
        // good to go - write the data
        memcpy(_data + index, src, srcBytes);

        // should this cause us to push our index (is this the farthest we've written in data)?
        _writeIndex = std::max(_data + index + srcBytes, _writeIndex);

        // return the number of bytes written
        return srcBytes;
    } else {
        // not enough space left for this write - return an error
        return PACKET_WRITE_ERROR;
    }
}

qint64 PacketPayload::readNext(char* dest, qint64 maxSize) {
    // call read at the current _readIndex
    int numBytesRead = read(dest, maxSize, _readIndex);

    if (numBytesRead > 0) {
        // we read some data, push the read index
        _readIndex += numBytesRead;
    }

    return numBytesRead;
}

const qint64 PACKET_READ_ERROR = -1;

qint64 PacketPayload::read(char* dest, qint64 maxSize, qint64 index) {
    if (index >= _maxBytes) {
        // we were passed a bad index, return error
        return PACKET_READ_ERROR;
    }

    // we're either reading what is left from the index or what was asked to be read
    qint64 numBytesToRead = std::min(_maxSize - index, maxSize);

    if (numBytesToRead > 0) {
        // read out the data
        memcpy(dest, _data + index, numBytesToRead);
    }

    return numBytesToRead;
}
