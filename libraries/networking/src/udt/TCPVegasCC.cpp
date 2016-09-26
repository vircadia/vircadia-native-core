//
//  TCPVegasCC.cpp
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2016-09-20.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TCPVegasCC.h"

#include <QtCore/QtGlobal>

using namespace udt;
using namespace std::chrono;

void TCPVegasCC::onACK(SequenceNumber ackNum) {
    auto it = _sentPacketTimes.find(ackNum);

    if (it != _sentPacketTimes.end()) {

        // calculate the RTT (time now - time ACK sent)
        auto now = p_high_resolution_clock::now();
        int lastRTT = duration_cast<microseconds>(now - it->second.first).count();

        if (lastRTT < 0) {
            Q_ASSERT_X(false, "TCPVegasCC::onACK", "calculated an RTT that is not > 0");
            return;
        } else if (lastRTT == 0) {
            // we do not allow a 0 microsecond RTT
            lastRTT = 1;
        }

        // keep track of the lowest RTT during connection
        _baseRTT = std::min(_baseRTT, lastRTT);

        // find the min RTT during the last RTT
        _currentMinRTT = std::min(_currentMinRTT, lastRTT);

        ++_numRTT;

        if (ackNum > _lastRTTMaxSeqNum) {
            int numACK = seqlen(_lastACK, ackNum);
            performCongestionAvoidance(ackNum, numACK);
        }

        _lastACK = ackNum;

        // remove this sent packet time from the hash
        _sentPacketTimes.erase(it);

    } else {
        Q_ASSERT_X(false,
                   "TCPVegasCC::onACK",
                   "called with a sequence number that has not been sent");
    }
}

void TCPVegasCC::performCongestionAvoidance(udt::SequenceNumber ack, int numAcked) {
    static const int VEGAS_MIN_RTT_FOR_CALC = 3;

    static const int VEGAS_ALPHA_SEGMENTS = 2;
    static const int VEGAS_BETA_SEGMENTS = 4;
    static const int VEGAS_GAMMA_SEGMENTS = 1;

    if (_numRTT < VEGAS_MIN_RTT_FOR_CALC) {
        // Vegas calculations are only done if there are enough RTT samples to be
        // pretty sure that at least one sample did not come from a delayed ACK.
        // If that is the case, we fallback to the Reno behaviour

        TCPRenoCC::performCongestionAvoidance(ack, numAcked);
    } else {
        // There are enough RTT samples, use the Vegas algorithm to see if we should
        // increase or decrease the congestion window size, and by how much

        // Grab the minimum RTT seen during the last RTT
        // Taking the min avoids the effects of delayed ACKs
        // (though congestion may be noticed a bit later)
        int rtt = _currentMinRTT;

        int expectedWindowSize = _congestionWindowSize * _baseRTT / rtt;
        int diff = _congestionWindowSize * (rtt - _baseRTT) / _baseRTT;

        bool inWindowReduction = false;

        if (diff > VEGAS_GAMMA_SEGMENTS && isInSlowStart()) {
            // we're going too fast, slow down and switch to congestion avoidance

            // the congestion window should match the actual rate
            _congestionWindowSize = std::min(_congestionWindowSize, expectedWindowSize + 1);
            adjustSlowStartThreshold();

            inWindowReduction = true;
        } else if (isInSlowStart()) {
            // slow start
            performSlowStart(numAcked);
        } else {
            // figure out where the congestion window should be
            if (diff > VEGAS_BETA_SEGMENTS) {
                // the old congestion window was too fast (difference > beta)
                // so reduce it to slow down
                --_congestionWindowSize;
                adjustSlowStartThreshold();

                inWindowReduction = true;
            } else if (diff < VEGAS_ALPHA_SEGMENTS) {
                // there aren't enough packets on the wire, add more to the congestion window
                ++_congestionWindowSize;
            } else {
                // sending rate seems good, no congestion window adjustment
            }
        }

        // we never allow the congestion window to be smaller than two packets
        static int VEGAS_CW_MIN_PACKETS = 2;
        _congestionWindowSize = std::min(_congestionWindowSize, VEGAS_CW_MIN_PACKETS);

        if (!inWindowReduction && _congestionWindowSize > _sendSlowStartThreshold) {
            // if we didn't just reduce the congestion window size and the
            // the congestion window is greater than the slow start threshold
            // we raise the slow start threshold half the distance to the congestion window
            _sendSlowStartThreshold = (_congestionWindowSize >> 1) + (_congestionWindowSize >> 2);
        }

        _lastRTTMaxSeqNum = _sendCurrSeqNum;

        // reset our state for the next RTT
        _currentMinRTT = std::numeric_limits<int>::max();
        _numRTT = 0;
    }
}


void TCPVegasCC::onPacketSent(int packetSize, SequenceNumber seqNum) {
    _sentPacketTimes[seqNum] = { p_high_resolution_clock::now(), packetSize };
}

