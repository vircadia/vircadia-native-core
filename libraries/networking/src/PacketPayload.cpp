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

PacketPayload::PacketPayload(char* data, qint64 size) :
    _data(data)
    _size(size)
{

}

const int PACKET_READ_ERROR = -1;

qint64 PacketPayload::writeData(const char* data, qint64 maxSize) {

    qint64 currentPos = pos();

    // make sure we have the space required to write this block
    qint64 bytesAvailable = _size - currentPos;

    if (bytesAvailable < srcBytes) {
        // good to go - write the data
        memcpy(_data + currentPos, src, srcBytes);

        // should this cause us to push our index (is this the farthest we've written in data)?
        seek(currentPos + srcBytes);

        // return the number of bytes written
        return srcBytes;
    } else {
        // not enough space left for this write - return an error
        return PACKET_WRITE_ERROR;
    }
}

const qint64 PACKET_READ_ERROR = -1;

qint64 PacketPayload::readData(char* dest, qint64 maxSize) {

    int currentPosition = pos();

    // we're either reading what is left from the current position or what was asked to be read
    qint64 numBytesToRead = std::min(_maxSize - currentPosition, maxSize);

    if (numBytesToRead > 0) {
        // read out the data
        memcpy(dest, _data + currentPosition, numBytesToRead);

        // seek to the end of the read
        seek(_data + currentPosition + numBytesToRead);
    }

    return numBytesToRead;
}
