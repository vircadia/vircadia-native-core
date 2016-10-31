//
//  ReceivedMessage.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/09/17
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "ReceivedMessage.h"

#include "QSharedPointer"

int receivedMessageMetaTypeId = qRegisterMetaType<ReceivedMessage*>("ReceivedMessage*");
int sharedPtrReceivedMessageMetaTypeId = qRegisterMetaType<QSharedPointer<ReceivedMessage>>("QSharedPointer<ReceivedMessage>");

static const int HEAD_DATA_SIZE = 512;

ReceivedMessage::ReceivedMessage(const NLPacketList& packetList)
    : _data(packetList.getMessage()),
      _headData(_data.mid(0, HEAD_DATA_SIZE)),
      _numPackets(packetList.getNumPackets()),
      _sourceID(packetList.getSourceID()),
      _packetType(packetList.getType()),
      _packetVersion(packetList.getVersion()),
      _senderSockAddr(packetList.getSenderSockAddr()),
      _isComplete(true)
{
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

    if (packet.getPacketPosition() == NLPacket::PacketPosition::LAST) {
        _isComplete = true;
        emit completed();
    }
}

qint64 ReceivedMessage::peek(char* data, qint64 size) {
    memcpy(data, _data.constData() + _position, size);
    return size;
}

qint64 ReceivedMessage::read(char* data, qint64 size) {
    memcpy(data, _data.constData() + _position, size);
    _position += size;
    return size;
}

qint64 ReceivedMessage::readHead(char* data, qint64 size) {
    memcpy(data, _headData.constData() + _position, size);
    _position += size;
    return size;
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
