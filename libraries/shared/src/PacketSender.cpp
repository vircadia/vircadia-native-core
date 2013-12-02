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

const uint64_t PacketSender::USECS_PER_SECOND = 1000 * 1000;
const uint64_t PacketSender::SENDING_INTERVAL_ADJUST = 200; // approaximate 200us
const int PacketSender::TARGET_FPS = 60;
const int PacketSender::MAX_SLEEP_INTERVAL = PacketSender::USECS_PER_SECOND;

const int PacketSender::DEFAULT_PACKETS_PER_SECOND = 30;
const int PacketSender::MINIMUM_PACKETS_PER_SECOND = 1;
const int PacketSender::MINIMAL_SLEEP_INTERVAL = (USECS_PER_SECOND / TARGET_FPS) / 2;

const int AVERAGE_CALL_TIME_SAMPLES = 10;

PacketSender::PacketSender(PacketSenderNotify* notify, int packetsPerSecond) : 
    _packetsPerSecond(packetsPerSecond),
    _usecsPerProcessCallHint(0),
    _lastProcessCallTime(0),
    _averageProcessCallTime(AVERAGE_CALL_TIME_SAMPLES),
    _lastSendTime(0), // Note: we set this to 0 to indicate we haven't yet sent something
    _notify(notify),
    _lastPPSCheck(0),
    _packetsOverCheckInterval(0),
    _started(usecTimestampNow()),
    _totalPacketsSent(0),
    _totalBytesSent(0),
    _totalPacketsQueued(0),
    _totalBytesQueued(0)
{
}


void PacketSender::queuePacketForSending(const HifiSockAddr& address, unsigned char* packetData, ssize_t packetLength) {
    NetworkPacket packet(address, packetData, packetLength);
    lock();
    _packets.push_back(packet);
    unlock();
    _totalPacketsQueued++;
    _totalBytesQueued += packetLength;
}

bool PacketSender::process() {
    if (isThreaded()) {
        return threadedProcess();
    }
    return nonThreadedProcess();
}


bool PacketSender::threadedProcess() {
    bool hasSlept = false;
    
    if (_lastSendTime == 0) {
        _lastSendTime = usecTimestampNow();
    }
    
    // in threaded mode, we keep running and just empty our packet queue sleeping enough to keep our PPS on target
    while (_packets.size() > 0) {
        // Recalculate our SEND_INTERVAL_USECS each time, in case the caller has changed it on us..
        int packetsPerSecondTarget = (_packetsPerSecond > MINIMUM_PACKETS_PER_SECOND) 
                                            ? _packetsPerSecond : MINIMUM_PACKETS_PER_SECOND;

        uint64_t intervalBetweenSends = USECS_PER_SECOND / packetsPerSecondTarget;
        uint64_t sleepInterval = (intervalBetweenSends > SENDING_INTERVAL_ADJUST) ? 
                    intervalBetweenSends - SENDING_INTERVAL_ADJUST : intervalBetweenSends;

        // We'll sleep before we send, this way, we can set our last send time to be our ACTUAL last send time
        uint64_t now = usecTimestampNow();
        uint64_t elapsed = now - _lastSendTime;
        int usecToSleep =  sleepInterval - elapsed;

        // If we've never sent, or it's been a long time since we sent, then our elapsed time will be quite large
        // and therefore usecToSleep will be less than 0 and we won't sleep before sending...
        if (usecToSleep > 0) {
            if (usecToSleep > MAX_SLEEP_INTERVAL) {
                usecToSleep = MAX_SLEEP_INTERVAL;
            }
            usleep(usecToSleep);
            hasSlept = true;
        }
    
        // call our non-threaded version of ourselves
        bool keepRunning = nonThreadedProcess();
        
        if (!keepRunning) {
            break;
        }
    }

    // if threaded and we haven't slept? We want to sleep a little so we don't hog the CPU, but
    // we don't want to sleep too long because how ever much we sleep will delay any future unsent
    // packets that arrive while we're sleeping. So we sleep 1/2 of our target fps interval
    if (!hasSlept) {
        usleep(MINIMAL_SLEEP_INTERVAL);
    }

    return isStillRunning();
}


