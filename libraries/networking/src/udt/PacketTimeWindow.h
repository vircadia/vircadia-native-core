//
//  PacketTimeWindow.h
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-28.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_PacketTimeWindow_h
#define hifi_PacketTimeWindow_h

#include <chrono>
#include <vector>

namespace udt {
    
class PacketTimeWindow {
public:
    PacketTimeWindow(int numPacketIntervals = 16, int numProbeIntervals = 16);
    
    void onPacketArrival();
    void onProbePair1Arrival();
    void onProbePair2Arrival();
    
    int32_t getPacketReceiveSpeed() const;
    int32_t getEstimatedBandwidth() const;
    
    void reset();
private:    
    int _numPacketIntervals { 0 }; // the number of packet intervals to store
    int _numProbeIntervals { 0 }; // the number of probe intervals to store
    
    int _currentPacketInterval { 0 }; // index for the current packet interval
    int _currentProbeInterval { 0 }; // index for the current probe interval
    
    std::vector<int> _packetIntervals; // vector of microsecond intervals between packet arrivals
    std::vector<int> _probeIntervals; // vector of microsecond intervals between probe pair arrivals
    
    std::chrono::high_resolution_clock::time_point _lastPacketTime; // the time_point when last packet arrived
    std::chrono::high_resolution_clock::time_point _firstProbeTime; // the time_point when first probe in pair arrived
};
    
}

#endif // hifi_PacketTimeWindow_h
