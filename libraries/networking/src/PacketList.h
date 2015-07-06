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

#pragma once

template <class T> class PacketList : public QIODevice {
public:
    PacketList(PacketType::Value packetType, bool isOrdered = false);

    virtual bool isSequential() const { return true; }

    void startSegment() { _segmentStartIndex = currentPacket->payload().pos(); }
    void endSegment() { _segmentStartIndex = -1; }

    void closeCurrentPacket();

    void setExtendedHeader(const QByteArray& extendedHeader) { _extendedHeader = extendedHeader; }
protected:
    qint64 writeData(const char* data, qint64 maxSize);
    qint64 readData(const char* data, qint64 maxSize) { return 0 };
private:
    void createPacketWithExtendedHeader();

    PacketType::Value _packetType;
    bool isOrdered;

    std::unique_ptr<T> _currentPacket;
    std::list<std::unique_ptr<T>> _packets;

    int _segmentStartIndex = -1;

    QByteArray _extendedHeader = extendedHeader;
}

#endif // hifi_PacketList_h
