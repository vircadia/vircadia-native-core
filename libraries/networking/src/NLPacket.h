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

#include <UUID.h>

#include "udt/Packet.h"

class NLPacket : public udt::Packet {
    Q_OBJECT
public:

    //     0                   1                   2                   3
    //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |          Packet Type          |        Packet Version         |
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |          Node UUID            |    Hash (only if verified)    |  Optional (only if sourced)
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    //    NLPacket Header Format

    // this is used by the Octree classes - must be known at compile time
    static const int MAX_PACKET_HEADER_SIZE =
        sizeof(udt::Packet::SequenceNumberAndBitField) + sizeof(udt::Packet::MessageNumberAndBitField) +
        sizeof(PacketType) + sizeof(PacketVersion) + NUM_BYTES_RFC4122_UUID + NUM_BYTES_MD5_HASH;
    
    static std::unique_ptr<NLPacket> create(PacketType type, qint64 size = -1,
                    bool isReliable = false, bool isPartOfMessage = false, PacketVersion version = 0);
    
    static std::unique_ptr<NLPacket> fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size,
                                                        const HifiSockAddr& senderSockAddr);
    static std::unique_ptr<NLPacket> fromBase(std::unique_ptr<Packet> packet);
    
    // Provided for convenience, try to limit use
    static std::unique_ptr<NLPacket> createCopy(const NLPacket& other);
    
    // Current level's header size
    static int localHeaderSize(PacketType type);
    // Cumulated size of all the headers
    static int totalHeaderSize(PacketType type, bool isPartOfMessage = false);
    // The maximum payload size this packet can use to fit in MTU
    static int maxPayloadSize(PacketType type, bool isPartOfMessage = false);
    
    static PacketType typeInHeader(const udt::Packet& packet);
    static PacketVersion versionInHeader(const udt::Packet& packet);
    
    static QUuid sourceIDInHeader(const udt::Packet& packet);
    static QByteArray verificationHashInHeader(const udt::Packet& packet);
    static QByteArray hashForPacketAndSecret(const udt::Packet& packet, const QUuid& connectionSecret);
    
    PacketType getType() const { return _type; }
    void setType(PacketType type);
    
    PacketVersion getVersion() const { return _version; }
    void setVersion(PacketVersion version);

    const QUuid& getSourceID() const { return _sourceID; }
    
    void writeSourceID(const QUuid& sourceID) const;
    void writeVerificationHashGivenSecret(const QUuid& connectionSecret) const;

protected:
    
    NLPacket(PacketType type, qint64 size = -1, bool forceReliable = false, bool isPartOfMessage = false, PacketVersion version = 0);
    NLPacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr);
    
    NLPacket(const NLPacket& other);
    NLPacket(NLPacket&& other);
    NLPacket(Packet&& other);
    
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
    mutable QUuid _sourceID;
};

#endif // hifi_NLPacket_h