// We may be called more frequently than we get packets or need to send packets, we may also get called less frequently.
//
// If we're called more often then out target PPS then we will space out our actual sends to be a single packet for multiple
// calls to process. Those calls to proces in which we do not need to send a packet to keep up with our target PPS we will
// just track our call rate (in order to predict our sends per call) but we won't actually send any packets.
//
// When we are called less frequently than we have packets to send, we will send enough packets per call to keep up with our
// target PPS. 
//
// We also keep a running total of packets sent over multiple calls to process() so that we can adjust up or down for 
// possible rounding error that would occur if we only considered whole integer packet counts per call to process
bool PacketSender::nonThreadedProcess() {
    uint64_t now = usecTimestampNow();

    if (_lastProcessCallTime == 0) {
        _lastProcessCallTime = now - _usecsPerProcessCallHint;
    }

    const uint64_t MINIMUM_POSSIBLE_CALL_TIME = 10; // in usecs
    const uint64_t USECS_PER_SECOND = 1000 * 1000;
    const float ZERO_RESET_CALLS_PER_SECOND = 1; // used in guard against divide by zero
    
    bool wantDebugging = false;
    if (wantDebugging) {
        printf("\n\nPacketSender::nonThreadedProcess() _packets.size()=%ld\n",_packets.size());
    }
    
    // keep track of our process call times, so we have a reliable account of how often our caller calls us
    uint64_t elapsedSinceLastCall = now - _lastProcessCallTime;
    _lastProcessCallTime = now;
    _averageProcessCallTime.updateAverage(elapsedSinceLastCall);

    float averageCallTime = 0;
    const int TRUST_AVERAGE_AFTER = AVERAGE_CALL_TIME_SAMPLES * 2;
    if (_usecsPerProcessCallHint == 0 || _averageProcessCallTime.getSampleCount() > TRUST_AVERAGE_AFTER) {
        averageCallTime = _averageProcessCallTime.getAverage();

        if (wantDebugging) {
            printf("averageCallTime = _averageProcessCallTime.getAverage() =%f\n",averageCallTime);
        }

    } else {
        averageCallTime = _usecsPerProcessCallHint;

        if (wantDebugging) {
            printf("averageCallTime = _usecsPerProcessCallHint =%f\n",averageCallTime);
        }

    }

    if (wantDebugging) {
        printf("elapsedSinceLastCall=%llu averageCallTime=%f\n",elapsedSinceLastCall, averageCallTime);
    }
    
    if (_packets.size() == 0) {
        // in non-threaded mode, if there's nothing to do, just return, keep running till they terminate us
        return isStillRunning(); 
    }

    // This only happens once, the first time we get this far... so we can use it as an accurate initialization
    // point for these important timing variables    
    if (_lastPPSCheck == 0) {
        _lastPPSCheck = now;
        // pretend like our lifetime began once call cycle for now, this makes our lifetime PPS start out most accurately
        _started = now - (uint64_t)averageCallTime;
    }
    

    float averagePacketsPerCall = 0;  // might be less than 1, if our caller calls us more frequently than the target PPS
    int packetsSentThisCall = 0;
    int packetsToSendThisCall = 0;
    
    // Since we're in non-threaded mode, we need to determine how many packets to send per call to process
    // based on how often we get called... We do this by keeping a running average of our call times, and we determine
    // how many packets to send per call

    // We assume you can't possibly call us less than MINIMUM_POSSIBLE_CALL_TIME apart
    if (averageCallTime <= 0) {
        averageCallTime = MINIMUM_POSSIBLE_CALL_TIME;
    }

    // we can determine how many packets we need to send per call to achieve our desired
    // packets per second send rate.
    float callsPerSecond = USECS_PER_SECOND / averageCallTime;

    if (wantDebugging) {
        printf("PacketSender::process() USECS_PER_SECOND=%llu averageCallTime=%f callsPerSecond=%f\n",
            USECS_PER_SECOND, averageCallTime, callsPerSecond );
    }    

    // theoretically we could get called less than 1 time per second... but since we're using floats, it really shouldn't be
    // possible to get 0 calls per second, but we will guard agains that here, just in case.
    if (callsPerSecond == 0) {
        callsPerSecond = ZERO_RESET_CALLS_PER_SECOND;
        qDebug("PacketSender::nonThreadedProcess() UNEXPECTED:callsPerSecond==0, assuming ZERO_RESET_CALLS_PER_SECOND\n");
    }
    
    // This is the average number of packets per call...
    averagePacketsPerCall = _packetsPerSecond / callsPerSecond;
    packetsToSendThisCall = averagePacketsPerCall;
    if (wantDebugging) {
        printf("PacketSender::process() averageCallTime=%f averagePacketsPerCall=%f packetsToSendThisCall=%d\n",
            averageCallTime, averagePacketsPerCall, packetsToSendThisCall);
    }    
    
    // if we get called more than 1 per second, we want to mostly divide the packets evenly across the calls...
    // but we want to track the remainder and make sure over the course of a second, we are sending the target PPS
    // e.g. 
    //     200pps called 60 times per second...
    //     200/60 = 3.333... so really...
    //     each call we should send 3
    //     every 3rd call we should send 4...
    //     3,3,4,3,3,4...3,3,4 = 200...
    
    // if we get called less than 1 per second, then we want to send more than our PPS each time...
    // e.g.
    //     200pps called ever 1332.5ms
    //     200 / (1000/1332.5) = 200/(0.7505) = 266.5 packets per call
    //     so... 
    //        every other call we should send 266 packets
    //        then on the next call we should send 267 packets
    
    // So no mater whether or not we're getting called more or less than once per second, we still need to do some bookkeeping
    // to make sure we send a few extra packets to even out our flow rate.
    uint64_t elapsedSinceLastCheck = now - _lastPPSCheck;
    
    // we might want to tun this in the future and only check after a certain number of call intervals. for now we check
    // each time and adjust accordingly
    const float CALL_INTERVALS_TO_CHECK = 1;
    const float MIN_CALL_INTERVALS_PER_RESET = 5;
    
    // we will reset our check PPS and time each second (callsPerSecond) or at least 5 calls (if we get called less frequently
    // than 5 times per second) This gives us sufficient smoothing in our packet adjustments
    float callIntervalsPerReset = fmax(callsPerSecond, MIN_CALL_INTERVALS_PER_RESET);

    if (wantDebugging) {
        printf("about to check interval... callsPerSecond=%f elapsedSinceLastPPSCheck=%llu checkInterval=%f\n",
            callsPerSecond, elapsedSinceLastCheck, (averageCallTime * CALL_INTERVALS_TO_CHECK));
    }

    if  (elapsedSinceLastCheck > (averageCallTime * CALL_INTERVALS_TO_CHECK)) {

        if (wantDebugging) {
            printf(">>>>>>>>>>>>>>>  check interval... _packetsOverCheckInterval=%d elapsedSinceLastPPSCheck=%llu\n",
                _packetsOverCheckInterval, elapsedSinceLastCheck);
        }
        
        float ppsOverCheckInterval = (float)_packetsOverCheckInterval;
        float ppsExpectedForCheckInterval = (float)_packetsPerSecond * ((float)elapsedSinceLastCheck / (float)USECS_PER_SECOND);

        if (wantDebugging) {
            printf(">>>>>>>>>>>>>>>  check interval... ppsOverCheckInterval=%f ppsExpectedForCheckInterval=%f\n",
                ppsOverCheckInterval, ppsExpectedForCheckInterval);
        }
                
        if (ppsOverCheckInterval < ppsExpectedForCheckInterval) {
            int adjust = ppsExpectedForCheckInterval - ppsOverCheckInterval;
            packetsToSendThisCall += adjust;
            if (wantDebugging) {
                qDebug("Lower pps [%f] than expected [%f] over check interval [%llu], adjusting UP by %d.\n",
                        ppsOverCheckInterval, ppsExpectedForCheckInterval, elapsedSinceLastCheck, adjust);
            }
        } else if (ppsOverCheckInterval > ppsExpectedForCheckInterval) {
            int adjust = ppsOverCheckInterval - ppsExpectedForCheckInterval;
            packetsToSendThisCall -= adjust;
            if (wantDebugging) {
                qDebug("Higher pps [%f] than expected [%f] over check interval [%llu], adjusting DOWN by %d.\n",
                        ppsOverCheckInterval, ppsExpectedForCheckInterval, elapsedSinceLastCheck, adjust);
            }
        } else {
            if (wantDebugging) {
                qDebug("pps [%f] is expected [%f] over check interval [%llu], NO ADJUSTMENT.\n",
                        ppsOverCheckInterval, ppsExpectedForCheckInterval, elapsedSinceLastCheck);
            }
        }
        
        // now, do we want to reset the check interval? don't want to completely reset, because we would still have
        // a rounding error. instead, we check to see that we've passed the reset interval (which is much larger than
        // the check interval), and on those reset intervals we take the second half average and keep that for the next
        // interval window...
        if (wantDebugging) {
            printf(">>>>>>>>>>> RESET >>>>>>>>>>>>>>> Should we reset? callsPerSecond=%f elapsedSinceLastPPSCheck=%llu resetInterval=%f\n",
                callsPerSecond, elapsedSinceLastCheck, (averageCallTime * callIntervalsPerReset));
        }
        
        if (elapsedSinceLastCheck > (averageCallTime * callIntervalsPerReset)) {

            if (wantDebugging) {
                printf(">>>>>>>>>>> RESET >>>>>>>>>>>>>>> elapsedSinceLastCheck/2=%llu _packetsOverCheckInterval/2=%d\n",
                    (elapsedSinceLastCheck / 2), (_packetsOverCheckInterval / 2));
            }

            // Keep average packets and time for "second half" of check interval
            _lastPPSCheck += (elapsedSinceLastCheck / 2);
            _packetsOverCheckInterval = (_packetsOverCheckInterval / 2);

            elapsedSinceLastCheck = now - _lastPPSCheck;

            if (wantDebugging) {
                printf(">>>>>>>>>>> RESET >>>>>>>>>>>>>>> NEW _lastPPSCheck=%llu elapsedSinceLastCheck=%llu  _packetsOverCheckInterval=%d\n",
                    _lastPPSCheck, elapsedSinceLastCheck, _packetsOverCheckInterval);
            }
        }
    }
    
    int packetsLeft = _packets.size();

    if (wantDebugging) {
        printf("packetsSentThisCall=%d packetsToSendThisCall=%d packetsLeft=%d\n",
            packetsSentThisCall, packetsToSendThisCall, packetsLeft);
    }
    
    // Now that we know how many packets to send this call to process, just send them.
    while ((packetsSentThisCall < packetsToSendThisCall) && (packetsLeft > 0)) {
        lock();
        NetworkPacket& packet = _packets.front();
        NetworkPacket temporary = packet; // make a copy
        _packets.erase(_packets.begin());
        packetsLeft = _packets.size();
        unlock();

        // send the packet through the NodeList...
        NodeList::getInstance()->getNodeSocket().writeDatagram((char*) temporary.getData(), temporary.getLength(),
                                                               temporary.getSockAddr().getAddress(),
                                                               temporary.getSockAddr().getPort());
        packetsSentThisCall++;
        _packetsOverCheckInterval++;
        _totalPacketsSent++;
        _totalBytesSent += temporary.getLength();

        if (wantDebugging) {
            printf("nodeSocket->send()... packetsSentThisCall=%d _packetsOverCheckInterval=%d\n",
                packetsSentThisCall, _packetsOverCheckInterval);
        }
        if (_notify) {
            _notify->packetSentNotification(temporary.getLength());
        }
        _lastSendTime = now;
    }
    return isStillRunning();
}