//
//  CongestionControl.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 7/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CongestionControl.h"

using namespace udt;
using namespace std::chrono;

static const double USECS_PER_SECOND = 1000000.0;

void DefaultCC::init() {
    _lastRCTime = high_resolution_clock::now();
    setAckTimer(synInterval());
    
    _slowStartLastAck = _sendCurrSeqNum;
    _lastDecreaseMaxSeq = SequenceNumber { SequenceNumber::MAX };
    
    _congestionWindowSize = 16.0;
    _packetSendPeriod = 1.0;
}

void DefaultCC::onACK(SequenceNumber ackNum) {
    double increase = 0;
    
    // Note: 1/24/2012
    // The minimum increase parameter is increased from "1.0 / _mss" to 0.01
    // because the original was too small and caused sending rate to stay at low level
    // for long time.
    const double minimumIncrease = 0.01;
    
    // we will only adjust once per sync interval so check that it has been at least that long now
    auto now = high_resolution_clock::now();
    if (duration_cast<microseconds>(now - _lastRCTime).count() < synInterval()) {
        return;
    }
    
    // our last rate increase time is now
    _lastRCTime = now;

    if (_slowStart) {
        // we are in slow start phase - increase the congestion window size by the number of packets just ACKed
        _congestionWindowSize += seqlen(_slowStartLastAck, ackNum);
        
        // update the last ACK
        _slowStartLastAck = ackNum;
        
        // check if we can get out of slow start (is our new congestion window size bigger than the max)
        if (_congestionWindowSize > _maxCongestionWindowSize) {
            _slowStart = false;
            
            if (_receiveRate > 0) {
                // if we have a valid receive rate we set the send period to whatever the receive rate dictates
                _packetSendPeriod = USECS_PER_SECOND / _receiveRate;
            } else {
                // no valid receive rate, packet send period is dictated by estimated RTT and current congestion window size
                _packetSendPeriod = (_rtt + synInterval()) / _congestionWindowSize;
            }
        }
    } else {
        // not in slow start - window size should be arrival rate * (RTT + SYN) + 16
        _congestionWindowSize = _receiveRate / USECS_PER_SECOND * (_rtt + synInterval()) + 16;
    }
    
    // during slow start we perform no rate increases
    if (_slowStart) {
        return;
    }
    
    // if loss has happened since the last rate increase we do not perform another increase
    if (_loss) {
        _loss = false;
        return;
    }
    
    int capacitySpeedDelta = (int) (_bandwidth - USECS_PER_SECOND / _packetSendPeriod);
    
    // UDT uses what they call DAIMD - additive increase multiplicative decrease with decreasing increases
    // This factor is a protocol parameter that is part of the DAIMD algorithim
    static const int AIMD_DECREASING_INCREASE_FACTOR = 9;
    
    if ((_packetSendPeriod > _lastDecreasePeriod) && ((_bandwidth / AIMD_DECREASING_INCREASE_FACTOR) < capacitySpeedDelta)) {
        capacitySpeedDelta = _bandwidth / AIMD_DECREASING_INCREASE_FACTOR;
    }
    
    if (capacitySpeedDelta <= 0) {
        increase = minimumIncrease;
    } else {
        // use UDTs DAIMD algorithm to figure out what the send period increase factor should be
        
        // inc = max(10 ^ ceil(log10(B * MSS * 8 ) * Beta / MSS, minimumIncrease)
        // B = estimated link capacity
        // Beta = 1.5 * 10^(-6)
        
        static const double BETA = 0.0000015;
        static const double BITS_PER_BYTE = 8.0;
        
        increase = pow(10.0, ceil(log10(capacitySpeedDelta * _mss * BITS_PER_BYTE))) * BETA / _mss;
        
        if (increase < minimumIncrease) {
            increase = minimumIncrease;
        }
    }
    
    _packetSendPeriod = (_packetSendPeriod * synInterval()) / (_packetSendPeriod * increase + synInterval());
}

void DefaultCC::onLoss(SequenceNumber rangeStart, SequenceNumber rangeEnd) {
    //Slow Start stopped, if it hasn't yet
    if (_slowStart) {
        _slowStart = false;
        if (_receiveRate > 0) {
            // Set the sending rate to the receiving rate.
            _packetSendPeriod = USECS_PER_SECOND / _receiveRate;
            return;
        }
        // If no receiving rate is observed, we have to compute the sending
        // rate according to the current window size, and decrease it
        // using the method below.
        _packetSendPeriod = _congestionWindowSize / (_rtt + synInterval());
    }
    
    _loss = true;
    
    // check if this NAK starts a new congestion period - this will be the case if the
    // NAK received occured for a packet sent after the last decrease
    if (rangeStart > _lastDecreaseMaxSeq) {
        _lastDecreasePeriod = _packetSendPeriod;
        
        static const double INTER_PACKET_ARRIVAL_INCREASE = 1.125;
        _packetSendPeriod = ceil(_packetSendPeriod * INTER_PACKET_ARRIVAL_INCREASE);
        
        // use EWMA to update the average number of NAKs per congestion
        static const double NAK_EWMA_ALPHA = 0.125;
        _avgNAKNum = (int)ceil(_avgNAKNum * (1 - NAK_EWMA_ALPHA) + _nakCount * NAK_EWMA_ALPHA);
        
        
        _nakCount = 1;
        _decCount = 1;
        
        _lastDecreaseMaxSeq = _sendCurrSeqNum;
        
        // remove global synchronization using randomization
        srand((uint32_t)_lastDecreaseMaxSeq);
        _decRandom = (int)ceil(_avgNAKNum * (double(rand()) / RAND_MAX));
        
        if (_decRandom < 1) {
            _decRandom = 1;
        }
        
    } else if ((_decCount ++ < 5) && (0 == (++ _nakCount % _decRandom))) {
        // 0.875^5 = 0.51, rate should not be decreased by more than half within a congestion period
        _packetSendPeriod = ceil(_packetSendPeriod * 1.125);
        _lastDecreaseMaxSeq = _sendCurrSeqNum;
    }
}

void DefaultCC::onTimeout() {
    if (_slowStart) {
        _slowStart = false;
        if (_receiveRate > 0) {
            _packetSendPeriod = USECS_PER_SECOND / _receiveRate;
        } else {
            _packetSendPeriod = _congestionWindowSize / (_rtt + synInterval());
        }
    } else {
        /*
         _lastDecPeriod = _packetSendPeriod;
         _packetSendPeriod = ceil(_packetSendPeriod * 2);
         _lastDecSeq = _lastAck;
         */
    }
}
