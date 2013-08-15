//
//  PacketSender.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet sender.
//

#include <stdint.h>

const uint64_t SEND_INTERVAL_USECS = 1000 * 5; // no more than 200pps... should be settable

#include "NodeList.h"
#include "PacketSender.h"
#include "SharedUtil.h"

PacketSender::PacketSender() {
    _lastSendTime = usecTimestampNow();
}


void PacketSender::queuePacket(sockaddr& address, unsigned char* packetData, ssize_t packetLength) {
    NetworkPacket packet(address, packetData, packetLength);
    lock();
    _packets.push_back(packet);
    unlock();
}

bool PacketSender::process() {
    if (_packets.size() == 0) {
        const uint64_t SEND_THREAD_SLEEP_INTERVAL = (1000 * 1000)/60; // check at 60fps
        usleep(SEND_THREAD_SLEEP_INTERVAL);
    }
    while (_packets.size() > 0) {
        NetworkPacket& packet = _packets.front();
        
        // send the packet through the NodeList...
        UDPSocket* nodeSocket = NodeList::getInstance()->getNodeSocket();

        nodeSocket->send(&packet.getAddress(), packet.getData(), packet.getLength());

        lock();
        _packets.erase(_packets.begin());
        unlock();

        uint64_t now = usecTimestampNow();
        // dynamically sleep until we need to fire off the next set of voxels
        uint64_t elapsed = now - _lastSendTime;
        int usecToSleep =  SEND_INTERVAL_USECS - elapsed;
        _lastSendTime = now;
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }

    }
    return true;  // keep running till they terminate us
}
