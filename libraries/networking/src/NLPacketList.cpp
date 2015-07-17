//
//  NLPacketList.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/06/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NLPacketList.h"

#include "NLPacket.h"

NLPacketList::NLPacketList(PacketType::Value packetType, QByteArray extendedHeader) :
    PacketList(packetType, extendedHeader)
{

}

std::unique_ptr<Packet> NLPacketList::createPacket() {
    return NLPacket::create(getType());
}

