//
//  NLPacket.h
//  libraries/networking/src
//
//  Created by Clement on 7/6/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NLPacket_h
#define hifi_NLPacket_h

#include <QtCore/QSharedPointer>

#include "udt/Packet.h"

class NLPacket : public udt::Packet {
    Q_OBJECT
public:
    
    // this is used by the Octree classes - must be known at compile time
    static const int MAX_PACKET_HEADER_SIZE =
        sizeof(udt::Packet::SequenceNumberAndBitField) + sizeof(udt::Packet::MessageNumberAndBitField) +
        sizeof(PacketType) + sizeof(PacketVersion) + NUM_BYTES_RFC4122_UUID + NUM_BYTES_MD5_HASH;
    
    static std::unique_ptr<NLPacket> create(PacketType type, qint64 size = -1,
                                            bool isReliable = false, bool isPartOfMessage = false);
    
    static std::unique_ptr<NLPacket> fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size,
                                                        const HifiSockAddr& senderSockAddr);
    static std::unique_ptr<NLPacket> fromBase(std::unique_ptr<Packet> packet);
    
    // Provided for convenience, try to limit use
    static std::unique_ptr<NLPacket> createCopy(const NLPacket& other);
    
    static PacketType typeInHeader(const udt::Packet& packet);
    static PacketVersion versionInHeader(const udt::Packet& packet);
    
    static QUuid sourceIDInHeader(const udt::Packet& packet);
    static QByteArray verificationHashInHeader(const udt::Packet& packet);
    static QByteArray hashForPacketAndSecret(const udt::Packet& packet, const QUuid& connectionSecret);
    
    static qint64 maxPayloadSize(PacketType type);
    static qint64 localHeaderSize(PacketType type);
    
    virtual qint64 maxPayloadSize() const; // The maximum payload size this packet can use to fit in MTU
    virtual qint64 totalHeadersSize() const; // Cumulated size of all the headers
    virtual qint64 localHeaderSize() const;  // Current level's header size
    
    PacketType getType() const { return _type; }
    void setType(PacketType type);
    
    PacketVersion getVersion() const { return _version; }

    const QUuid& getSourceID() const { return _sourceID; }
    
    void writeSourceID(const QUuid& sourceID);
    void writeVerificationHashGivenSecret(const QUuid& connectionSecret);

protected:
    
    NLPacket(PacketType type, bool forceReliable = false, bool isPartOfMessage = false);
    NLPacket(PacketType type, qint64 size, bool forceReliable = false, bool isPartOfMessage = false);
    NLPacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr);
    NLPacket(std::unique_ptr<Packet> packet);
    
    NLPacket(const NLPacket& other);
    NLPacket(NLPacket&& other);
    
    NLPacket& operator=(const NLPacket& other);
    NLPacket& operator=(NLPacket&& other);
    
    // Header writers
    void writeTypeAndVersion();

    // Header readers, used to set member variables after getting a packet from the network
    void readType();
    void readVersion();
    void readSourceID();
    
    PacketType _type;
    PacketVersion _version;
    QUuid _sourceID;
};

#endif // hifi_NLPacket_h
