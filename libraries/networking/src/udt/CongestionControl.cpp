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

#include <SharedUtil.h>

using namespace udt;

void UdtCC::init() {
    _rcInterval = _synInterval;
    _lastRCTime = usecTimestampNow();
    setAckTimer(_rcInterval);
    
    _lastAck = _sendCurrSeqNum;
    _lastDecSeq = SequenceNumber{ SequenceNumber::MAX };
    
    _congestionWindowSize = 16.0;
    _packetSendPeriod = 1.0;
}

void UdtCC::onACK(SequenceNumber ackNum) {
    int64_t B = 0;
    double inc = 0;
    // Note: 1/24/2012
    // The minimum increase parameter is increased from "1.0 / _mss" to 0.01
    // because the original was too small and caused sending rate to stay at low level
    // for long time.
    const double min_inc = 0.01;
    
    uint64_t currtime = usecTimestampNow();
    if (currtime - _lastRCTime < (uint64_t)_rcInterval) {
        return;
    }
    
    _lastRCTime = currtime;
    
    if (_slowStart) {
        _congestionWindowSize += seqlen(_lastAck, ackNum);
        _lastAck = ackNum;
        
        if (_congestionWindowSize > _maxCongestionWindowSize) {
            _slowStart = false;
            if (_recvieveRate > 0) {
                _packetSendPeriod = 1000000.0 / _recvieveRate;
            } else {
                _packetSendPeriod = (_rtt + _rcInterval) / _congestionWindowSize;
            }
        }
    } else {
        _congestionWindowSize = _recvieveRate / 1000000.0 * (_rtt + _rcInterval) + 16;
    }
    
    // During Slow Start, no rate increase
    if (_slowStart) {
        return;
    }
    
    if (_loss) {
        _loss = false;
        return;
    }
    
    B = (int64_t)(_bandwidth - 1000000.0 / _packetSendPeriod);
    if ((_packetSendPeriod > _lastDecPeriod) && ((_bandwidth / 9) < B)) {
        B = _bandwidth / 9;
    }
    if (B <= 0) {
        inc = min_inc;
    } else {
        // inc = max(10 ^ ceil(log10( B * MSS * 8 ) * Beta / MSS, 1/MSS)
        // Beta = 1.5 * 10^(-6)
        
        inc = pow(10.0, ceil(log10(B * _mss * 8.0))) * 0.0000015 / _mss;
        
        if (inc < min_inc) {
            inc = min_inc;
        }
    }
    
    _packetSendPeriod = (_packetSendPeriod * _rcInterval) / (_packetSendPeriod * inc + _rcInterval);
}

void UdtCC::onLoss(const std::vector<SequenceNumber>& losslist) {
    //Slow Start stopped, if it hasn't yet
    if (_slowStart) {
        _slowStart = false;
        if (_recvieveRate > 0) {
            // Set the sending rate to the receiving rate.
            _packetSendPeriod = 1000000.0 / _recvieveRate;
            return;
        }
        // If no receiving rate is observed, we have to compute the sending
        // rate according to the current window size, and decrease it
        // using the method below.
        _packetSendPeriod = _congestionWindowSize / (_rtt + _rcInterval);
    }
    
    _loss = true;
    
    if (losslist[0] > _lastDecSeq) {
        _lastDecPeriod = _packetSendPeriod;
        _packetSendPeriod = ceil(_packetSendPeriod * 1.125);
        
        _avgNAKNum = (int)ceil(_avgNAKNum * 0.875 + _nakCount * 0.125);
        _nakCount = 1;
        _decCount = 1;
        
        _lastDecSeq = _sendCurrSeqNum;
        
        // remove global synchronization using randomization
        srand((uint32_t)_lastDecSeq);
        _decRandom = (int)ceil(_avgNAKNum * (double(rand()) / RAND_MAX));
        if (_decRandom < 1)
            _decRandom = 1;
    } else if ((_decCount ++ < 5) && (0 == (++ _nakCount % _decRandom))) {
        // 0.875^5 = 0.51, rate should not be decreased by more than half within a congestion period
        _packetSendPeriod = ceil(_packetSendPeriod * 1.125);
        _lastDecSeq = _sendCurrSeqNum;
    }
}

void UdtCC::onTimeout() {
    if (_slowStart) {
        _slowStart = false;
        if (_recvieveRate > 0) {
            _packetSendPeriod = 1000000.0 / _recvieveRate;
        } else {
            _packetSendPeriod = _congestionWindowSize / (_rtt + _rcInterval);
        }
    } else {
        /*
         _lastDecPeriod = _packetSendPeriod;
         _packetSendPeriod = ceil(_packetSendPeriod * 2);
         _lastDecSeq = _lastAck;
         */
    }
}
