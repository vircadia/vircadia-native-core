//
//  PacketReceiver.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet receiver.
//

#include "PacketReceiver.h"

void PacketReceiver::queuePacket(sockaddr& address, unsigned char* packetData, ssize_t packetLength) {
    _packets.push_back(NetworkPacket(address, packetData, packetLength));
}

bool PacketReceiver::process() {
    while (_packets.size() > 0) {
        NetworkPacket& packet = _packets.front();
        processPacket(packet.getAddress(), packet.getData(), packet.getLength());
        _packets.erase(_packets.begin());
    }
    return true;  // keep running till they terminate us
}
