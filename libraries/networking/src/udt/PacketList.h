//
//  PacketList.h
//
//
//  Created by Clement on 7/13/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketList_h
#define hifi_PacketList_h

#include <memory>

#include <QtCore/QIODevice>

#include "Packet.h"
#include "PacketHeaders.h"

class LimitedNodeList;

namespace udt {

class Packet;

class PacketList : public QIODevice {
    Q_OBJECT
public:
    static std::unique_ptr<PacketList> create(PacketType packetType, QByteArray extendedHeader = QByteArray(),
                                              bool isReliable = false, bool isOrdered = false);
    static std::unique_ptr<PacketList> fromReceivedPackets(std::list<std::unique_ptr<Packet>>&& packets);
    
    bool isReliable() const { return _isReliable; }
    bool isOrdered() const { return _isOrdered; }
    
    void startSegment();
    void endSegment();
    
    PacketType getType() const { return _packetType; }
    int getNumPackets() const { return _packets.size() + (_currentPacket ? 1 : 0); }

    QByteArray getExtendedHeader() const { return _extendedHeader; }

    size_t getDataSize() const;
    size_t getMessageSize() const;
    
    void closeCurrentPacket(bool shouldSendEmpty = false);

    QByteArray getMessage();

    // QIODevice virtual functions
    virtual bool isSequential() const  { return false; }
    virtual qint64 size() const { return getDataSize(); }
    
    template<typename T> qint64 readPrimitive(T* data);
    template<typename T> qint64 writePrimitive(const T& data);
    
protected:
    PacketList(PacketType packetType, QByteArray extendedHeader = QByteArray(), bool isReliable = false, bool isOrdered = false);
    PacketList(PacketList&& other);

    virtual qint64 writeData(const char* data, qint64 maxSize);
    // Not implemented, added an assert so that it doesn't get used by accident
    virtual qint64 readData(char* data, qint64 maxSize) { Q_ASSERT(false); return 0; }
    
    PacketType _packetType;
    std::list<std::unique_ptr<Packet>> _packets;
    
private:
    friend class ::LimitedNodeList;
    friend class SendQueue;
    friend class Socket;
    
    PacketList(const PacketList& other) = delete;
    PacketList& operator=(const PacketList& other) = delete;
    
    // Takes the first packet of the list and returns it.
    template<typename T> std::unique_ptr<T> takeFront();
    
    // Creates a new packet, can be overriden to change return underlying type
    virtual std::unique_ptr<Packet> createPacket();
    std::unique_ptr<Packet> createPacketWithExtendedHeader();
    
    Packet::MessageNumber _messageNumber;
    bool _isReliable = false;
    bool _isOrdered = false;
    
    std::unique_ptr<Packet> _currentPacket;
    
    int _segmentStartIndex = -1;
    
    QByteArray _extendedHeader;
};

template <typename T> qint64 PacketList::readPrimitive(T* data) {
    static_assert(!std::is_pointer<T>::value, "T must not be a pointer");
    return read(reinterpret_cast<char*>(data), sizeof(T));
}

template <typename T> qint64 PacketList::writePrimitive(const T& data) {
    static_assert(!std::is_pointer<T>::value, "T must not be a pointer");
    return write(reinterpret_cast<const char*>(&data), sizeof(T));
}

template<typename T> std::unique_ptr<T> PacketList::takeFront() {
    static_assert(std::is_base_of<Packet, T>::value, "T must derive from Packet.");
    
    auto packet = std::move(_packets.front());
    _packets.pop_front();
    return std::unique_ptr<T>(dynamic_cast<T*>(packet.release()));
}
    
}

#endif // hifi_PacketList_h
