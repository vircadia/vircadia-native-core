//
//  NLPacket.h
//  libraries/networking/src
//
//  Created by Clement on 7/6/15.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NLPacket_h
#define hifi_NLPacket_h

#include <UUID.h>

#include "udt/Packet.h"

class HMACAuth;

class NLPacket : public udt::Packet {
    Q_OBJECT
public:
    //
    //    NLPacket format:
    //
    //    |      BYTE     |      BYTE     |      BYTE     |      BYTE     |
    //     0                   1                   2                   3
    //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |  Packet Type  |    Version    | Local Node ID - sourced only  |
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |                                                               |
    //    |                 MD5 Verification - 16 bytes                   |
    //    |                 (ONLY FOR VERIFIED PACKETS)                   |
    //    |                                                               |
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //

    using LocalID = NetworkLocalID;
    static const LocalID NULL_LOCAL_ID = 0;
    
    static const int NUM_BYTES_LOCALID = sizeof(LocalID);
    // this is used by the Octree classes - must be known at compile time
    static const int MAX_PACKET_HEADER_SIZE =
        sizeof(udt::Packet::SequenceNumberAndBitField) + sizeof(udt::Packet::MessageNumberAndBitField) +
        sizeof(PacketType) + sizeof(PacketVersion) + NUM_BYTES_LOCALID + NUM_BYTES_MD5_HASH;
    
    static std::unique_ptr<NLPacket> create(PacketType type, qint64 size = -1,
                    bool isReliable = false, bool isPartOfMessage = false, PacketVersion version = 0);
    
    static std::unique_ptr<NLPacket> fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size,
                                                        const SockAddr& senderSockAddr);

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
    
    static LocalID sourceIDInHeader(const udt::Packet& packet);
    static QByteArray verificationHashInHeader(const udt::Packet& packet);
    static QByteArray hashForPacketAndHMAC(const udt::Packet& packet, HMACAuth& hash);
    
    PacketType getType() const { return _type; }
    void setType(PacketType type);
    
    PacketVersion getVersion() const { return _version; }
    void setVersion(PacketVersion version);

    LocalID getSourceID() const { return _sourceID; }
    
    void writeSourceID(LocalID sourceID) const;
    void writeVerificationHash(HMACAuth& hmacAuth) const;

protected:
    
    NLPacket(PacketType type, qint64 size = -1, bool forceReliable = false, bool isPartOfMessage = false, PacketVersion version = 0);
    NLPacket(std::unique_ptr<char[]> data, qint64 size, const SockAddr& senderSockAddr);
    
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
    mutable LocalID _sourceID;
};

#endif // hifi_NLPacket_h
