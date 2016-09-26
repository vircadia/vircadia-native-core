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

namespace udt {
    

class TCPVegasCC : public CongestionControl {
public:
    TCPVegasCC();

public:
    virtual bool onACK(SequenceNumber ackNum) override;
    virtual void onLoss(SequenceNumber rangeStart, SequenceNumber rangeEnd) override {};
    virtual void onTimeout() override {};

    virtual bool shouldNAK() override { return false; }

    virtual void onPacketSent(int packetSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) override;
    
protected:
    virtual void performCongestionAvoidance(SequenceNumber ack);
    virtual void setInitialSendSequenceNumber(SequenceNumber seqNum) override { _lastAck = seqNum - 1; }
private:

    using TimeSizePair = std::pair<p_high_resolution_clock::time_point, int>;
    using PacketTimeList = std::map<SequenceNumber, TimeSizePair>;
    PacketTimeList _sentPacketTimes; // Map of sequence numbers to sent time

    p_high_resolution_clock::time_point _lastAdjustmentTime; // Time of last congestion control adjustment

    bool _slowStart { false }; // Marker for slow start phase

    SequenceNumber _lastAck; // Sequence number of last packet that was ACKed

    int _numACKSinceFastRetransmit { 0 }; // Number of ACKs received since last fast re-transmit

    int _currentMinRTT { 0x7FFFFFFF }; // Current RTT, in microseconds
    int _baseRTT { 0x7FFFFFFF }; // Lowest RTT during connection, in microseconds
    int _numRTT { 0 }; // Number of RTT collected during last RTT
    int _ewmaRTT { 0 }; // Exponential weighted moving average RTT
    int _rttVariance { 0 }; // Variance in collected RTT values

    int _slowStartOddAdjust { 0 }; // Marker for every window adjustment every other RTT in slow-start

};

}





#endif // hifi_TCPVegasCC_h
