//
//  PacketTimeWindow.cpp
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-28.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketTimeWindow.h"

#include <cmath>

#include <NumericalConstants.h>

using namespace udt;
using namespace std::chrono;

PacketTimeWindow::PacketTimeWindow(int numPacketIntervals, int numProbeIntervals) :
    _numPacketIntervals(numPacketIntervals),
    _numProbeIntervals(numProbeIntervals),
    _packetIntervals({ _numPacketIntervals }),
    _probeIntervals({ _numProbeIntervals })
{
    
}

int32_t meanOfMedianFilteredValues(std::vector<int> intervals, int numValues, int valuesRequired = 0) {
    // sort the intervals from smallest to largest
    std::sort(intervals.begin(), intervals.end());
    
    int median = 0;
    if (numValues % 2 == 0) {
        median = intervals[numValues / 2];
    } else {
        median = (intervals[(numValues / 2) - 1] + intervals[numValues / 2]) / 2;
    }
    
    int count = 0;
    int sum = 0;
    int upperBound = median * 8;
    int lowerBound = median / 8;
    
    for (auto& interval : intervals) {
        if ((interval < upperBound) && interval > lowerBound) {
            ++count;
            sum += interval;
        }
    }
    
    if (count >= valuesRequired) {
        return (int32_t) ceil((double) USECS_PER_MSEC / ((double) sum) / ((double) count));
    } else {
        return 0;
    }
}

int32_t PacketTimeWindow::getPacketReceiveSpeed() const {
    // return the mean value of median filtered values (per second) - or zero if there are too few filtered values
    return meanOfMedianFilteredValues(_packetIntervals, _numPacketIntervals, _numPacketIntervals / 2);
}

int32_t PacketTimeWindow::getEstimatedBandwidth() const {
    // return mean value of median filtered values (per second)
    return meanOfMedianFilteredValues(_probeIntervals, _numProbeIntervals);
}

void PacketTimeWindow::onPacketArrival() {
    // take the current time
    auto now = high_resolution_clock::now();
    
    // record the interval between this packet and the last one
    _packetIntervals[_currentPacketInterval++] = duration_cast<microseconds>(now - _lastPacketTime).count();
    
    // reset the currentPacketInterval index when it wraps
    if (_currentPacketInterval == _numPacketIntervals) {
        _currentPacketInterval = 0;
    }
    
    // remember this as the last packet arrival time
    _lastPacketTime = now;
}

void PacketTimeWindow::onProbePair1Arrival() {
    // take the current time as the first probe time
    _firstProbeTime = high_resolution_clock::now();
}

void PacketTimeWindow::onProbePair2Arrival() {
    // store the interval between the two probes
    auto now = high_resolution_clock::now();
    
    _probeIntervals[_currentProbeInterval++] = duration_cast<microseconds>(now - _firstProbeTime).count();
    
    // reset the currentProbeInterval index when it wraps
    if (_currentProbeInterval == _numProbeIntervals) {
        _currentProbeInterval = 0;
    }
}
