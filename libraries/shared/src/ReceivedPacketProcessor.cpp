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

ReceivedPacketProcessor::ReceivedPacketProcessor() {
    _dontSleep = false;
}

void ReceivedPacketProcessor::terminating() {
    _hasPackets.wakeAll();
}

void ReceivedPacketProcessor::queueReceivedPacket(const SharedNodePointer& destinationNode, const QByteArray& packet) {
    // Make sure our Node and NodeList knows we've heard from this node.
    destinationNode->setLastHeardMicrostamp(usecTimestampNow());

    NetworkPacket networkPacket(destinationNode, packet);
    lock();
    _packets.push_back(networkPacket);
    unlock();
    
    // Make sure to  wake our actual processing thread because we  now have packets for it to process.
    _hasPackets.wakeAll();
}

bool ReceivedPacketProcessor::process() {

    // If a derived class handles process sleeping, like the JurisdiciontListener, then it can set
    // this _dontSleep member and we will honor that request.
    if (_packets.size() == 0 && !_dontSleep) {
        _waitingOnPacketsMutex.lock();
        _hasPackets.wait(&_waitingOnPacketsMutex);
        _waitingOnPacketsMutex.unlock();
    }
    while (_packets.size() > 0) {
        lock(); // lock to make sure nothing changes on us
        NetworkPacket& packet = _packets.front(); // get the oldest packet
        NetworkPacket temporary = packet; // make a copy of the packet in case the vector is resized on us
        _packets.erase(_packets.begin()); // remove the oldest packet
        unlock(); // let others add to the packets
        processPacket(temporary.getDestinationNode(), temporary.getByteArray()); // process our temporary copy
    }
    return isStillRunning();  // keep running till they terminate us
}
