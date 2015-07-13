//
//  NLPacketList.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/06/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NLPacketList_h
#define hifi_NLPacketList_h

#include <QtCore/QIODevice>

#include "PacketHeaders.h"

#include "NLPacket.h"

class NLPacketList : public QIODevice {
    Q_OBJECT
public:
    NLPacketList(PacketType::Value packetType);

    virtual bool isSequential() const { return true; }

    void startSegment() { _segmentStartIndex = _currentPacket->pos(); }
    void endSegment() { _segmentStartIndex = -1; }

    int getNumPackets() const { return _packets.size() + (_currentPacket ? 1 : 0); }

    void closeCurrentPacket();

    void setExtendedHeader(const QByteArray& extendedHeader) { _extendedHeader = extendedHeader; }

    template<typename T> qint64 readPrimitive(T* data);
    template<typename T> qint64 writePrimitive(const T& data);
protected:
    virtual qint64 writeData(const char* data, qint64 maxSize);
    virtual qint64 readData(char* data, qint64 maxSize) { return 0; }

private:
    friend class LimitedNodeList;
    
    NLPacketList(const NLPacketList& other) = delete;
    NLPacketList& operator=(const NLPacketList& other) = delete;

    std::unique_ptr<NLPacket> createPacketWithExtendedHeader();

    PacketType::Value _packetType;
    bool _isOrdered = false;

    std::unique_ptr<NLPacket> _currentPacket;
    std::list<std::unique_ptr<NLPacket>> _packets;

    int _segmentStartIndex = -1;

    QByteArray _extendedHeader;
};

template <typename T> qint64 NLPacketList::readPrimitive(T* data) {
    return QIODevice::read(reinterpret_cast<char*>(data), sizeof(T));
}

template <typename T> qint64 NLPacketList::writePrimitive(const T& data) {
    static_assert(!std::is_pointer<T>::value, "T must not be a pointer");
    return QIODevice::write(reinterpret_cast<const char*>(&data), sizeof(T));
}


#endif // hifi_PacketList_h
