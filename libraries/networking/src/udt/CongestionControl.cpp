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
static const int BITS_PER_BYTE = 8;

void CongestionControl::setMaxBandwidth(int maxBandwidth) {
    _maxBandwidth = maxBandwidth;
    setPacketSendPeriod(_packetSendPeriod);
}

void CongestionControl::setPacketSendPeriod(double newSendPeriod) {
    Q_ASSERT_X(newSendPeriod >= 0, "CongestionControl::setPacketPeriod", "Can not set a negative packet send period");

    auto packetsPerSecond = (double)_maxBandwidth / (BITS_PER_BYTE * _mss);
    if (packetsPerSecond > 0.0) {
        // anytime the packet send period is about to be increased, make sure it stays below the minimum period,
        // calculated based on the maximum desired bandwidth
        double minPacketSendPeriod = USECS_PER_SECOND / packetsPerSecond;
        _packetSendPeriod = std::max(newSendPeriod, minPacketSendPeriod);
    } else {
        _packetSendPeriod = newSendPeriod;
    }
}
