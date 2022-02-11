//
//  ReceivedMessage.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/09/17
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ReceivedMessage.h"

#include <algorithm>
#include <chrono>

int receivedMessageMetaTypeId = qRegisterMetaType<ReceivedMessage*>("ReceivedMessage*");
int sharedPtrReceivedMessageMetaTypeId = qRegisterMetaType<QSharedPointer<ReceivedMessage>>("QSharedPointer<ReceivedMessage>");

static const int HEAD_DATA_SIZE = 512;

using namespace std::chrono;

ReceivedMessage::ReceivedMessage(const NLPacketList& packetList)
    : _data(packetList.getMessage()),
      _headData(_data.mid(0, HEAD_DATA_SIZE)),
      _numPackets(packetList.getNumPackets()),
      _sourceID(packetList.getSourceID()),
      _packetType(packetList.getType()),
      _packetVersion(packetList.getVersion()),
      _senderSockAddr(packetList.getSenderSockAddr())
{
    _firstPacketReceiveTime = duration_cast<microseconds>(packetList.getFirstPacketReceiveTime().time_since_epoch()).count();
}

ReceivedMessage::ReceivedMessage(NLPacket& packet)
    : _data(packet.readAll()),
      _headData(_data.mid(0, HEAD_DATA_SIZE)),
      _numPackets(1),
      _sourceID(packet.getSourceID()),
      _packetType(packet.getType()),
      _packetVersion(packet.getVersion()),
      _senderSockAddr(packet.getSenderSockAddr()),
      _isComplete(packet.getPacketPosition() == NLPacket::ONLY)
{
    _firstPacketReceiveTime = duration_cast<microseconds>(packet.getReceiveTime().time_since_epoch()).count();
}

ReceivedMessage::ReceivedMessage(QByteArray byteArray, PacketType packetType, PacketVersion packetVersion,
                const SockAddr& senderSockAddr, NLPacket::LocalID sourceID) :
    _data(byteArray),
    _headData(_data.mid(0, HEAD_DATA_SIZE)),
    _numPackets(1),
    _firstPacketReceiveTime(0),
    _sourceID(sourceID),
    _packetType(packetType),
    _packetVersion(packetVersion),
    _senderSockAddr(senderSockAddr),
    _isComplete(true)
{
}

void ReceivedMessage::setFailed() {
    _failed = true;
    _isComplete = true;
    emit completed();
}

void ReceivedMessage::appendPacket(NLPacket& packet) {
    Q_ASSERT_X(!_isComplete, "ReceivedMessage::appendPacket", 
               "We should not be appending to a complete message");

    // Limit progress signal to every X packets
    const int EMIT_PROGRESS_EVERY_X_PACKETS = 50;

    ++_numPackets;

    _data.append(packet.getPayload(), packet.getPayloadSize());

    if (_numPackets % EMIT_PROGRESS_EVERY_X_PACKETS == 0) {
        emit progress(getSize());
    }

    auto packetPosition = packet.getPacketPosition();
    if ((packetPosition == NLPacket::PacketPosition::FIRST) ||
        (packetPosition == NLPacket::PacketPosition::ONLY)) {
        _firstPacketReceiveTime = duration_cast<microseconds>(packet.getReceiveTime().time_since_epoch()).count();
    }

    if (packetPosition == NLPacket::PacketPosition::LAST) {
        _isComplete = true;
        emit completed();
    }
}

qint64 ReceivedMessage::peek(char* data, qint64 size) {
    size_t bytesLeft = _data.size() - _position;
    size_t sizeRead = std::min((size_t)size, bytesLeft);
    memcpy(data, _data.constData() + _position, sizeRead);
    return sizeRead;
}

qint64 ReceivedMessage::read(char* data, qint64 size) {
    size_t bytesLeft = _data.size() - _position;
    size_t sizeRead = std::min((size_t)size, bytesLeft);
    memcpy(data, _data.constData() + _position, sizeRead);
    _position += sizeRead;
    return sizeRead;
}

qint64 ReceivedMessage::readHead(char* data, qint64 size) {
    size_t bytesLeft = _headData.size() - _position;
    size_t sizeRead = std::min((size_t)size, bytesLeft);
    memcpy(data, _headData.constData() + _position, sizeRead);
    _position += sizeRead;
    return sizeRead;
}

QByteArray ReceivedMessage::peek(qint64 size) {
    return _data.mid(_position, size);
}

QByteArray ReceivedMessage::read(qint64 size) {
    auto data = _data.mid(_position, size);
    _position += size;
    return data;
}

QByteArray ReceivedMessage::readHead(qint64 size) {
    auto data = _headData.mid(_position, size);
    _position += size;
    return data;
}

QByteArray ReceivedMessage::readAll() {
    return read(getBytesLeftToRead());
}

QString ReceivedMessage::readString() {
    uint32_t size;
    readPrimitive(&size);
    //Q_ASSERT(size <= _size - _position);
    auto string = QString::fromUtf8(_data.constData() + _position, size);
    _position += size;
    return string;
}

QByteArray ReceivedMessage::readWithoutCopy(qint64 size) {
    QByteArray data { QByteArray::fromRawData(_data.constData() + _position, size) };
    _position += size;
    return data;
}

void ReceivedMessage::onComplete() {
    _isComplete = true;
    emit completed();
}
