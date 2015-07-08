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
    // Provided for convenience, try to limit use
    static std::unique_ptr<Packet> createCopy(const std::unique_ptr<Packet>& other);

    static qint64 localHeaderSize(PacketType::Value type);
    static qint64 maxPayloadSize(PacketType::Value type);

    virtual qint64 totalHeadersSize() const; // Cumulated size of all the headers
    virtual qint64 localHeaderSize() const;  // Current level's header size

    // Payload direct access to the payload, use responsibly!
    char* getPayload() { return _payloadStart; }
    const char* getPayload() const { return _payloadStart; }

    // Return direct access to the entire packet, use responsibly!
    char* getData() { return _packet.get(); }
    const char* getData() const { return _packet.get(); }

    PacketType::Value getType() const { return _type; }
    void setType(PacketType::Value type);

    qint64 getSizeWithHeader() const { return localHeaderSize() + getSizeUsed(); }
    qint64 getSizeUsed() const { return _sizeUsed; }
    void setSizeUsed(qint64 sizeUsed) { _sizeUsed = sizeUsed; }

    // Header readers
    PacketType::Value readType() const;
    PacketVersion readVersion() const;
    SequenceNumber readSequenceNumber() const;
    bool readIsControlPacket() const;

    // QIODevice virtual functions
    // WARNING: Those methods all refer to the payload ONLY and NOT the entire packet
    virtual bool isSequential() const  { return false; }
    virtual bool reset() { setSizeUsed(0); return QIODevice::reset(); }
    virtual qint64 size() const { return _capacity; }

    template<typename T> qint64 read(T* data);
    template<typename T> qint64 write(const T& data);

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
    void writePacketTypeAndVersion(PacketType::Value type);
    void writeSequenceNumber(SequenceNumber seqNum);

    PacketType::Value _type;       // Packet type

    qint64 _packetSize = 0;        // Total size of the allocated memory
    std::unique_ptr<char> _packet; // Allocated memory

    char* _payloadStart = nullptr; // Start of the payload
    qint64 _capacity = 0;          // Total capacity of the payload

    qint64 _sizeUsed = 0;          // How much of the payload is actually used
};

#endif // hifi_Packet_h
