//
//  ReceivedPacketProcessor.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet receiver.
//

#include "NodeList.h"
#include "ReceivedPacketProcessor.h"
#include "SharedUtil.h"

void ReceivedPacketProcessor::queueReceivedPacket(sockaddr& address, unsigned char* packetData, ssize_t packetLength) {
    // Make sure our Node and NodeList knows we've heard from this node.
    Node* node = NodeList::getInstance()->nodeWithAddress(&address);
    if (node) {
        node->setLastHeardMicrostamp(usecTimestampNow());
    }

    NetworkPacket packet(address, packetData, packetLength);
    lock();
    _packets.push_back(packet);
    unlock();
}

bool ReceivedPacketProcessor::process() {
    if (_packets.size() == 0) {
        const uint64_t RECEIVED_THREAD_SLEEP_INTERVAL = (1000 * 1000)/60; // check at 60fps
        usleep(RECEIVED_THREAD_SLEEP_INTERVAL);
    }
    while (_packets.size() > 0) {

        lock(); // lock to make sure nothing changes on us
        NetworkPacket& packet = _packets.front(); // get the oldest packet
        NetworkPacket temporary = packet; // make a copy of the packet in case the vector is resized on us
        _packets.erase(_packets.begin()); // remove the oldest packet
        unlock(); // let others add to the packets
        processPacket(temporary.getAddress(), temporary.getData(), temporary.getLength()); // process our temporary copy
    }
    return isStillRunning();  // keep running till they terminate us
}
