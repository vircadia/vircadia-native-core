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

#include "udt/PacketList.h"

#include "NLPacket.h"

class NLPacketList : public udt::PacketList {
public:
    NLPacketList(PacketType packetType, QByteArray extendedHeader = QByteArray(), bool isReliable = false, bool isOrdered = false);
    NLPacketList(PacketList&& packetList);

    const QUuid& getSourceID() const { return _sourceID; }
    
private:
    NLPacketList(const NLPacketList& other) = delete;
    NLPacketList& operator=(const NLPacketList& other) = delete;

    virtual std::unique_ptr<udt::Packet> createPacket();

    QUuid _sourceID;
};

#endif // hifi_PacketList_h
