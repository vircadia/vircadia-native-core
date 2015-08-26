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

#include <numeric>
#include <cmath>

#include <NumericalConstants.h>

using namespace udt;
using namespace std::chrono;

static const int DEFAULT_PACKET_INTERVAL_MICROSECONDS = 1000000; // 1s
static const int DEFAULT_PROBE_INTERVAL_MICROSECONDS = 1000; // 1ms

PacketTimeWindow::PacketTimeWindow(int numPacketIntervals, int numProbeIntervals) :
    _numPacketIntervals(numPacketIntervals),
    _numProbeIntervals(numProbeIntervals),
    _packetIntervals(_numPacketIntervals, DEFAULT_PACKET_INTERVAL_MICROSECONDS),
    _probeIntervals(_numProbeIntervals, DEFAULT_PROBE_INTERVAL_MICROSECONDS)
{
    
}

void PacketTimeWindow::reset() {
    _packetIntervals.assign(_numPacketIntervals, DEFAULT_PACKET_INTERVAL_MICROSECONDS);
    _probeIntervals.assign(_numProbeIntervals, DEFAULT_PROBE_INTERVAL_MICROSECONDS);
}

template <typename Iterator>
int median(Iterator begin, Iterator end) {
    // use std::nth_element to grab the middle - for an even number of elements this is the upper middle
    Iterator middle = begin + (end - begin) / 2;
    std::nth_element(begin, middle, end);
    
    if ((end - begin) % 2 != 0) {
        // odd number of elements, just return the middle
        return *middle;
    } else {
        // even number of elements, return the mean of the upper middle and the lower middle
        Iterator lowerMiddle = std::max_element(begin, middle);
        return (*middle + *lowerMiddle) / 2;
    }
}

int32_t meanOfMedianFilteredValues(std::vector<int> intervals, int numValues, int valuesRequired = 0) {
    // grab the median value of the intervals vector
    int intervalsMedian = median(intervals.begin(), intervals.end());
    
    // figure out our bounds for median filtering
    static const int MEDIAN_FILTERING_BOUND_MULTIPLIER = 8;
    int upperBound = intervalsMedian * MEDIAN_FILTERING_BOUND_MULTIPLIER;
    int lowerBound = intervalsMedian / MEDIAN_FILTERING_BOUND_MULTIPLIER;
    
    int sum = 0;
    int count = 0;
    
    // sum the values that are inside the median filtered bounds
    for (auto& interval : intervals) {
        if ((interval < upperBound) && (interval > lowerBound)) {
            ++count;
            sum += interval;
        }
    }
    
    // make sure we hit our threshold of values required
    if (count >= valuesRequired) {
        // return the frequency (per second) for the mean interval
        static const double USECS_PER_SEC = 1000000.0;
        return (int32_t) ceil(USECS_PER_SEC / (((double) sum) / ((double) count)));
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
