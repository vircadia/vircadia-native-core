//
//  PacketList.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/06/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketList_h
#define hifi_PacketList_h

#include <QIODevice>

#include "PacketHeaders.h"

class NLPacket;

template <typename T> class PacketList : public QIODevice {
public:
    PacketList(PacketType::Value packetType);

    virtual bool isSequential() const { return true; }

    void startSegment() { _segmentStartIndex = _currentPacket->payload().pos(); }
    void endSegment() { _segmentStartIndex = -1; }

    int getNumPackets() const { return _packets.size() + (_currentPacket ? 1 : 0); }

    void getCurrentPacketCapacity() const { return _currentPacket->bytesAvailable(); }

    void closeCurrentPacket();

    void setExtendedHeader(const QByteArray& extendedHeader) { _extendedHeader = extendedHeader; }

    template<typename U> qint64 readPrimitive(U* data);
    template<typename U> qint64 writePrimitive(const U& data);
protected:
    qint64 writeData(const char* data, qint64 maxSize);
    qint64 readData(char* data, qint64 maxSize) { return 0; }
private:
    PacketList(const PacketList& other) = delete;
    PacketList& operator=(const PacketList& other) = delete;

    std::unique_ptr<NLPacket> createPacketWithExtendedHeader();

    PacketType::Value _packetType;
    bool _isOrdered = false;

    std::unique_ptr<T> _currentPacket;
    std::list<std::unique_ptr<T>> _packets;

    int _segmentStartIndex = -1;

    QByteArray _extendedHeader;
};

template <typename T> template <typename U> PacketList<T>::readPrimitive(U* data) {
    return QIODevice::read(reinterpret_cast<char*>(data), sizeof(U));
}

template <typename T> template <typename U> PacketList<T>::writePrimitive(const U& data) {
    static_assert(!std::is_pointer<U>::value, "U must not be a pointer");
    return QIODevice::write(reinterpret_cast<const char*>(&data), sizeof(U));
}


#endif // hifi_PacketList_h
