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
    
    _lastAck = _sendCurrSeqNum;
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
    
    auto now = high_resolution_clock::now();
    if (duration_cast<microseconds>(now - _lastRCTime).count() < synInterval()) {
        return;
    }
    
    _lastRCTime = now;
    
    if (_slowStart) {
        _congestionWindowSize += seqlen(_lastAck, ackNum);
        _lastAck = ackNum;
        
        if (_congestionWindowSize > _maxCongestionWindowSize) {
            _slowStart = false;
            if (_receiveRate > 0) {
                _packetSendPeriod = USECS_PER_SECOND / _receiveRate;
            } else {
                _packetSendPeriod = (_rtt + synInterval()) / _congestionWindowSize;
            }
        }
    } else {
        _congestionWindowSize = _receiveRate / USECS_PER_SECOND * (_rtt + synInterval()) + 16;
    }
    
    // During Slow Start, no rate increase
    if (_slowStart) {
        return;
    }
    
    if (_loss) {
        _loss = false;
        return;
    }
    
    int capacitySpeedDelta = (int) (_bandwidth - USECS_PER_SECOND / _packetSendPeriod);
    
    if ((_packetSendPeriod > _lastDecreasePeriod) && ((_bandwidth / 9) < capacitySpeedDelta)) {
        capacitySpeedDelta = _bandwidth / 9;
    }
    
    if (capacitySpeedDelta <= 0) {
        increase = minimumIncrease;
    } else {
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
    
    if (rangeStart > _lastDecreaseMaxSeq) {
        _lastDecreasePeriod = _packetSendPeriod;
        _packetSendPeriod = ceil(_packetSendPeriod * 1.125);
        
        _avgNAKNum = (int)ceil(_avgNAKNum * 0.875 + _nakCount * 0.125);
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
