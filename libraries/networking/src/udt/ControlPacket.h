//
//  ControlPacket.h
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-24.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_ControlPacket_h
#define hifi_ControlPacket_h

#include <vector>

#include "BasePacket.h"
#include "Packet.h"

namespace udt {
    
class ControlPacket : public BasePacket {
    Q_OBJECT
public:
    using ControlBitAndType = uint32_t;
    
    enum Type : uint16_t {
        ACK,
        Handshake,
        HandshakeACK,
        HandshakeRequest
    };
    
    static std::unique_ptr<ControlPacket> create(Type type, qint64 size = -1);
    static std::unique_ptr<ControlPacket> fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size,
                                                             const SockAddr& senderSockAddr);
    // Current level's header size
    static int localHeaderSize();
    // Cumulated size of all the headers
    static int totalHeaderSize();
    // The maximum payload size this packet can use to fit in MTU
    static int maxPayloadSize();
    
    Type getType() const { return _type; }
    void setType(Type type);
    
private:
    Q_DISABLE_COPY(ControlPacket)
    ControlPacket(Type type, qint64 size = -1);
    ControlPacket(std::unique_ptr<char[]> data, qint64 size, const SockAddr& senderSockAddr);
    ControlPacket(ControlPacket&& other);
    
    ControlPacket& operator=(ControlPacket&& other);
    
    // Header read/write
    void readType();
    void writeType();
    
    Type _type;
};
    
} // namespace udt


#endif // hifi_ControlPacket_h
