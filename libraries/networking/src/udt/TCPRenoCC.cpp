//
//  TCPRenoCC.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 9/20/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TCPRenoCC.h"

#include <algorithm>

using namespace udt;

void TCPRenoCC::init() {
    _packetSendPeriod = 0.0;
    _congestionWindowSize = 2.0;

    setAckInterval(1);
    setRTO(1000000);
}

void TCPRenoCC::onACK(SequenceNumber ackNum) {
    int numAcked = seqlen(_lastACK, ackNum);

    performCongestionAvoidance(ackNum, numAcked);

    _lastACK = ackNum;
}

bool TCPRenoCC::isInSlowStart() {
    return _congestionWindowSize < _sendSlowStartThreshold;
}

int TCPRenoCC::performSlowStart(int numAcked) {
    int congestionWindow = std::min(_congestionWindowSize + numAcked, _sendSlowStartThreshold);
    numAcked -= congestionWindow - _congestionWindowSize;
    _congestionWindowSize = congestionWindow;

    return numAcked;
}

int TCPRenoCC::slowStartThreshold() {
    // Slow start threshold is half the congestion window (min 2)
    return std::max(_congestionWindowSize >> 1, 2);
}

bool TCPRenoCC::isCongestionWindowLimited() {
    if (isInSlowStart()) {
        return _congestionWindowSize < 2 * _maxPacketsOut;
    }

    return _isCongestionWindowLimited;
}

void TCPRenoCC::performCongestionAvoidance(SequenceNumber ack, int numAcked) {
    if (!isCongestionWindowLimited()) {
        return;
    }

    // In "safe" area, increase.
    if (isInSlowStart()) {
        numAcked = performSlowStart(numAcked);
        if (!numAcked) {
            return;
        }
    }

    // In dangerous area, increase slowly.
    performCongestionAvoidanceAI(_congestionWindowSize, numAcked);
}

void TCPRenoCC::performCongestionAvoidanceAI(int sendCongestionWindowSize, int numAcked) {
    // If credits accumulated at a higher w, apply them gently now.
    if (_sendCongestionWindowCount >= sendCongestionWindowSize) {
        _sendCongestionWindowCount = 0;
        _congestionWindowSize++;
    }

    _sendCongestionWindowCount += numAcked;
    if (_sendCongestionWindowCount >= sendCongestionWindowSize) {
        int delta = _sendCongestionWindowCount / sendCongestionWindowSize;

        _sendCongestionWindowCount -= delta * sendCongestionWindowSize;
        _sendCongestionWindowCount += delta;
    }
}

