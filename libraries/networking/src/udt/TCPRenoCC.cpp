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


using namespace udt;

void TCPRenoCC::init() {
    _slowStart = true;
    _issThreshold = 83333;

    _packetSendPeriod = 0.0;
    _congestionWindowSize = 2.0;

    setAckInterval(2);
    setRTO(1000000);
}

void TCPRenoCC::onACK(SequenceNumber ackNum) {
    if (ackNum == _lastACK) {
        if (3 == ++_duplicateAckCount) {
            duplicateACKAction();
        } else if (_duplicateAckCount > 3) {
            _congestionWindowSize += 1.0;
        } else {
            ackAction();
        }
    } else {
        if (_duplicateAckCount >= 3) {
            _congestionWindowSize = _issThreshold;
        }

        _lastACK = ackNum;
        _duplicateAckCount = 1;

        ackAction();
    }
}

void TCPRenoCC::onTimeout() {
    _issThreshold = seqlen(_lastACK, _sendCurrSeqNum) / 2;
    if (_issThreshold < 2) {
        _issThreshold = 2;
    }

    _slowStart = true;
    _congestionWindowSize = 2.0;
}

void TCPRenoCC::ackAction() {
    if (_slowStart) {
        _congestionWindowSize += 1.0;

        if (_congestionWindowSize >= _issThreshold) {
            _slowStart = false;
        }
    } else {
        _congestionWindowSize += 1.0 / _congestionWindowSize;
    }
}

void TCPRenoCC::duplicateACKAction() {
    _slowStart = false;

    _issThreshold = seqlen(_lastACK, _sendCurrSeqNum) / 2;
    if (_issThreshold < 2) {
        _issThreshold = 2;
    }

    _congestionWindowSize = _issThreshold + 3;
}

void TCPRenoCC::setInitialSendSequenceNumber(SequenceNumber seqNum) {
    _lastACK = seqNum - 1;
}
