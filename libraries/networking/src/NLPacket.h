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
    static std::unique_ptr<NLPacket> create(PacketType::Value type, qint64 size = -1);
    static std::unique_ptr<NLPacket> fromReceivedPacket(std::unique_ptr<char> data, qint64 size,
                                                        const HifiSockAddr& senderSockAddr);
    static std::unique_ptr<NLPacket> fromBase(std::unique_ptr<Packet> packet);
    
    // Provided for convenience, try to limit use
    static std::unique_ptr<NLPacket> createCopy(const NLPacket& other);
    
    static QUuid sourceIDInHeader(const udt::Packet& packet);
    static QByteArray verificationHashInHeader(const udt::Packet& packet);
    static QByteArray hashForPacketAndSecret(const udt::Packet& packet, const QUuid& connectionSecret);

    static qint64 localHeaderSize(PacketType::Value type);
    static qint64 maxPayloadSize(PacketType::Value type);

    virtual qint64 totalHeadersSize() const; // Cumulated size of all the headers
    virtual qint64 localHeaderSize() const;  // Current level's header size

    const QUuid& getSourceID() const { return _sourceID; }
    
    void writeSourceID(const QUuid& sourceID);
    void writeVerificationHashGivenSecret(const QUuid& connectionSecret);

protected:
    
    void adjustPayloadStartAndCapacity(bool shouldDecreasePayloadSize = false);
    
    NLPacket(PacketType::Value type);
    NLPacket(PacketType::Value type, qint64 size);
    
    NLPacket(std::unique_ptr<char> data, qint64 size, const HifiSockAddr& senderSockAddr);
    NLPacket(std::unique_ptr<Packet> packet);
    
    NLPacket(const NLPacket& other);
    NLPacket& operator=(const NLPacket& other);
    
    NLPacket(NLPacket&& other);
    NLPacket& operator=(NLPacket&& other);

    void readSourceID();

    QUuid _sourceID;
};

#endif // hifi_NLPacket_h
