//
//  PacketList.cpp
//
//
//  Created by Clement on 7/13/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketList.h"

#include "../NetworkLogging.h"

#include <QDebug>

using namespace udt;

int packetListMetaTypeId = qRegisterMetaType<PacketList*>("PacketList*");

std::unique_ptr<PacketList> PacketList::create(PacketType packetType, QByteArray extendedHeader,
                                               bool isReliable, bool isOrdered) {
    auto packetList = std::unique_ptr<PacketList>(new PacketList(packetType, extendedHeader,
                                                                 isReliable, isOrdered));
    packetList->open(WriteOnly);
    return packetList;
}

std::unique_ptr<PacketList> PacketList::fromReceivedPackets(std::list<std::unique_ptr<Packet>>&& packets) {
    auto packetList = std::unique_ptr<PacketList>(new PacketList(PacketType::Unknown, QByteArray(), true, true));
    packetList->_packets = std::move(packets);
    packetList->open(ReadOnly);
    return packetList;
}

PacketList::PacketList(PacketType packetType, QByteArray extendedHeader, bool isReliable, bool isOrdered) :
    _packetType(packetType),
    _isOrdered(isOrdered),
    _isReliable(isReliable),
    _extendedHeader(extendedHeader)
{
    Q_ASSERT_X(!(!_isReliable && _isOrdered), "PacketList", "Unreliable ordered PacketLists are not currently supported");
}

PacketList::PacketList(PacketList&& other) :
    _packetType(other._packetType),
    _packets(std::move(other._packets)),
    _isOrdered(other._isOrdered),
    _isReliable(other._isReliable),
    _extendedHeader(std::move(other._extendedHeader))
{
}

HifiSockAddr PacketList::getSenderSockAddr() const {
    return _packets.size() > 0 ? _packets.front()->getSenderSockAddr() : HifiSockAddr();
}

void PacketList::startSegment() {
    _segmentStartIndex = _currentPacket ? _currentPacket->pos() : _extendedHeader.size();
}

void PacketList::endSegment() {
    _segmentStartIndex = -1;
}

size_t PacketList::getDataSize() const {
    size_t totalBytes = 0;
    for (const auto& packet : _packets) {
        totalBytes += packet->getDataSize();
    }

    if (_currentPacket) {
        totalBytes += _currentPacket->getDataSize();
    }

    return totalBytes;
}

size_t PacketList::getMessageSize() const {
    size_t totalBytes = 0;
    for (const auto& packet: _packets) {
        totalBytes += packet->getPayloadSize();
    }
    
    if (_currentPacket) {
        totalBytes += _currentPacket->getPayloadSize();
    }
    
    return totalBytes;
}

std::unique_ptr<Packet> PacketList::createPacket() {
    // use the static create method to create a new packet
    // If this packet list is supposed to be ordered then we consider this to be part of a message
    bool isPartOfMessage = _isOrdered;
    return Packet::create(-1, _isReliable, isPartOfMessage);
}

std::unique_ptr<Packet> PacketList::createPacketWithExtendedHeader() {
    // use the static create method to create a new packet
    auto packet = createPacket();

    if (!_extendedHeader.isEmpty()) {
        // add the extended header to the front of the packet
        if (packet->write(_extendedHeader) == -1) {
            qCDebug(networking) << "Could not write extendedHeader in PacketList::createPacketWithExtendedHeader"
                << "- make sure that _extendedHeader is not larger than the payload capacity.";
        }
    }

    return packet;
}

void PacketList::closeCurrentPacket(bool shouldSendEmpty) {
    if (shouldSendEmpty && !_currentPacket && _packets.empty()) {
        _currentPacket = createPacketWithExtendedHeader();
    }
    
    if (_currentPacket) {
        // move the current packet to our list of packets
        _packets.push_back(std::move(_currentPacket));
    }
}

QByteArray PacketList::getMessage() const {
    size_t sizeBytes = 0;

    for (const auto& packet : _packets) {
        sizeBytes += packet->size();
    }

    QByteArray data;
    data.reserve((int)sizeBytes);

    for (auto& packet : _packets) {
        data.append(packet->getPayload(), packet->getPayloadSize());
    }

    return data;
}

