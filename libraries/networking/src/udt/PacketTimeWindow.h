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

#include <vector>

#include <PortableHighResolutionClock.h>

namespace udt {
    
class PacketTimeWindow {
public:
    PacketTimeWindow(int numPacketIntervals = 16);
    
    void onPacketArrival();
    
    int32_t getPacketReceiveSpeed() const;
    
    void reset();
private:    
    int _numPacketIntervals { 0 }; // the number of packet intervals to store
    
    int _currentPacketInterval { 0 }; // index for the current packet interval
    
    std::vector<int> _packetIntervals; // vector of microsecond intervals between packet arrivals
    
    p_high_resolution_clock::time_point _lastPacketTime = p_high_resolution_clock::now(); // the time_point when last packet arrived
};
    
}

#endif // hifi_PacketTimeWindow_h
