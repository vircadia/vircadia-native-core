//
//  NLPacket.cpp
//  libraries/networking/src
//
//  Created by Clement on 7/6/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NLPacket.h"

qint64 NLPacket::localHeaderSize(PacketType::Value type) {
    qint64 size = ((NON_SOURCED_PACKETS.contains(type)) ? 0 : NUM_BYTES_RFC4122_UUID) +
                    ((NON_SOURCED_PACKETS.contains(type) || NON_VERIFIED_PACKETS.contains(type)) ? 0 : NUM_BYTES_MD5_HASH);
    return size;
}

qint64 NLPacket::maxPayloadSize(PacketType::Value type) {
    return Packet::maxPayloadSize(type) - localHeaderSize(type);
}

qint64 NLPacket::totalHeadersSize() const {
    return localHeaderSize() + Packet::localHeaderSize();
}

qint64 NLPacket::localHeaderSize() const {
    return localHeaderSize(_type);
}

std::unique_ptr<NLPacket> NLPacket::create(PacketType::Value type, qint64 size) {
    std::unique_ptr<NLPacket> packet;
    
    if (size == -1) {
        packet = std::unique_ptr<NLPacket>(new NLPacket(type));
    } else {
        // Fail with invalid size
        Q_ASSERT(size >= 0);

        packet = std::unique_ptr<NLPacket>(new NLPacket(type, size));
    }
        
    packet->open(QIODevice::ReadWrite);
    
    return packet;
}

std::unique_ptr<NLPacket> NLPacket::fromReceivedPacket(std::unique_ptr<char> data, qint64 size,
                                                       const HifiSockAddr& senderSockAddr) {
    // Fail with null data
    Q_ASSERT(data);
    
    // Fail with invalid size
    Q_ASSERT(size >= 0);

    // allocate memory
    auto packet = std::unique_ptr<NLPacket>(new NLPacket(std::move(data), size, senderSockAddr));

    packet->open(QIODevice::ReadOnly);

    return packet;
 
}

std::unique_ptr<NLPacket> NLPacket::createCopy(const NLPacket& other) {
    return std::unique_ptr<NLPacket>(new NLPacket(other));
}

NLPacket::NLPacket(PacketType::Value type, qint64 size) :
    Packet(type, localHeaderSize(type) + size)
{
    Q_ASSERT(size >= 0);
    
    adjustPayloadStartAndCapacity();
}

NLPacket::NLPacket(PacketType::Value type) :
    Packet(type, -1)
{
    adjustPayloadStartAndCapacity();
}

NLPacket::NLPacket(const NLPacket& other) : Packet(other) {
    
}

NLPacket::NLPacket(std::unique_ptr<char> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    Packet(std::move(data), size, senderSockAddr)
{
    adjustPayloadStartAndCapacity();
    _payloadSize = _payloadCapacity;
    
    readSourceID();
    readVerificationHash();
}

void NLPacket::adjustPayloadStartAndCapacity() {
    qint64 headerSize = localHeaderSize(_type);
    _payloadStart += headerSize;
    _payloadCapacity -= headerSize;
}

void NLPacket::readSourceID() {
    if (!NON_SOURCED_PACKETS.contains(_type)) {
        auto offset = Packet::localHeaderSize();
        _sourceID = QUuid::fromRfc4122(QByteArray::fromRawData(_packet.get() + offset, NUM_BYTES_RFC4122_UUID));
    }
}

void NLPacket::readVerificationHash() {
    if (!NON_SOURCED_PACKETS.contains(_type) && !NON_VERIFIED_PACKETS.contains(_type)) {
        auto offset = Packet::localHeaderSize() + NUM_BYTES_RFC4122_UUID;
        _verificationHash = QByteArray(_packet.get() + offset, NUM_BYTES_MD5_HASH);
    }
}

void NLPacket::writeSourceID(const QUuid& sourceID) {
    Q_ASSERT(!NON_SOURCED_PACKETS.contains(_type));
    
    auto offset = Packet::localHeaderSize();
    memcpy(_packet.get() + offset, sourceID.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
    
    _sourceID = sourceID;
}

void NLPacket::writeVerificationHash(const QByteArray& verificationHash) {
    Q_ASSERT(!NON_SOURCED_PACKETS.contains(_type) && !NON_VERIFIED_PACKETS.contains(_type));

    auto offset = Packet::localHeaderSize() + NUM_BYTES_RFC4122_UUID;
    memcpy(_packet.get() + offset, verificationHash.data(), verificationHash.size());
    
    _verificationHash = verificationHash;
}

QByteArray NLPacket::payloadHashWithConnectionUUID(const QUuid& connectionUUID) const {
    QCryptographicHash hash(QCryptographicHash::Md5);
    
    // add the packet payload and the connection UUID
    hash.addData(_payloadStart, _payloadSize);
    hash.addData(connectionUUID.toRfc4122());
    
    // return the hash
    return hash.result();
}
