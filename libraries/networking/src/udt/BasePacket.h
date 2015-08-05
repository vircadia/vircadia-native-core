//
//  BasePacket.h
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-23.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_BasePacket_h
#define hifi_BasePacket_h

#include <QtCore/QIODevice>

#include "../HifiSockAddr.h"

namespace udt {
    
static const int MAX_PACKET_SIZE = 1450;
    
class BasePacket : public QIODevice {
    Q_OBJECT
public:
    static const qint64 PACKET_WRITE_ERROR;
    
    static std::unique_ptr<BasePacket> create(qint64 size = -1);
    static std::unique_ptr<BasePacket> fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr);
    
    static qint64 maxPayloadSize() { return MAX_PACKET_SIZE; }  // The maximum payload size this packet can use to fit in MTU
    static qint64 localHeaderSize() { return 0; } // Current level's header size
    
    virtual qint64 totalHeadersSize() const { return 0; } // Cumulated size of all the headers
    
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
    BasePacket(qint64 size);
    BasePacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr);
    BasePacket(const BasePacket& other);
    BasePacket& operator=(const BasePacket& other);
    BasePacket(BasePacket&& other);
    BasePacket& operator=(BasePacket&& other);
    
    // QIODevice virtual functions
    virtual qint64 writeData(const char* data, qint64 maxSize);
    virtual qint64 readData(char* data, qint64 maxSize);
    
    virtual void adjustPayloadStartAndCapacity(bool shouldDecreasePayloadSize = false);
    
    qint64 _packetSize = 0;        // Total size of the allocated memory
    std::unique_ptr<char[]> _packet; // Allocated memory
    
    char* _payloadStart = nullptr; // Start of the payload
    qint64 _payloadCapacity = 0;          // Total capacity of the payload
    
    qint64 _payloadSize = 0;          // How much of the payload is actually used
    
    HifiSockAddr _senderSockAddr;  // sender address for packet (only used on receiving end)
};

template<typename T> qint64 BasePacket::peekPrimitive(T* data) {
    return QIODevice::peek(reinterpret_cast<char*>(data), sizeof(T));
}

template<typename T> qint64 BasePacket::readPrimitive(T* data) {
    return QIODevice::read(reinterpret_cast<char*>(data), sizeof(T));
}

template<typename T> qint64 BasePacket::writePrimitive(const T& data) {
    static_assert(!std::is_pointer<T>::value, "T must not be a pointer");
    return QIODevice::write(reinterpret_cast<const char*>(&data), sizeof(T));
}
    
} // namespace udt


#endif // hifi_BasePacket_h
