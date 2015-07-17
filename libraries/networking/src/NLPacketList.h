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

class NLPacketList : public PacketList {
public:
    NLPacketList(PacketType::Value packetType, QByteArray extendedHeader = QByteArray());
    
private:
    NLPacketList(const NLPacketList& other) = delete;
    NLPacketList& operator=(const NLPacketList& other) = delete;
    
    virtual std::unique_ptr<Packet> createPacket();
};

#endif // hifi_PacketList_h
