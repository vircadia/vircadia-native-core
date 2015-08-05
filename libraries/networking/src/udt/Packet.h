//
//  Packet.h
//  libraries/networking/src/udt
//
//  Created by Clement on 7/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Packet_h
#define hifi_Packet_h

#include <memory>

#include <QtCore/QIODevice>

#include "BasePacket.h"
#include "PacketHeaders.h"

namespace udt {

class Packet : public BasePacket {
    Q_OBJECT
public:
    // NOTE: The SequenceNumber is only actually 29 bits to leave room for a bit field
    using SequenceNumber = uint32_t;
    using SequenceNumberAndBitField = uint32_t;
    
    // NOTE: The MessageNumber is only actually 29 bits to leave room for a bit field
    using MessageNumber = uint32_t;
    using MessageNumberAndBitField = uint32_t;
    
    static const uint32_t DEFAULT_SEQUENCE_NUMBER = 0;

    static std::unique_ptr<Packet> create(qint64 size = -1, bool isReliable = false, bool isPartOfMessage = false);
    static std::unique_ptr<Packet> fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr);
    
    // Provided for convenience, try to limit use
    static std::unique_ptr<Packet> createCopy(const Packet& other);
    
    // The maximum payload size this packet can use to fit in MTU
    static qint64 maxPayloadSize(bool isPartOfMessage);
    virtual qint64 maxPayloadSize() const;
    
    // Current level's header size
    static qint64 localHeaderSize(bool isPartOfMessage);
    virtual qint64 localHeaderSize() const;
    
    virtual qint64 totalHeadersSize() const; // Cumulated size of all the headers

    void writeSequenceNumber(SequenceNumber sequenceNumber);

protected:
    Packet(qint64 size, bool isReliable = false, bool isPartOfMessage = false);
    Packet(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr);
    
    Packet(const Packet& other);
    Packet(Packet&& other);
    
    Packet& operator=(const Packet& other);
    Packet& operator=(Packet&& other);

    // Header readers - these read data to member variables after pulling packet off wire
    void readIsPartOfMessage();
    void readIsReliable();
    void readSequenceNumber();

    bool _isReliable { false };
    bool _isPartOfMessage { false };
    SequenceNumber _sequenceNumber { 0 };
};
    
} // namespace udt

#endif // hifi_Packet_h
