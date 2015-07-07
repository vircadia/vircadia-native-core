//
//  Packet.h
//  libraries/networking/src
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

#include "PacketHeaders.h"
#include "PacketPayload.h"

class Packet {
public:
    using SequenceNumber = uint16_t;
    
    static int64_t headerSize(PacketType::Value type);
    static int64_t maxPayloadSize(PacketType::Value type);
    
    static std::unique_ptr<Packet> create(PacketType::Value type, int64_t size = -1);
    
    PacketType::Value getPacketType() const;
    PacketVersion getPacketTypeVersion() const;
    SequenceNumber getSequenceNumber() const;
    bool isControlPacket() const;
    
    PacketPayload& payload() { return _payload; }
    
protected:
    Packet(PacketType::Value type, int64_t size);
    Packet(Packet&&) = delete;
    Packet(const Packet&) = delete;
    Packet& operator=(Packet&&) = delete;
    Packet& operator=(const Packet&) = delete;
    
    void setPacketTypeAndVersion(PacketType::Value type);
    void setSequenceNumber(SequenceNumber seqNum);
    
    int64_t _packetSize;
    std::unique_ptr<char> _packet;
    
    PacketPayload _payload;
};

#endif // hifi_Packet_h