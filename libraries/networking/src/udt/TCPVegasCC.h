//
//  TCPVegasCC.h
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2016-09-20.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_TCPVegasCC_h
#define hifi_TCPVegasCC_h

#include <map>

#include "CongestionControl.h"
#include "Constants.h"
#include "TCPRenoCC.h"

namespace udt {
    

class TCPVegasCC : public TCPRenoCC {
public:
    TCPVegasCC() {};

public:
    virtual void onACK(SequenceNumber ackNum) override;
    virtual void onLoss(SequenceNumber rangeStart, SequenceNumber rangeEnd) override {};
    virtual void onTimeout() override {};
    virtual void onPacketSent(int packetSize, SequenceNumber seqNum) override;
    
protected:
    virtual void performCongestionAvoidance(SequenceNumber ack, int numACK) override;
private:
    void adjustSlowStartThreshold()
        { _slowStartThreshold = std::min(_slowStartThreshold, (uint32_t) _congestionWindowSize - 1); }

    using TimeSizePair = std::pair<p_high_resolution_clock::time_point, int>;
    using PacketTimeList = std::map<SequenceNumber, TimeSizePair>;
    PacketTimeList _sentPacketTimes; // Map of sequence numbers to sent time

    int _currentMinRTT { 0x7FFFFFFF }; // Current RTT, in microseconds
    int _baseRTT { 0x7FFFFFFF }; // Lowest RTT during connection, in microseconds
    int _numRTT { 0 }; // Number of RTT collected during last RTT

    SequenceNumber _lastRTTMaxSeqNum; // Highest sent sequence number at time of last congestion window adjustment
};

}





#endif // hifi_TCPVegasCC_h
