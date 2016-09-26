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

#include <QtCore/QDebug>
#include <QtCore/QtGlobal>

using namespace udt;
using namespace std::chrono;

TCPVegasCC::TCPVegasCC() {
    _packetSendPeriod = 0.0;
    _congestionWindowSize = 2;

    setAckInterval(1); // TCP sends an ACK for every packet received
}

bool TCPVegasCC::onACK(SequenceNumber ack) {
    auto it = _sentPacketTimes.find(ack);

    if (it != _sentPacketTimes.end()) {

        // calculate the RTT (time now - time ACK sent)
        auto now = p_high_resolution_clock::now();
        int lastRTT = duration_cast<microseconds>(now - it->second.first).count();

        if (lastRTT < 0) {
            Q_ASSERT_X(false, "TCPVegasCC::onACK", "calculated an RTT that is not > 0");
            return false;
        } else if (lastRTT == 0) {
            // we do not allow a 0 microsecond RTT
            lastRTT = 1;
        }

        static const int RTT_ESTIMATION_ALPHA_NUMERATOR = 8;
        static const int RTT_ESTIMATION_VARIANCE_ALPHA_NUMERATOR = 4;

        _ewmaRTT = (_ewmaRTT * (RTT_ESTIMATION_ALPHA_NUMERATOR - 1) + lastRTT) / RTT_ESTIMATION_ALPHA_NUMERATOR;
        _rttVariance = (_rttVariance * (RTT_ESTIMATION_VARIANCE_ALPHA_NUMERATOR - 1)
                        + abs(lastRTT - _ewmaRTT)) / RTT_ESTIMATION_VARIANCE_ALPHA_NUMERATOR;

        // keep track of the lowest RTT during connection
        _baseRTT = std::min(_baseRTT, lastRTT);

        // find the min RTT during the last RTT
        _currentMinRTT = std::min(_currentMinRTT, lastRTT);

        auto sinceLastAdjustment = duration_cast<microseconds>(now - _lastAdjustmentTime).count();
        if (sinceLastAdjustment >= _ewmaRTT) {
            performCongestionAvoidance(ack);
        }

        // remove this sent packet time from the hash
        _sentPacketTimes.erase(it);

    } else {
        Q_ASSERT_X(false,
                   "TCPVegasCC::onACK",
                   "called with a sequence number that has not been sent");
    }


    // we check if we need a fast re-transmit if this is a duplicate ACK or if this is the first or second ACK
    // after a previous fast re-transmit
    ++_numACKSinceFastRetransmit;

    if (ack == _lastAck || _numACKSinceFastRetransmit < 3) {
        // we may need to re-send ackNum + 1 if it has been more than our estimated timeout since it was sent
        qDebug() << "FRT:" << (uint32_t) ack <<  (uint32_t) _lastAck << _numACKSinceFastRetransmit;

        auto it = _sentPacketTimes.find(ack + 1);
        if (it != _sentPacketTimes.end()) {
            auto estimatedTimeout = _ewmaRTT + _rttVariance * 4;

            auto now = p_high_resolution_clock::now();
            auto sinceSend = duration_cast<microseconds>(now - it->second.first).count();

            if (sinceSend >= estimatedTimeout) {
                // we've detected we need a fast re-transmit, send that back to the connection
                _numACKSinceFastRetransmit = 0;
                return true;
            }
        }
    }

    // ACK processed, no fast re-transmit required
    return false;
}

void TCPVegasCC::performCongestionAvoidance(udt::SequenceNumber ack) {
    static int VEGAS_ALPHA_SEGMENTS = 2;
    static int VEGAS_BETA_SEGMENTS = 4;
    static int VEGAS_GAMMA_SEGMENTS = 1;

    qDebug() << "============";
    qDebug() << "PCA" << _numRTT;
    qDebug() << "CWS:" << _congestionWindowSize << "SS:" << _slowStart;

    // Use the Vegas algorithm to see if we should
    // increase or decrease the congestion window size, and by how much

    // Grab the minimum RTT seen during the last RTT
    // Taking the min avoids the effects of delayed ACKs
    // (though congestion may be noticed a bit later)
    int rtt = _currentMinRTT;

    int windowSizeDiff = _congestionWindowSize * (rtt - _baseRTT) / _baseRTT;

    qDebug() << "BRTT:" << _baseRTT << "CRTT:" << _currentMinRTT << "ERTT:" << _ewmaRTT;
    qDebug() << "D:" << windowSizeDiff;

    if (_slowStart) {
        if (windowSizeDiff > VEGAS_GAMMA_SEGMENTS) {
            // we're going too fast - this breaks us out of slow start and we switch to linear increase/decrease
            _slowStart = false;

             int expectedWindowSize = _congestionWindowSize * _baseRTT / rtt;

            qDebug() << "EWS:" << expectedWindowSize;

            // drop the congestion window size to the expected size, if smaller
            _congestionWindowSize = std::min(_congestionWindowSize, expectedWindowSize + 1);
            
        } else if (++_slowStartOddAdjust & 1) {
            // we're in slow start and not going too fast
            // this means that once every second RTT we perform exponential congestion window growth
            _congestionWindowSize *= 2;
        }
    } else {
        // this is the normal linear increase/decrease of the Vegas algorithm
        // to figure out where the congestion window should be
        if (windowSizeDiff > VEGAS_BETA_SEGMENTS) {
            // the old congestion window was too fast (difference > beta)
            // so reduce it to slow down
            --_congestionWindowSize;

        } else if (windowSizeDiff < VEGAS_ALPHA_SEGMENTS) {
            // there aren't enough packets on the wire, add more to the congestion window
            ++_congestionWindowSize;
        } else {
            // sending rate seems good, no congestion window adjustment
        }
    }

    // we never allow the congestion window to be smaller than two packets
    static int VEGAS_CW_MIN_PACKETS = 2;
    _congestionWindowSize = std::min(_congestionWindowSize, VEGAS_CW_MIN_PACKETS);

    // mark this as the last adjustment time
    _lastAdjustmentTime = p_high_resolution_clock::now();

    // reset our state for the next RTT
    _currentMinRTT = std::numeric_limits<int>::max();

    qDebug() << "CW:" << _congestionWindowSize << "SS:" << _slowStart;
    qDebug() << "============";
}


void TCPVegasCC::onPacketSent(int packetSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) {
    _sentPacketTimes[seqNum] = { timePoint, packetSize };
}

