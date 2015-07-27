//
//  Connection.h
//
//
//  Created by Clement on 7/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Connection_h
#define hifi_Connection_h

#include <memory>

#include "SendQueue.h"

class HifiSockAddr;

namespace udt {
    
class ControlPacket;
class Packet;
class Socket;

class Connection {

public:
    Connection(Socket* parentSocket, HifiSockAddr destination);
    
    void send(std::unique_ptr<Packet> packet);
    void processControl(std::unique_ptr<ControlPacket> controlPacket);
    
private:
    std::unique_ptr<SendQueue> _sendQueue;
};
    
}

#endif // hifi_Connection_h