void PacketList::preparePackets(MessageNumber messageNumber) {
    Q_ASSERT(_packets.size() > 0);
    
    if (_packets.size() == 1) {
        _packets.front()->writeMessageNumber(messageNumber, Packet::PacketPosition::ONLY, 0);
    } else {
        const auto second = ++_packets.begin();
        const auto last = --_packets.end();
        Packet::MessagePartNumber messagePartNumber = 0;
        std::for_each(second, last, [&](const PacketPointer& packet) {
            packet->writeMessageNumber(messageNumber, Packet::PacketPosition::MIDDLE, ++messagePartNumber);
        });
        
        _packets.front()->writeMessageNumber(messageNumber, Packet::PacketPosition::FIRST, 0);
        _packets.back()->writeMessageNumber(messageNumber, Packet::PacketPosition::LAST, ++messagePartNumber);
    }
}

const qint64 PACKET_LIST_WRITE_ERROR = -1;

qint64 PacketList::writeString(const QString& string) {
    QByteArray data = string.toUtf8();
    writePrimitive(static_cast<uint32_t>(data.length()));
    return writeData(data.constData(), data.length());
}

qint64 PacketList::writeData(const char* data, qint64 maxSize) {
    auto sizeRemaining = maxSize;

    while (sizeRemaining > 0) {
        if (!_currentPacket) {
            // we don't have a current packet, time to set one up
            _currentPacket = createPacketWithExtendedHeader();
        }

        // check if this block of data can fit into the currentPacket
        if (sizeRemaining <= _currentPacket->bytesAvailableForWrite()) {
            // it fits, just write it to the current packet
            _currentPacket->write(data, sizeRemaining);

            sizeRemaining = 0;
        } else {
            // it does not fit - this may need to be in the next packet

            if (!_isOrdered) {
                auto newPacket = createPacketWithExtendedHeader();

                if (_segmentStartIndex >= 0) {
                    // We in the process of writing a segment for an unordered PacketList.
                    // We need to try and pull the first part of the segment out to our new packet

                    // check now to see if this is an unsupported write
                    int segmentSize = _currentPacket->pos() - _segmentStartIndex;
                    
                    if (segmentSize + sizeRemaining > newPacket->getPayloadCapacity()) {
                        // this is an unsupported case - the segment is bigger than the size of an individual packet
                        // but the PacketList is not going to be sent ordered
                        qCDebug(networking) << "Error in PacketList::writeData - attempted to write a segment to an unordered packet that is"
                            << "larger than the payload size.";
                        Q_ASSERT(false);
                        
                        // we won't be writing this new data to the packet
                        // go back before the current segment and return -1 to indicate error
                        _currentPacket->seek(_segmentStartIndex);
                        _currentPacket->setPayloadSize(_segmentStartIndex);
                        
                        return PACKET_LIST_WRITE_ERROR;
                    } else {
                        // copy from currentPacket where the segment started to the beginning of the newPacket
                        newPacket->write(_currentPacket->getPayload() + _segmentStartIndex, segmentSize);
                        
                        // shrink the current payload to the actual size of the packet
                        _currentPacket->setPayloadSize(_segmentStartIndex);
                        
                        // the current segment now starts at the beginning of the new packet
                        _segmentStartIndex = _extendedHeader.size();
                    }
                }
                
                if (sizeRemaining > newPacket->getPayloadCapacity()) {
                    // this is an unsupported case - attempting to write a block of data larger
                    // than the capacity of a new packet in an unordered PacketList
                    qCDebug(networking) << "Error in PacketList::writeData - attempted to write data to an unordered packet that is"
                        << "larger than the payload size.";
                    Q_ASSERT(false);
                    
                    return PACKET_LIST_WRITE_ERROR;
                }

                // move the current packet to our list of packets
                _packets.push_back(std::move(_currentPacket));

                // write the data to the newPacket
                newPacket->write(data, sizeRemaining);

                // swap our current packet with the new packet
                _currentPacket.swap(newPacket);

                // We've written all of the data, so set sizeRemaining to 0
                sizeRemaining = 0;
            } else {
                // we're an ordered PacketList - let's fit what we can into the current packet and then put the leftover
                // into a new packet

                int numBytesToEnd = _currentPacket->bytesAvailableForWrite();
                _currentPacket->write(data, numBytesToEnd);

                // Remove number of bytes written from sizeRemaining
                sizeRemaining -= numBytesToEnd;

                // Move the data pointer forward
                data += numBytesToEnd;

                // move the current packet to our list of packets
                _packets.push_back(std::move(_currentPacket));
            }
        }
    }

    return maxSize;
}
