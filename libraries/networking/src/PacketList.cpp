//
//  PacketList.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/06/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketList.h"

PacketList::PacketList(PacketType::Value packetType, bool isOrdered) :
    _packetType(packetType),
    _isOrdered(isOrdered)
{

}

qint64 writeData(const char* data, qint64 maxSize) {
    if (!_currentPacket) {
        // we don't have a current packet, time to set one up
        _currentPacket = T::create(_packetType);
    }

    // check if this block of data can fit into the currentPacket
    if (maxSize <= _currentPacket.size()) {
        // it fits, just write it to the current packet
        _currentPacket.write(data, maxSize);

        // return the number of bytes written
        return maxSize;
    } else {
        // it does not fit - this may need to be in the next packet

        if (!_isOrdered) {
            auto newPacket = T::create(_packetType);
            PacketPayload& newPayload = newPacket.payload();

            if (_segmentStartIndex >= 0) {
                // We in the process of writing a segment for an unordered PacketList.
                // We need to try and pull the first part of the segment out to our new packet

                PacketPayload& currentPayload = _currentPacket->payload();

                // check now to see if this is an unsupported write
                int numBytesToEnd = currentPayload.size() - _segmentStartIndex;

                if ((newPayload.size() - numBytesToEnd) < maxSize) {
                    // this is an unsupported case - the segment is bigger than the size of an individual packet
                    // but the PacketList is not going to be sent ordered
                    qDebug() << "Error in PacketList::writeData - attempted to write a segment to an unordered packet that is"
                        << "larger than the payload size.";
                    return -1;
                }

                // copy from currentPacket where the segment started to the beginning of the newPacket
                newPayload.write(currentPacket.constData() + _segmentStartIndex, numBytesToEnd);

                // the current segment now starts at the beginning of the new packet
                _segmentStartIndex = 0;

                // shrink the current payload to the actual size of the packet
                currentPayload.setActualSize(_segmentStartIndex);
            } else {
                // shrink the current payload to the actual size of the packet
                currentPayload.trim();
            }

            // move the current packet to our list of packets
            _packets.insert(std::move(_currentPacket));

            // write the data to the newPacket
            newPayload.write(data, maxSize);

            // set our current packet to the new packet
            _currentPacket = newPacket;

        } else {
            // we're an ordered PacketList - let's fit what we can into the current packet and then put the leftover
            // into a new packet
            PacketPayload& currentPayload = _currentPacket.payload();

            int numBytesToEnd = _currentPayload.size() - _currentPayload.pos();
            _currentPacket.write(data, numBytesToEnd);

            // move the current packet to our list of packets
            _packets.insert(std::move(_currentPacket));

            // recursively call our writeData method for the remaining data to write to a new packet
            writeData(data + numBytesToEnd, maxSize - numBytesToEnd);
        }
    }
}

