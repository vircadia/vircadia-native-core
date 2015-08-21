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

#include "PacketHeaders.h"

class Packet;

class PacketList : public QIODevice {
    Q_OBJECT
public:
    PacketList(PacketType::Value packetType, QByteArray extendedHeader = QByteArray());
    
    virtual bool isSequential() const { return true; }
    
    void startSegment();
    void endSegment();
    
    PacketType::Value getType() const { return _packetType; }
    int getNumPackets() const { return _packets.size() + (_currentPacket ? 1 : 0); }
    
    void closeCurrentPacket(bool shouldSendEmpty = false);
    
    template<typename T> qint64 readPrimitive(T* data);
    template<typename T> qint64 writePrimitive(const T& data);
protected:
    virtual qint64 writeData(const char* data, qint64 maxSize);
    virtual qint64 readData(char* data, qint64 maxSize) { return 0; }
    
private:
    friend class LimitedNodeList;
    
    PacketList(const PacketList& other) = delete;
    PacketList& operator=(const PacketList& other) = delete;
    
    // Takes the first packet of the list and returns it.
    template<typename T> std::unique_ptr<T> takeFront();
    
    // Creates a new packet, can be overriden to change return underlying type
    virtual std::unique_ptr<Packet> createPacket();
    std::unique_ptr<Packet> createPacketWithExtendedHeader();
    
    PacketType::Value _packetType;
    bool _isOrdered = false;
    
    std::unique_ptr<Packet> _currentPacket;
    std::list<std::unique_ptr<Packet>> _packets;
    
    int _segmentStartIndex = -1;
    
    QByteArray _extendedHeader;
};

template <typename T> qint64 PacketList::readPrimitive(T* data) {
    static_assert(!std::is_pointer<T>::value, "T must not be a pointer");
    return QIODevice::read(reinterpret_cast<char*>(data), sizeof(T));
}

template <typename T> qint64 PacketList::writePrimitive(const T& data) {
    static_assert(!std::is_pointer<T>::value, "T must not be a pointer");
    return QIODevice::write(reinterpret_cast<const char*>(&data), sizeof(T));
}

template<typename T> std::unique_ptr<T> PacketList::takeFront() {
    static_assert(std::is_base_of<Packet, T>::value, "T must derive from Packet.");
    
    auto packet = std::move(_packets.front());
    _packets.pop_front();
    return std::unique_ptr<T>(dynamic_cast<T*>(packet.release()));
}

#endif // hifi_PacketList_h
