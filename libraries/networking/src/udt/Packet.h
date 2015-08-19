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
#include "SequenceNumber.h"

namespace udt {

class Packet : public BasePacket {
    Q_OBJECT
public:
    // NOTE: The SequenceNumber is only actually 29 bits to leave room for a bit field
    using SequenceNumberAndBitField = uint32_t;
    
    // NOTE: The MessageNumber is only actually 29 bits to leave room for a bit field
    using MessageNumber = uint32_t;
    using MessageNumberAndBitField = uint32_t;

    // Use same size as MessageNumberAndBitField so we can use the enum with bitwise operations
    enum PacketPosition : MessageNumberAndBitField {
        ONLY   = 0x0,
        FIRST  = 0x2,
        MIDDLE = 0x3,
        LAST   = 0x1
    };
    
    static std::unique_ptr<Packet> create(qint64 size = -1, bool isReliable = false, bool isPartOfMessage = false);
    static std::unique_ptr<Packet> fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr);
    
    // Provided for convenience, try to limit use
    static std::unique_ptr<Packet> createCopy(const Packet& other);
    
    // Current level's header size
    static int localHeaderSize(bool isPartOfMessage = false);
    // Cumulated size of all the headers
    static int totalHeaderSize(bool isPartOfMessage = false);
    // The maximum payload size this packet can use to fit in MTU
    static int maxPayloadSize(bool isPartOfMessage = false);
    
    bool isPartOfMessage() const { return _isPartOfMessage; }
    bool isReliable() const { return _isReliable; }
    SequenceNumber getSequenceNumber() const { return _sequenceNumber; }

    MessageNumber getMessageNumber() const { return _messageNumber; }

    void setPacketPosition(PacketPosition position) { _packetPosition = position; }
    PacketPosition getPacketPosition() const { return _packetPosition; }
    
    void writeMessageNumber(MessageNumber messageNumber);
    void writeSequenceNumber(SequenceNumber sequenceNumber) const;

protected:
    Packet(qint64 size, bool isReliable = false, bool isPartOfMessage = false);
    Packet(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr);
    
    Packet(const Packet& other);
    Packet(Packet&& other);
    
    Packet& operator=(const Packet& other);
    Packet& operator=(Packet&& other);

private:
    // Header readers - these read data to member variables after pulling packet off wire
    void readHeader() const;
    void writeHeader() const;

    // Simple holders to prevent multiple reading and bitwise ops
    mutable bool _isReliable { false };
    mutable bool _isPartOfMessage { false };
    mutable SequenceNumber _sequenceNumber;
    mutable PacketPosition _packetPosition { PacketPosition::ONLY };
    mutable MessageNumber _messageNumber { 0 };
};
    
} // namespace udt

#endif // hifi_Packet_h
