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
    virtual bool onACK(SequenceNumber ackNum, p_high_resolution_clock::time_point receiveTime) override;
    virtual void onLoss(SequenceNumber rangeStart, SequenceNumber rangeEnd) override {};
    virtual void onTimeout() override {};

    virtual bool shouldNAK() override { return false; }
    virtual bool shouldACK2() override { return false; }
    virtual bool shouldProbe() override { return false; }

    virtual void onPacketSent(int packetSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) override;
    
protected:
    virtual void performCongestionAvoidance(SequenceNumber ack);
    virtual void setInitialSendSequenceNumber(SequenceNumber seqNum) override { _lastAck = seqNum - 1; }
private:
    bool isCongestionWindowLimited();
    void performRenoCongestionAvoidance(SequenceNumber ack);

    using TimeSizePair = std::pair<p_high_resolution_clock::time_point, int>;
    using PacketTimeList = std::map<SequenceNumber, TimeSizePair>;
    PacketTimeList _sentPacketTimes; // Map of sequence numbers to sent time

    p_high_resolution_clock::time_point _lastAdjustmentTime; // Time of last congestion control adjustment

    bool _slowStart { true }; // Marker for slow start phase

    SequenceNumber _lastAck; // Sequence number of last packet that was ACKed

    int _numACKSinceFastRetransmit { 3 }; // Number of ACKs received since last fast re-transmit

    int _currentMinRTT { 0x7FFFFFFF }; // Current RTT, in microseconds
    int _baseRTT { 0x7FFFFFFF }; // Lowest RTT during connection, in microseconds
    int _ewmaRTT { -1 }; // Exponential weighted moving average RTT
    int _rttVariance { 0 }; // Variance in collected RTT values

    int _numACKs { 0 }; // Holds the number of ACKs received during the last RTT

    int _ackAICount { 0 }; // Counter for number of ACKs received for Reno additive increase
    int _duplicateACKCount { 0 }; // Counter for duplicate ACKs received

    int _slowStartOddAdjust { 0 }; // Marker for every window adjustment every other RTT in slow-start

};

}





#endif // hifi_TCPVegasCC_h
