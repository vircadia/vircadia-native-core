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

int NLPacket::localHeaderSize(PacketType type) {
    bool nonSourced = NON_SOURCED_PACKETS.contains(type);
    bool nonVerified = NON_VERIFIED_PACKETS.contains(type);
    qint64 optionalSize = (nonSourced ? 0 : NUM_BYTES_RFC4122_UUID) + ((nonSourced || nonVerified) ? 0 : NUM_BYTES_MD5_HASH);
    return sizeof(PacketType) + sizeof(PacketVersion) + optionalSize;
}
int NLPacket::totalHeaderSize(PacketType type, bool isPartOfMessage) {
    return Packet::totalHeaderSize(isPartOfMessage) + NLPacket::localHeaderSize(type);
}
int NLPacket::maxPayloadSize(PacketType type, bool isPartOfMessage) {
    return Packet::maxPayloadSize(isPartOfMessage) - NLPacket::localHeaderSize(type);
}

std::unique_ptr<NLPacket> NLPacket::create(PacketType type, qint64 size, bool isReliable, bool isPartOfMessage) {
    auto packet = std::unique_ptr<NLPacket>(new NLPacket(type, size, isReliable, isPartOfMessage));
        
    packet->open(QIODevice::ReadWrite);
    
    return packet;
}

std::unique_ptr<NLPacket> NLPacket::fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size,
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

std::unique_ptr<NLPacket> NLPacket::fromBase(std::unique_ptr<Packet> packet) {
    // Fail with null packet
    Q_ASSERT(packet);
    
    // call our constructor to create an NLPacket from this Packet
    return std::unique_ptr<NLPacket>(new NLPacket(std::move(*packet)));
}

std::unique_ptr<NLPacket> NLPacket::createCopy(const NLPacket& other) {
    return std::unique_ptr<NLPacket>(new NLPacket(other));
}

NLPacket::NLPacket(PacketType type, qint64 size, bool isReliable, bool isPartOfMessage) :
    Packet((size == -1) ? -1 : NLPacket::localHeaderSize(type) + size, isReliable, isPartOfMessage),
    _type(type),
    _version(versionForPacketType(type))
{
    adjustPayloadStartAndCapacity(NLPacket::localHeaderSize(_type));
    
    writeTypeAndVersion();
}

NLPacket::NLPacket(Packet&& packet) :
    Packet(std::move(packet))
{
    readType();
    readVersion();
    readSourceID();
    
    adjustPayloadStartAndCapacity(NLPacket::localHeaderSize(_type), _payloadSize > 0);
}

NLPacket::NLPacket(const NLPacket& other) : Packet(other) {
    _type = other._type;
    _version = other._version;
    _sourceID = other._sourceID;
}

NLPacket::NLPacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    Packet(std::move(data), size, senderSockAddr)
{    
    // sanity check before we decrease the payloadSize with the payloadCapacity
    Q_ASSERT(_payloadSize == _payloadCapacity);
    
    readType();
    readVersion();
    readSourceID();
    
    adjustPayloadStartAndCapacity(NLPacket::localHeaderSize(_type), _payloadSize > 0);
}

NLPacket::NLPacket(NLPacket&& other) :
    Packet(std::move(other))
{
    _type = other._type;
    _version = other._version;
    _sourceID = std::move(other._sourceID);
}

NLPacket& NLPacket::operator=(const NLPacket& other) {
    Packet::operator=(other);
    
    _type = other._type;
    _version = other._version;
    _sourceID = other._sourceID;
    
    return *this;
}

NLPacket& NLPacket::operator=(NLPacket&& other) {
    
    Packet::operator=(std::move(other));
    
    _type = other._type;
    _version = other._version;
    _sourceID = std::move(other._sourceID);
    
    return *this;
}

PacketType NLPacket::typeInHeader(const udt::Packet& packet) {
    auto headerOffset = Packet::totalHeaderSize(packet.isPartOfMessage());
    return *reinterpret_cast<const PacketType*>(packet.getData() + headerOffset);
}

