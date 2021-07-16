//
//  NLPacket.cpp
//  libraries/networking/src
//
//  Created by Clement on 7/6/15.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NLPacket.h"

#include "HMACAuth.h"

int NLPacket::localHeaderSize(PacketType type) {
    bool nonSourced = PacketTypeEnum::getNonSourcedPackets().contains(type);
    bool nonVerified = PacketTypeEnum::getNonVerifiedPackets().contains(type);
    qint64 optionalSize = (nonSourced ? 0 : NUM_BYTES_LOCALID) + ((nonSourced || nonVerified) ? 0 : NUM_BYTES_MD5_HASH);
    return sizeof(PacketType) + sizeof(PacketVersion) + optionalSize;
}
int NLPacket::totalHeaderSize(PacketType type, bool isPartOfMessage) {
    return Packet::totalHeaderSize(isPartOfMessage) + NLPacket::localHeaderSize(type);
}
int NLPacket::maxPayloadSize(PacketType type, bool isPartOfMessage) {
    return Packet::maxPayloadSize(isPartOfMessage) - NLPacket::localHeaderSize(type);
}

std::unique_ptr<NLPacket> NLPacket::create(PacketType type, qint64 size, bool isReliable, bool isPartOfMessage, PacketVersion version) {
    auto packet = std::unique_ptr<NLPacket>(new NLPacket(type, size, isReliable, isPartOfMessage, version));
        
    packet->open(QIODevice::ReadWrite);
    
    return packet;
}

std::unique_ptr<NLPacket> NLPacket::fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size,
                                                       const SockAddr& senderSockAddr) {
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

NLPacket::NLPacket(PacketType type, qint64 size, bool isReliable, bool isPartOfMessage, PacketVersion version) :
    Packet((size == -1) ? -1 : NLPacket::localHeaderSize(type) + size, isReliable, isPartOfMessage),
    _type(type),
    _version((version == 0) ? versionForPacketType(type) : version)
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

NLPacket::NLPacket(std::unique_ptr<char[]> data, qint64 size, const SockAddr& senderSockAddr) :
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

NLPacket::LocalID NLPacket::sourceIDInHeader(const udt::Packet& packet) {
    int offset = Packet::totalHeaderSize(packet.isPartOfMessage()) + sizeof(PacketType) + sizeof(PacketVersion);
    return *reinterpret_cast<const LocalID*>(packet.getData() + offset);
}

QByteArray NLPacket::verificationHashInHeader(const udt::Packet& packet) {
    int offset = Packet::totalHeaderSize(packet.isPartOfMessage()) + sizeof(PacketType) +
        sizeof(PacketVersion) + NUM_BYTES_LOCALID;
    return QByteArray(packet.getData() + offset, NUM_BYTES_MD5_HASH);
}

QByteArray NLPacket::hashForPacketAndHMAC(const udt::Packet& packet, HMACAuth& hash) {
    int offset = Packet::totalHeaderSize(packet.isPartOfMessage()) + sizeof(PacketType) + sizeof(PacketVersion)
        + NUM_BYTES_LOCALID + NUM_BYTES_MD5_HASH;
    
    // add the packet payload and the connection UUID
    HMACAuth::HMACHash hashResult;
    if (!hash.calculateHash(hashResult, packet.getData() + offset, packet.getDataSize() - offset)) {
        return QByteArray();
    }
    return QByteArray((const char*) hashResult.data(), (int) hashResult.size());
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

void NLPacket::setVersion(PacketVersion version) {
    _version = version;
    writeTypeAndVersion();
}

void NLPacket::readType() {
    _type = NLPacket::typeInHeader(*this);
}

void NLPacket::readVersion() {
    _version = NLPacket::versionInHeader(*this);
}

void NLPacket::readSourceID() {
    if (PacketTypeEnum::getNonSourcedPackets().contains(_type)) {
        _sourceID = NULL_LOCAL_ID;
    } else {
        _sourceID = sourceIDInHeader(*this);
    }
}

void NLPacket::writeSourceID(LocalID sourceID) const {
    Q_ASSERT(!PacketTypeEnum::getNonSourcedPackets().contains(_type));
    
    auto offset = Packet::totalHeaderSize(isPartOfMessage()) + sizeof(PacketType) + sizeof(PacketVersion);

    memcpy(_packet.get() + offset, &sourceID, sizeof(sourceID));
    
    _sourceID = sourceID;
}

void NLPacket::writeVerificationHash(HMACAuth& hmacAuth) const {
    Q_ASSERT(!PacketTypeEnum::getNonSourcedPackets().contains(_type) &&
             !PacketTypeEnum::getNonVerifiedPackets().contains(_type));
    
    auto offset = Packet::totalHeaderSize(isPartOfMessage()) + sizeof(PacketType) + sizeof(PacketVersion)
                + NUM_BYTES_LOCALID;

    QByteArray verificationHash = hashForPacketAndHMAC(*this, hmacAuth);
    
    memcpy(_packet.get() + offset, verificationHash.data(), verificationHash.size());
}
