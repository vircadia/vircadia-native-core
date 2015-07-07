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

#include <QIODevice>

#include "PacketHeaders.h"

class Packet : public QIODevice {
public:
    using SequenceNumber = uint16_t;
    
    static std::unique_ptr<Packet> create(PacketType::Value type, int64_t size = -1);
    
    static qint64 localHeaderSize(PacketType::Value type);
    static qint64 maxPayloadSize(PacketType::Value type);
    
    virtual qint64 totalHeadersSize() const; // Cumulated size of all the headers
    virtual qint64 localHeaderSize() const;  // Current level's header size
    
    // Payload direct access, use responsibly!
    char* getPayload() { return _payloadStart; }
    const char* getPayload() const { return _payloadStart; }
    
    qint64 getSizeWithHeader() const { return localHeaderSize() + getSizeUsed(); }
    qint64 getSizeUsed() const { return _sizeUsed; }
    void setSizeUsed(qint64 sizeUsed) { _sizeUsed = sizeUsed; }

    // Header readers
    PacketType::Value getPacketType() const;
    PacketVersion getPacketTypeVersion() const;
    SequenceNumber getSequenceNumber() const;
    bool isControlPacket() const;
    
    // QIODevice virtual functions
    // WARNING: Those methods all refer to the payload ONLY and NOT the entire packet
    virtual bool atEnd() const { return _position == _capacity; }
    virtual qint64 bytesAvailable() const { return size() - pos(); }
    virtual bool canReadLine() const { return false; }
    virtual bool isSequential() const  { return false; }
    virtual qint64 pos() const { return _position; }
    virtual bool reset() { return seek(0); }
    virtual bool seek(qint64 pos);
    virtual qint64 size() const { return _capacity; }
    
protected:
    Packet(PacketType::Value type, int64_t size);
    Packet(const Packet& other);
    Packet& operator=(const Packet& other);
    Packet(Packet&& other);
    Packet& operator=(Packet&& other);

    // QIODevice virtual functions
    virtual qint64 writeData(const char* data, qint64 maxSize);
    virtual qint64 readData(char* data, qint64 maxSize);
    
    // Header writers
    void setPacketTypeAndVersion(PacketType::Value type);
    void setSequenceNumber(SequenceNumber seqNum);
    
    PacketType::Value _type;
    
    qint64 _packetSize = 0;        // Total size of the allocated memory
    std::unique_ptr<char> _packet; // Allocated memory
    
    char* _payloadStart = nullptr; // Start of the payload
    qint64 _position = 0;          // Current position in the payload
    qint64 _capacity = 0;          // Total capacity of the payload
    
    qint64 _sizeUsed = 0;          // How much of the payload is actually used
};

#endif // hifi_Packet_h