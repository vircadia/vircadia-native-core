//
//  Packet.h
//
//
//  Created by Clement on 7/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Packet_h
#define hifi_Packet_h

#include <memory>

class QByteArray;

class Packet {
    std::unique_ptr<Packet> makePacket();
    QByteArray payload();
    
protected:
    Packet();
    Packet(const Packet&) = delete;
    Packet(Packet&&) = delete;
    Packet& operator=(const Packet&) = delete;
    Packet& operator=(Packet&&) = delete;
};

#endif // hifi_Packet_h