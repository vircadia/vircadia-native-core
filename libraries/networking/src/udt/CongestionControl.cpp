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

#include <random>

#include "Packet.h"

using namespace udt;
using namespace std::chrono;

static const double USECS_PER_SECOND = 1000000.0;

void CongestionControl::setPacketSendPeriod(double newSendPeriod) {
    Q_ASSERT_X(newSendPeriod >= 0, "CongestionControl::setPacketPeriod", "Can not set a negative packet send period");
    
    if (_maxBandwidth > 0) {
        // anytime the packet send period is about to be increased, make sure it stays below the minimum period,
        // calculated based on the maximum desired bandwidth
        double minPacketSendPeriod = USECS_PER_SECOND / (((double) _maxBandwidth) / _mss);
        _packetSendPeriod = std::max(newSendPeriod, minPacketSendPeriod);
    } else {
        _packetSendPeriod = newSendPeriod;
    }
}

DefaultCC::DefaultCC() :
    _lastRCTime(high_resolution_clock::now()),
    _slowStartLastAck(_sendCurrSeqNum),
    _lastDecreaseMaxSeq(SequenceNumber {SequenceNumber::MAX })
{
    _mss = udt::MAX_PACKET_SIZE_WITH_UDP_HEADER;
    
    _congestionWindowSize = 16.0;
    _packetSendPeriod = 1.0;
}

void DefaultCC::onACK(SequenceNumber ackNum) {
    double increase = 0;
    
    // Note from UDT original code:
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
    
    double capacitySpeedDelta = (_bandwidth - USECS_PER_SECOND / _packetSendPeriod);
    
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
    
    setPacketSendPeriod((_packetSendPeriod * synInterval()) / (_packetSendPeriod * increase + synInterval()));
}

void DefaultCC::onLoss(SequenceNumber rangeStart, SequenceNumber rangeEnd) {
    // stop the slow start if we haven't yet
    if (_slowStart) {
        stopSlowStart();
        
        // if the change to send rate was driven by a known receive rate, then we don't continue with the decrease
        if (_receiveRate > 0) {
            return;
        }
    }
    
    _loss = true;
    
    static const double INTER_PACKET_ARRIVAL_INCREASE = 1.125;
    static const int MAX_DECREASES_PER_CONGESTION_EPOCH = 5;
    
    // check if this NAK starts a new congestion period - this will be the case if the
    // NAK received occured for a packet sent after the last decrease
    if (rangeStart > _lastDecreaseMaxSeq) {
        _lastDecreasePeriod = _packetSendPeriod;
        
        _packetSendPeriod = ceil(_packetSendPeriod * INTER_PACKET_ARRIVAL_INCREASE);
        
        // use EWMA to update the average number of NAKs per congestion
        static const double NAK_EWMA_ALPHA = 0.125;
        _avgNAKNum = (int)ceil(_avgNAKNum * (1 - NAK_EWMA_ALPHA) + _nakCount * NAK_EWMA_ALPHA);
        
        // update the count of NAKs and count of decreases in this interval
        _nakCount = 1;
        _decreaseCount = 1;
        
        _lastDecreaseMaxSeq = _sendCurrSeqNum;
        
        // avoid synchronous rate decrease across connections using randomization
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(1, _avgNAKNum);
        
        _randomDecreaseThreshold = distribution(generator);
        
    } else if ((_decreaseCount++ < MAX_DECREASES_PER_CONGESTION_EPOCH) && ((++_nakCount % _randomDecreaseThreshold) == 0)) {
        // there have been fewer than MAX_DECREASES_PER_CONGESTION_EPOCH AND this NAK matches the random count at which we
        // decided we would decrease the packet send period
        
        _packetSendPeriod = ceil(_packetSendPeriod * INTER_PACKET_ARRIVAL_INCREASE);
        _lastDecreaseMaxSeq = _sendCurrSeqNum;
    }
}

// Note: This isn't currently being called by anything since we, unlike UDT, don't have TTL on our packets
void DefaultCC::onTimeout() {
    if (_slowStart) {
        stopSlowStart();
    } else {
        // UDT used to do the following on timeout if not in slow start - we should check if it could be helpful
        // _lastDecreasePeriod = _packetSendPeriod;
        // _packetSendPeriod = ceil(_packetSendPeriod * 2);
        
        // this seems odd - the last ack they were setting _lastDecreaseMaxSeq to only applies to slow start
        // _lastDecreaseMaxSeq = _slowStartLastAck;
    }
}

void DefaultCC::stopSlowStart() {
    _slowStart = false;
    
    if (_receiveRate > 0) {
        // Set the sending rate to the receiving rate.
        _packetSendPeriod = USECS_PER_SECOND / _receiveRate;
    } else {
        // If no receiving rate is observed, we have to compute the sending
        // rate according to the current window size, and decrease it
        // using the method below.
        _packetSendPeriod = _congestionWindowSize / (_rtt + synInterval());
    }
}
