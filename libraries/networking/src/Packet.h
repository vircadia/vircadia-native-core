//
//  Packet.h
//
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

#include "LimitedNodeList.h"
#include "PacketHeaders.h"

class Packet {
public:
    struct Header {
        // Required
        uint8_t _type;
        uint8_t _version;
        
        // Optionnal
        uint16_t _messageNumber;
        //   1st bit is the data/control bit, rest is the sequence number
        uint32_t _controlBitAndSequenceNumber;
    };
    // TODO sanity check Header struct size
    
    static int64_t headerSize();
    static int64_t maxPayloadSize();
    
    static std::unique_ptr<Packet> create(PacketType::Value type, int64_t size = -1);
    
    virtual const Header& getHeader() const;
    virtual char* getPayload();
    
protected:
    Packet(PacketType::Value type, int64_t size);
    Packet(Packet&&) = delete;
    Packet(const Packet&) = delete;
    Packet& operator=(Packet&&) = delete;
    Packet& operator=(const Packet&) = delete;
    
    Header& getHeader();
    
private:
    int64_t _packetSize;
    std::unique_ptr<char> _packet;
    
    char* _payloadStart;
};

class NodeListPacket : public Packet {
public:
    struct Header {
        // Required
        char _sourceUuid[NUM_BYTES_RFC4122_UUID];
        char _connectionUuid[NUM_BYTES_RFC4122_UUID];
    };
    static int64_t headerSize();
    static int64_t maxPayloadSize();
    
    static std::unique_ptr<NodeListPacket> create(PacketType::Value type, int64_t size = -1);
    
protected:
    NodeListPacket(PacketType::Value type, int64_t size);
};

#endif // hifi_Packet_h