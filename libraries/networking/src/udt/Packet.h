//
//  Packet.h
//  libraries/networking/src/udt
//
//  Created by Clement on 7/2/15.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
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
    //                         Packet Header Format
    //
    //     0                   1                   2                   3
    //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |C|R|M| O |               Sequence Number                       |
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    | P |                     Message Number                        |  Optional (only if M = 1)
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |                         Message Part Number                   |  Optional (only if M = 1)
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    //    C: Control bit
    //    R: Reliable bit
    //    M: Message bit
    //    O: Obfuscation level
    //    P: Position bits


    // NOTE: The SequenceNumber is only actually 29 bits to leave room for a bit field
    using SequenceNumberAndBitField = uint32_t;
    
    // NOTE: The MessageNumber is only actually 30 bits to leave room for a bit field
    using MessageNumber = uint32_t;
    using MessageNumberAndBitField = uint32_t;
    using MessagePartNumber = uint32_t;

    // Use same size as MessageNumberAndBitField so we can use the enum with bitwise operations
    enum PacketPosition : MessageNumberAndBitField {
        ONLY   = 0x0, // 00
        FIRST  = 0x2, // 10
        MIDDLE = 0x3, // 11
        LAST   = 0x1  // 01
    };

    // Use same size as SequenceNumberAndBitField so we can use the enum with bitwise operations
    enum ObfuscationLevel : SequenceNumberAndBitField {
        NoObfuscation = 0x0, // 00
        ObfuscationL1 = 0x1, // 01
        ObfuscationL2 = 0x2, // 10
        ObfuscationL3 = 0x3, // 11
    };

    static std::unique_ptr<Packet> create(qint64 size = -1, bool isReliable = false, bool isPartOfMessage = false);
    static std::unique_ptr<Packet> fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size, const SockAddr& senderSockAddr);
    
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
    void setReliable(bool reliable) { _isReliable = reliable; }

    ObfuscationLevel getObfuscationLevel() const { return _obfuscationLevel; }
    SequenceNumber getSequenceNumber() const { return _sequenceNumber; }
    MessageNumber getMessageNumber() const { return _messageNumber; }
    PacketPosition getPacketPosition() const { return _packetPosition; }
    MessagePartNumber getMessagePartNumber() const { return _messagePartNumber; }
    
    void writeMessageNumber(MessageNumber messageNumber, PacketPosition position, MessagePartNumber messagePartNumber);
    void writeSequenceNumber(SequenceNumber sequenceNumber) const;
    void obfuscate(ObfuscationLevel level);

protected:
    Packet(qint64 size, bool isReliable = false, bool isPartOfMessage = false);
    Packet(std::unique_ptr<char[]> data, qint64 size, const SockAddr& senderSockAddr);
    
    Packet(const Packet& other);
    Packet(Packet&& other);
    
    Packet& operator=(const Packet& other);
    Packet& operator=(Packet&& other);

private:
    void copyMembers(const Packet& other);

    // Header readers - these read data to member variables after pulling packet off wire
    void readHeader() const;
    void writeHeader() const;

    // Simple holders to prevent multiple reading and bitwise ops
    mutable bool _isReliable { false };
    mutable bool _isPartOfMessage { false };
    mutable ObfuscationLevel _obfuscationLevel { NoObfuscation };
    mutable SequenceNumber _sequenceNumber { 0 };
    mutable MessageNumber _messageNumber { 0 };
    mutable PacketPosition _packetPosition { PacketPosition::ONLY };
    mutable MessagePartNumber _messagePartNumber { 0 };
};

} // namespace udt

Q_DECLARE_METATYPE(udt::Packet*);

#endif // hifi_Packet_h