PacketVersion NLPacket::versionInHeader(const udt::Packet& packet) {
    auto headerOffset = Packet::totalHeaderSize(packet.isPartOfMessage());
    return *reinterpret_cast<const PacketVersion*>(packet.getData() + headerOffset + sizeof(PacketType));
}

QUuid NLPacket::sourceIDInHeader(const udt::Packet& packet) {
    int offset = Packet::totalHeaderSize(packet.isPartOfMessage()) + sizeof(PacketType) + sizeof(PacketVersion);
    return QUuid::fromRfc4122(QByteArray::fromRawData(packet.getData() + offset, NUM_BYTES_RFC4122_UUID));
}

QByteArray NLPacket::verificationHashInHeader(const udt::Packet& packet) {
    int offset = Packet::totalHeaderSize(packet.isPartOfMessage()) + sizeof(PacketType) + sizeof(PacketVersion) + NUM_BYTES_RFC4122_UUID;
    return QByteArray(packet.getData() + offset, NUM_BYTES_MD5_HASH);
}

QByteArray NLPacket::hashForPacketAndSecret(const udt::Packet& packet, const QUuid& connectionSecret) {
    QCryptographicHash hash(QCryptographicHash::Md5);
    
    int offset = Packet::totalHeaderSize(packet.isPartOfMessage()) + sizeof(PacketType) + sizeof(PacketVersion)
        + NUM_BYTES_RFC4122_UUID + NUM_BYTES_MD5_HASH;
    
    // add the packet payload and the connection UUID
    hash.addData(packet.getData() + offset, packet.getDataSize() - offset);
    hash.addData(connectionSecret.toRfc4122());
    
    // return the hash
    return hash.result();
}

void NLPacket::writeTypeAndVersion() {
    auto headerOffset = Packet::totalHeaderSize(isPartOfMessage());
    
    // Pack the packet type
    memcpy(_packet.get() + headerOffset, &_type, sizeof(PacketType));
    
    // Pack the packet version
    memcpy(_packet.get() + headerOffset + sizeof(PacketType), &_version, sizeof(_version));
}

void NLPacket::setType(PacketType type) {
    // Setting new packet type with a different header size not currently supported
    Q_ASSERT(NLPacket::totalHeaderSize(_type, isPartOfMessage()) ==
             NLPacket::totalHeaderSize(type, isPartOfMessage()));
    
    _type = type;
    _version = versionForPacketType(_type);
    
    writeTypeAndVersion();
}

void NLPacket::readType() {
    _type = NLPacket::typeInHeader(*this);
}

void NLPacket::readVersion() {
    _version = NLPacket::versionInHeader(*this);
}

void NLPacket::readSourceID() {
    if (!NON_SOURCED_PACKETS.contains(_type)) {
        _sourceID = sourceIDInHeader(*this);
    }
}

void NLPacket::writeSourceID(const QUuid& sourceID) const {
    Q_ASSERT(!NON_SOURCED_PACKETS.contains(_type));
    
    auto offset = Packet::totalHeaderSize(isPartOfMessage()) + sizeof(PacketType) + sizeof(PacketVersion);
    memcpy(_packet.get() + offset, sourceID.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
    
    _sourceID = sourceID;
}

void NLPacket::writeVerificationHashGivenSecret(const QUuid& connectionSecret) const {
    Q_ASSERT(!NON_SOURCED_PACKETS.contains(_type) && !NON_VERIFIED_PACKETS.contains(_type));
    
    auto offset = Packet::totalHeaderSize(isPartOfMessage()) + sizeof(PacketType) + sizeof(PacketVersion)
                + NUM_BYTES_RFC4122_UUID;
    QByteArray verificationHash = hashForPacketAndSecret(*this, connectionSecret);
    
    memcpy(_packet.get() + offset, verificationHash.data(), verificationHash.size());
}
