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

PacketPayload::PacketPayload(char* data, int maxBytes) :
    _data(data)
    _maxBytes(maxBytes)
{

}

int PacketPayload::append(const char* src, int srcBytes) {
    // this is a call to write at the current index
    int numWrittenBytes = write(src, srcBytes, _index);

    if (numWrittenBytes > 0) {
        // we wrote some bytes, push the index
        _index += numWrittenBytes;
        return numWrittenBytes;
    } else {
        return numWrittenBytes;
    }
}

const int PACKET_WRITE_ERROR = -1;

int PacketPayload::write(const char* src, int srcBytes, int index) {
    if (index >= _maxBytes) {
        // we were passed a bad index, return -1
        return PACKET_WRITE_ERROR;
    }

    // make sure we have the space required to write this block
    int bytesAvailable = _maxBytes - index;

    if (bytesAvailable < srcBytes) {
        // good to go - write the data
        memcpy(_data + index, src, srcBytes);

        // should this cause us to push our index (is this the farthest we've written in data)?
        _index = std::max(_data + index + srcBytes, _index);

        // return the number of bytes written
        return srcBytes;
    } else {
        // not enough space left for this write - return an error
        return PACKET_WRITE_ERROR;
    }
}


