//
//  PacketSender.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet sender.
//

#include <math.h>
#include <stdint.h>

#include "NodeList.h"
#include "PacketSender.h"
#include "SharedUtil.h"

const int PacketSender::DEFAULT_PACKETS_PER_SECOND = 200;
const int PacketSender::MINIMUM_PACKETS_PER_SECOND = 1;

const int AVERAGE_CALL_TIME_SAMPLES = 10;

PacketSender::PacketSender(PacketSenderNotify* notify, int packetsPerSecond) : 
    _packetsPerSecond(packetsPerSecond),
    _usecsPerProcessCallHint(0),
    _lastProcessCallTime(usecTimestampNow()),
    _averageProcessCallTime(AVERAGE_CALL_TIME_SAMPLES),
    _lastSendTime(usecTimestampNow()),
    _notify(notify)
{
}


void PacketSender::queuePacketForSending(sockaddr& address, unsigned char* packetData, ssize_t packetLength) {
    NetworkPacket packet(address, packetData, packetLength);
    lock();
    _packets.push_back(packet);
    unlock();
}

bool PacketSender::process() {
    uint64_t USECS_PER_SECOND = 1000 * 1000;
    uint64_t SEND_INTERVAL_USECS = (_packetsPerSecond == 0) ? USECS_PER_SECOND : (USECS_PER_SECOND / _packetsPerSecond);
    
    // keep track of our process call times, so we have a reliable account of how often our caller calls us
    uint64_t now = usecTimestampNow();
    uint64_t elapsedSinceLastCall = now - _lastProcessCallTime;
    _lastProcessCallTime = now;
    _averageProcessCallTime.updateAverage(elapsedSinceLastCall);
    
    if (_packets.size() == 0) {
        if (isThreaded()) {
            usleep(SEND_INTERVAL_USECS);
        } else {
            return isStillRunning();  // in non-threaded mode, if there's nothing to do, just return, keep running till they terminate us
        }
    }

    int packetsPerCall = _packets.size(); // in threaded mode, we just empty this!
    int packetsThisCall = 0;
    
    // if we're in non-threaded mode, then we actually need to determine how many packets to send per call to process
    // based on how often we get called... We do this by keeping a running average of our call times, and we determine
    // how many packets to send per call
    if (!isThreaded()) {
        int averageCallTime;
        const int TRUST_AVERAGE_AFTER = AVERAGE_CALL_TIME_SAMPLES * 2;
        if (_usecsPerProcessCallHint == 0 || _averageProcessCallTime.getSampleCount() > TRUST_AVERAGE_AFTER) {
            averageCallTime = _averageProcessCallTime.getAverage();
        } else {
            averageCallTime = _usecsPerProcessCallHint;
        }
        
        // we can determine how many packets we need to send per call to achieve our desired
        // packets per second send rate.
        int callsPerSecond = USECS_PER_SECOND / averageCallTime;
        
        // make sure our number of calls per second doesn't cause a divide by zero
        callsPerSecond = glm::clamp(callsPerSecond, 1, _packetsPerSecond);
        
        packetsPerCall = ceil(_packetsPerSecond / callsPerSecond);
        
        // send at least one packet per call, if we have it
        if (packetsPerCall < 1) {
            packetsPerCall = 1;
        }
    }
    
    int packetsLeft = _packets.size();
    bool keepGoing = packetsLeft > 0;
    while (keepGoing) {
        uint64_t SEND_INTERVAL_USECS = (_packetsPerSecond == 0) ? USECS_PER_SECOND : (USECS_PER_SECOND / _packetsPerSecond);

        lock();
        NetworkPacket& packet = _packets.front();
        NetworkPacket temporary = packet; // make a copy
        _packets.erase(_packets.begin());
        packetsLeft = _packets.size();
        unlock();

        // send the packet through the NodeList...
        UDPSocket* nodeSocket = NodeList::getInstance()->getNodeSocket();

        nodeSocket->send(&temporary.getAddress(), temporary.getData(), temporary.getLength());
        packetsThisCall++;

        if (_notify) {
            _notify->packetSentNotification(temporary.getLength());
        }


        // in threaded mode, we go till we're empty
        if (isThreaded()) {
            keepGoing = packetsLeft > 0;

            // dynamically sleep until we need to fire off the next set of voxels we only sleep in threaded mode
            if (keepGoing) {
                now = usecTimestampNow();
                uint64_t elapsed = now - _lastSendTime;
                int usecToSleep =  SEND_INTERVAL_USECS - elapsed;
                
                // we only sleep in non-threaded mode
                if (usecToSleep > 0) {
                    if (usecToSleep > SEND_INTERVAL_USECS) {
                        usecToSleep = SEND_INTERVAL_USECS;
                    }
                    usleep(usecToSleep);
                }
            }

        } else {
            // in non-threaded mode, we send as many packets as we need per expected call to process()
            keepGoing = (packetsThisCall < packetsPerCall) && (packetsLeft > 0);
        }
        
        _lastSendTime = now;
    }

    return isStillRunning();  // keep running till they terminate us
}
