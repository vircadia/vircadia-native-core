//
//  NLPacket.h
//  libraries/networking/src
//
//  Created by Clement on 7/6/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NLPacket_h
#define hifi_NLPacket_h

#include "Packet.h"

class NLPacket : public Packet {
public:
    static int64_t headerSize(PacketType::Value type);
    static int64_t maxPayloadSize(PacketType::Value type);
    
    static std::unique_ptr<NLPacket> create(PacketType::Value type, int64_t size = -1);
    
protected:
    NLPacket(PacketType::Value type, int64_t size);
    
    void setSourceUuid(QUuid sourceUuid);
    void setConnectionUuid(QUuid connectionUuid);
    
};

#endif // hifi_NLPacket_h