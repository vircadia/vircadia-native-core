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

void PacketList::createPacketWithExtendedHeader() {
    // use the static create method to create a new packet
    _currentPacket = T::create(_packetType);

    // add the extended header to the front of the packet
    if (_currentPacket.write(_extendedHeader) == -1) {
        qDebug() << "Could not write extendedHeader in PacketList::createPacketWithExtendedHeader"
            << "- make sure that _extendedHeader is not larger than the payload capacity.";
    }
}

qint64 writeData(const char* data, qint64 maxSize) {
    if (!_currentPacket) {
        // we don't have a current packet, time to set one up
        createPacketWithExtendedHeader();
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

            if (_segmentStartIndex >= 0) {
                // We in the process of writing a segment for an unordered PacketList.
                // We need to try and pull the first part of the segment out to our new packet

                // check now to see if this is an unsupported write
                int numBytesToEnd = _currentPacket.size() - _segmentStartIndex;

                if ((newPacket.size() - numBytesToEnd) < maxSize) {
                    // this is an unsupported case - the segment is bigger than the size of an individual packet
                    // but the PacketList is not going to be sent ordered
                    qDebug() << "Error in PacketList::writeData - attempted to write a segment to an unordered packet that is"
                        << "larger than the payload size.";
                    return -1;
                }

                // copy from currentPacket where the segment started to the beginning of the newPacket
                newPacket.write(currentPacket.constData() + _segmentStartIndex, numBytesToEnd);

                // the current segment now starts at the beginning of the new packet
                _segmentStartIndex = 0;

                // shrink the current payload to the actual size of the packet
                currentPacket.setSizeUsed(_segmentStartIndex);
            }

            // move the current packet to our list of packets
            _packets.insert(std::move(_currentPacket));

            // write the data to the newPacket
            newPacket.write(data, maxSize);

            // set our current packet to the new packet
            _currentPacket = newPacket;

            // return the number of bytes written to the new packet
            return maxSize;
        } else {
            // we're an ordered PacketList - let's fit what we can into the current packet and then put the leftover
            // into a new packet

            int numBytesToEnd = _currentPacket.size() - _currentPacket.sizeUsed();
            _currentPacket.write(data, numBytesToEnd);

            // move the current packet to our list of packets
            _packets.insert(std::move(_currentPacket));

            // recursively call our writeData method for the remaining data to write to a new packet
            return numBytesToEnd + writeData(data + numBytesToEnd, maxSize - numBytesToEnd);
        }
    }
}

void PacketList::closeCurrentPacket() {
    // move the current packet to our list of packets
    _packets.insert(std::move(_currentPacket));
}

