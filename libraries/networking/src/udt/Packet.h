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

#include "../HifiSockAddr.h"
#include "PacketHeaders.h"

namespace udt {

class Packet : public QIODevice {
    Q_OBJECT
public:
    // NOTE: The SequenceNumber is only actually 29 bits to leave room for a bit field
    using SequenceNumber = uint32_t;
    using SequenceNumberAndBitField = uint32_t;
    
    // NOTE: The MessageNumber is only actually 29 bits to leave room for a bit field
    using MessageNumber = uint32_t;
    using MessageNumberAndBitField = uint32_t;
    
    static const uint32_t DEFAULT_SEQUENCE_NUMBER = 0;

    static const qint64 PACKET_WRITE_ERROR;

    static std::unique_ptr<Packet> create(qint64 size = -1, bool isReliable = false, bool isPartOfMessage = false);
    static std::unique_ptr<Packet> fromReceivedPacket(std::unique_ptr<char> data, qint64 size, const HifiSockAddr& senderSockAddr);
    
    // Provided for convenience, try to limit use
    static std::unique_ptr<Packet> createCopy(const Packet& other);
    
    // The maximum payload size this packet can use to fit in MTU
    static qint64 maxPayloadSize(bool isPartOfMessage);
    virtual qint64 maxPayloadSize() const;
    
    // Current level's header size
    static qint64 localHeaderSize(bool isPartOfMessage);
    virtual qint64 localHeaderSize() const;
    
    virtual qint64 totalHeadersSize() const; // Cumulated size of all the headers

    // Payload direct access to the payload, use responsibly!
    char* getPayload() { return _payloadStart; }
    const char* getPayload() const { return _payloadStart; }

    // Return direct access to the entire packet, use responsibly!
    char* getData() { return _packet.get(); }
    const char* getData() const { return _packet.get(); }
    
    // Returns the size of the packet, including the header
    qint64 getDataSize() const { return totalHeadersSize() + _payloadSize; }
    
    // Returns the size of the payload only
    qint64 getPayloadSize() const { return _payloadSize; }
    
    // Allows a writer to change the size of the payload used when writing directly
    void setPayloadSize(qint64 payloadSize);
    
    // Returns the number of bytes allocated for the payload
    qint64 getPayloadCapacity() const  { return _payloadCapacity; }

    qint64 bytesLeftToRead() const { return _payloadSize - pos(); }
    qint64 bytesAvailableForWrite() const { return _payloadCapacity - pos(); }

    HifiSockAddr& getSenderSockAddr() { return _senderSockAddr; }
    const HifiSockAddr& getSenderSockAddr() const { return _senderSockAddr; }
    
    void writeSequenceNumber(SequenceNumber sequenceNumber);

    // QIODevice virtual functions
    // WARNING: Those methods all refer to the payload ONLY and NOT the entire packet
    virtual bool isSequential() const  { return false; }
    virtual bool reset();
    virtual qint64 size() const { return _payloadCapacity; }

    using QIODevice::read;
    QByteArray read(qint64 maxSize);
    
    template<typename T> qint64 peekPrimitive(T* data);
    template<typename T> qint64 readPrimitive(T* data);
    template<typename T> qint64 writePrimitive(const T& data);

protected:
    Packet(qint64 size, bool isReliable = false, bool isPartOfMessage = false);
    Packet(std::unique_ptr<char> data, qint64 size, const HifiSockAddr& senderSockAddr);
    Packet(const Packet& other);
    Packet& operator=(const Packet& other);
    Packet(Packet&& other);
    Packet& operator=(Packet&& other);

    // QIODevice virtual functions
    virtual qint64 writeData(const char* data, qint64 maxSize);
    virtual qint64 readData(char* data, qint64 maxSize);
    
    // Header readers - these read data to member variables after pulling packet off wire
    void readIsPartOfMessage();
    void readIsReliable();
    void readSequenceNumber();

    qint64 _packetSize = 0;        // Total size of the allocated memory
    std::unique_ptr<char> _packet; // Allocated memory

    char* _payloadStart = nullptr; // Start of the payload
    qint64 _payloadCapacity = 0;          // Total capacity of the payload

    qint64 _payloadSize = 0;          // How much of the payload is actually used
    
    bool _isReliable { false };
    bool _isPartOfMessage { false };
    SequenceNumber _sequenceNumber { 0 };

    HifiSockAddr _senderSockAddr;  // sender address for packet (only used on receiving end)
};

template<typename T> qint64 Packet::peekPrimitive(T* data) {
    return QIODevice::peek(reinterpret_cast<char*>(data), sizeof(T));
}

template<typename T> qint64 Packet::readPrimitive(T* data) {
    return QIODevice::read(reinterpret_cast<char*>(data), sizeof(T));
}

template<typename T> qint64 Packet::writePrimitive(const T& data) {
    static_assert(!std::is_pointer<T>::value, "T must not be a pointer");
    return QIODevice::write(reinterpret_cast<const char*>(&data), sizeof(T));
}
    
} // namespace udt

#endif // hifi_Packet_h
