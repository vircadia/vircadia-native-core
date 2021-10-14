//
//  PacketList.h
//
//
//  Created by Clement on 7/13/15.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketList_h
#define hifi_PacketList_h

#include <memory>

#include "../ExtendedIODevice.h"
#include "Packet.h"
#include "PacketHeaders.h"

class LimitedNodeList;

namespace udt {

class Packet;

class PacketList : public ExtendedIODevice {
    Q_OBJECT
public:
    using MessageNumber = uint32_t;
    using PacketPointer = std::unique_ptr<Packet>;
    
    static std::unique_ptr<PacketList> create(PacketType packetType, QByteArray extendedHeader = QByteArray(),
                                              bool isReliable = false, bool isOrdered = false);
    static std::unique_ptr<PacketList> fromReceivedPackets(std::list<std::unique_ptr<Packet>>&& packets);
    
    PacketType getType() const { return _packetType; }
    bool isReliable() const { return _isReliable; }
    bool isOrdered() const { return _isOrdered; }
    
    size_t getNumPackets() const { return _packets.size() + (_currentPacket ? 1 : 0); }
    size_t getDataSize() const;
    size_t getMessageSize() const;
    QByteArray getMessage() const;
    
    QByteArray getExtendedHeader() const { return _extendedHeader; }
    
    void startSegment();
    void endSegment();

    virtual qint64 getMaxSegmentSize() const { return Packet::maxPayloadSize(_isOrdered); }

    SockAddr getSenderSockAddr() const;
    
    void closeCurrentPacket(bool shouldSendEmpty = false);

    // QIODevice virtual functions
    virtual bool isSequential() const override { return false; }
    virtual qint64 size() const override { return getDataSize(); }
    
    qint64 writeString(const QString& string);

    p_high_resolution_clock::time_point getFirstPacketReceiveTime() const;
    
    
protected:
    PacketList(PacketType packetType, QByteArray extendedHeader = QByteArray(), bool isReliable = false, bool isOrdered = false);
    PacketList(PacketList&& other);
    
    void preparePackets(MessageNumber messageNumber);

    virtual qint64 writeData(const char* data, qint64 maxSize) override;
    // Not implemented, added an assert so that it doesn't get used by accident
    virtual qint64 readData(char* data, qint64 maxSize) override { Q_ASSERT(false); return 0; }
    
    PacketType _packetType;
    std::list<std::unique_ptr<Packet>> _packets;

    bool _isOrdered = false;
    
private:
    friend class ::LimitedNodeList;
    friend class PacketQueue;
    friend class SendQueue;
    friend class Socket;

    Q_DISABLE_COPY(PacketList)
    
    // Takes the first packet of the list and returns it.
    template<typename T> std::unique_ptr<T> takeFront();
    
    // Creates a new packet, can be overriden to change return underlying type
    virtual std::unique_ptr<Packet> createPacket();
    std::unique_ptr<Packet> createPacketWithExtendedHeader();
    
    Packet::MessageNumber _messageNumber;
    bool _isReliable = false;
    
    std::unique_ptr<Packet> _currentPacket;
    
    int _segmentStartIndex = -1;
    
    QByteArray _extendedHeader;
};

template<typename T> std::unique_ptr<T> PacketList::takeFront() {
    static_assert(std::is_base_of<Packet, T>::value, "T must derive from Packet.");
    
    auto packet = std::move(_packets.front());
    _packets.pop_front();
    return std::unique_ptr<T>(dynamic_cast<T*>(packet.release()));
}
    
}

Q_DECLARE_METATYPE(udt::PacketList*);

#endif // hifi_PacketList_h
