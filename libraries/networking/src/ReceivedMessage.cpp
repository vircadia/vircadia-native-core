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

static int receivedMessageMetaTypeId = qRegisterMetaType<ReceivedMessage*>("ReceivedMessage*");
static int sharedPtrReceivedMessageMetaTypeId = qRegisterMetaType<QSharedPointer<ReceivedMessage>>("QSharedPointer<ReceivedMessage>");

ReceivedMessage::ReceivedMessage(const NLPacketList& packetList)
    : _data(packetList.getMessage()),
      _sourceID(packetList.getSourceID()),
      _numPackets(packetList.getNumPackets()),
      _packetType(packetList.getType()),
      _packetVersion(packetList.getVersion()),
      _senderSockAddr(packetList.getSenderSockAddr())
{
}

ReceivedMessage::ReceivedMessage(NLPacket& packet)
    : _data(packet.readAll()),
      _sourceID(packet.getSourceID()),
      _numPackets(1),
      _packetType(packet.getType()),
      _packetVersion(packet.getVersion()),
      _senderSockAddr(packet.getSenderSockAddr()),
      _isComplete(packet.getPacketPosition() == NLPacket::ONLY)
{
}

void ReceivedMessage::appendPacket(std::unique_ptr<NLPacket> packet) {
    ++_numPackets;

    _data.append(packet->getPayload(), packet->getPayloadSize());
    emit progress(this);
    if (packet->getPacketPosition() == NLPacket::PacketPosition::LAST) {
        _isComplete = true;
        emit completed(this);
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

QByteArray ReceivedMessage::peek(qint64 size) {
    return _data.mid(_position, size);
}

QByteArray ReceivedMessage::read(qint64 size) {
    auto data = _data.mid(_position, size);
    _position += size;
    return data;
}

QByteArray ReceivedMessage::readAll() {
    return read(getBytesLeftToRead());
}

QByteArray ReceivedMessage::readWithoutCopy(qint64 size) {
    QByteArray data { QByteArray::fromRawData(_data.constData() + _position, size) };
    _position += size;
    return data;
}

void ReceivedMessage::onComplete() {
    _isComplete = true;
    emit completed(this);
}
