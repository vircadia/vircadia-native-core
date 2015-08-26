//
//  BasePacket.cpp
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-23.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BasePacket.h"

using namespace udt;

const qint64 BasePacket::PACKET_WRITE_ERROR = -1;

int BasePacket::localHeaderSize() {
    return 0;
}
int BasePacket::totalHeaderSize() {
    return 0;
}
int BasePacket::maxPayloadSize() {
    return MAX_PACKET_SIZE;
}

std::unique_ptr<BasePacket> BasePacket::create(qint64 size) {
    auto packet = std::unique_ptr<BasePacket>(new BasePacket(size));
    
    packet->open(QIODevice::ReadWrite);
    
    return packet;
}

std::unique_ptr<BasePacket> BasePacket::fromReceivedPacket(std::unique_ptr<char[]> data,
                                                           qint64 size, const HifiSockAddr& senderSockAddr) {
    // Fail with invalid size
    Q_ASSERT(size >= 0);
    
    // allocate memory
    auto packet = std::unique_ptr<BasePacket>(new BasePacket(std::move(data), size, senderSockAddr));
    
    packet->open(QIODevice::ReadOnly);
    
    return packet;
}

BasePacket::BasePacket(qint64 size) {
    auto maxPayload = BasePacket::maxPayloadSize();
    
    if (size == -1) {
        // default size of -1, means biggest packet possible
        size = maxPayload;
    }
    
    // Sanity check
    Q_ASSERT(size >= 0 || size < maxPayload);
    
    _packetSize = size;
    _packet.reset(new char[_packetSize]);
    _payloadCapacity = _packetSize;
    _payloadSize = 0;
    _payloadStart = _packet.get();
}

BasePacket::BasePacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    _packetSize(size),
    _packet(std::move(data)),
    _payloadStart(_packet.get()),
    _payloadCapacity(size),
    _payloadSize(size),
    _senderSockAddr(senderSockAddr)
{
    
}

BasePacket::BasePacket(const BasePacket& other) :
    QIODevice()
{
    *this = other;
}

BasePacket& BasePacket::operator=(const BasePacket& other) {
    _packetSize = other._packetSize;
    _packet = std::unique_ptr<char[]>(new char[_packetSize]);
    memcpy(_packet.get(), other._packet.get(), _packetSize);
    
    _payloadStart = _packet.get() + (other._payloadStart - other._packet.get());
    _payloadCapacity = other._payloadCapacity;
    
    _payloadSize = other._payloadSize;
    
    _senderSockAddr = other._senderSockAddr;
    
    if (other.isOpen() && !isOpen()) {
        open(other.openMode());
    }
    
    seek(other.pos());
    
    return *this;
}

BasePacket::BasePacket(BasePacket&& other) {
    *this = std::move(other);
}

BasePacket& BasePacket::operator=(BasePacket&& other) {
    _packetSize = other._packetSize;
    _packet = std::move(other._packet);
    
    _payloadStart = other._payloadStart;
    _payloadCapacity = other._payloadCapacity;
    
    _payloadSize = other._payloadSize;
    
    _senderSockAddr = std::move(other._senderSockAddr);
    
    if (other.isOpen() && !isOpen()) {
        open(other.openMode());
    }
    
    seek(other.pos());
    
    return *this;
}

qint64 BasePacket::getDataSize() const {
    return (_payloadStart - _packet.get()) + _payloadSize;
}

void BasePacket::setPayloadSize(qint64 payloadSize) {
    if (isWritable()) {
        Q_ASSERT(payloadSize <= _payloadCapacity);
        _payloadSize = payloadSize;
    } else {
        qDebug() << "You can not call setPayloadSize for a non-writeable Packet.";
        Q_ASSERT(false);
    }
}

QByteArray BasePacket::read(qint64 maxSize) {
    qint64 sizeToRead = std::min(size() - pos(), maxSize);
    QByteArray data { getPayload() + pos(), (int) sizeToRead };
    seek(pos() + sizeToRead);
    return data;
}

QByteArray BasePacket::readWithoutCopy(qint64 maxSize) {
    qint64 sizeToRead = std::min(size() - pos(), maxSize);
    QByteArray data { QByteArray::fromRawData(getPayload() + pos(), sizeToRead) };
    seek(pos() + sizeToRead);
    return data;
}

bool BasePacket::reset() {
    if (isWritable()) {
        _payloadSize = 0;
    }
    
    return QIODevice::reset();
}

qint64 BasePacket::writeData(const char* data, qint64 maxSize) {
   
    Q_ASSERT_X(maxSize <= bytesAvailableForWrite(), "BasePacket::writeData", "not enough space for write");
    
    // make sure we have the space required to write this block
    if (maxSize <= bytesAvailableForWrite()) {
        qint64 currentPos = pos();

        // good to go - write the data
        memcpy(_payloadStart + currentPos, data, maxSize);
        
        // keep track of _payloadSize so we can just write the actual data when packet is about to be sent
        _payloadSize = std::max(currentPos + maxSize, _payloadSize);
        
        // return the number of bytes written
        return maxSize;
    } else {
        // not enough space left for this write - return an error
        return PACKET_WRITE_ERROR;
    }
}

qint64 BasePacket::readData(char* dest, qint64 maxSize) {
    // we're either reading what is left from the current position or what was asked to be read
    qint64 numBytesToRead = std::min(bytesLeftToRead(), maxSize);
    
    if (numBytesToRead > 0) {
        int currentPosition = pos();
        
        // read out the data
        memcpy(dest, _payloadStart + currentPosition, numBytesToRead);
    }
    
    return numBytesToRead;
}

void BasePacket::adjustPayloadStartAndCapacity(qint64 headerSize, bool shouldDecreasePayloadSize) {
    _payloadStart += headerSize;
    _payloadCapacity -= headerSize;
    
    if (shouldDecreasePayloadSize) {
        _payloadSize -= headerSize;
    }
}
