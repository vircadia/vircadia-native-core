//
//  PacketSender.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet sender.
//

#include <QDebug.h>

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
    while (_packets.size() > 0) {
        NetworkPacket& packet = _packets.front();
        
        // send the packet through the NodeList...
        UDPSocket* nodeSocket = NodeList::getInstance()->getNodeSocket();

        qDebug("PacketSender::process()... nodeSocket->send() length=%lu\n", packet.getLength());

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
            //qDebug("PacketSender::process()... sleeping for %d useconds\n", usecToSleep);
            usleep(usecToSleep);
        }

    }
    return true;  // keep running till they terminate us
}
