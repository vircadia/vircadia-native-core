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

#include "NodeList.h"
#include "PacketSender.h"
#include "SharedUtil.h"

const int PacketSender::DEFAULT_PACKETS_PER_SECOND = 200;
const int PacketSender::MINIMUM_PACKETS_PER_SECOND = 1;


PacketSender::PacketSender(PacketSenderNotify* notify, int packetsPerSecond) : 
    _packetsPerSecond(packetsPerSecond),
    _lastSendTime(usecTimestampNow()),
    _notify(notify)
{
}


void PacketSender::queuePacket(sockaddr& address, unsigned char* packetData, ssize_t packetLength) {
    NetworkPacket packet(address, packetData, packetLength);
    lock();
    _packets.push_back(packet);
    unlock();
}

bool PacketSender::process() {
//printf("PacketSender::process() packets pending=%ld _packetsPerSecond=%d\n",_packets.size(),_packetsPerSecond);

    
    uint64_t USECS_PER_SECOND = 1000 * 1000;
    uint64_t SEND_INTERVAL_USECS = (_packetsPerSecond == 0) ? USECS_PER_SECOND : (USECS_PER_SECOND / _packetsPerSecond);
    
    if (_packets.size() == 0) {
        usleep(SEND_INTERVAL_USECS);
    }
    while (_packets.size() > 0) {
        NetworkPacket& packet = _packets.front();
        
        // send the packet through the NodeList...
        UDPSocket* nodeSocket = NodeList::getInstance()->getNodeSocket();

//printf("sending a packet... length=%lu\n",packet.getLength());
        nodeSocket->send(&packet.getAddress(), packet.getData(), packet.getLength());
        
        if (_notify) {
            _notify->packetSentNotification(packet.getLength());
        }

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
