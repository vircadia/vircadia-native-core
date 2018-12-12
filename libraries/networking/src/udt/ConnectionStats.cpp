//
//  ConnectionStats.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 7/29/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ConnectionStats.h"

#include <QtCore/QDebug>

using namespace udt;
using namespace std::chrono;

ConnectionStats::ConnectionStats() {
    auto now = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    _currentSample.startTime = now;
}

ConnectionStats::Stats ConnectionStats::sample() {
    Stats sample = _currentSample;
    _currentSample = Stats();
    
    auto now = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    sample.endTime = now;
    _currentSample.startTime = now;
    
    return sample;
}

void ConnectionStats::record(Stats::Event event) {
    ++_currentSample.events[(int) event];
}

void ConnectionStats::recordSentPackets(int payload, int total) {
    ++_currentSample.sentPackets;
    _currentSample.sentUtilBytes += payload;
    _currentSample.sentBytes += total;
}

void ConnectionStats::recordReceivedPackets(int payload, int total) {
    ++_currentSample.receivedPackets;
    _currentSample.receivedUtilBytes += payload;
    _currentSample.receivedBytes += total;
}

void ConnectionStats::recordRetransmittedPackets(int payload, int total) {
    ++_currentSample.retransmittedPackets;
    _currentSample.retransmittedUtilBytes += payload;
    _currentSample.retransmittedBytes += total;
}

void ConnectionStats::recordDuplicatePackets(int payload, int total) {
    ++_currentSample.duplicatePackets;
    _currentSample.duplicateUtilBytes += payload;
    _currentSample.duplicateBytes += total;
}

void ConnectionStats::recordUnreliableSentPackets(int payload, int total) {
    ++_currentSample.sentUnreliablePackets;
    _currentSample.sentUnreliableUtilBytes += payload;
    _currentSample.sentUnreliableBytes += total;
}

void ConnectionStats::recordUnreliableReceivedPackets(int payload, int total) {
    ++_currentSample.receivedUnreliablePackets;
    _currentSample.receivedUnreliableUtilBytes += payload;
    _currentSample.receivedUnreliableBytes += total;
}

void ConnectionStats::recordCongestionWindowSize(int sample) {
    _currentSample.congestionWindowSize = sample;
}

void ConnectionStats::recordPacketSendPeriod(int sample) {
    _currentSample.packetSendPeriod = sample;
}

QDebug& operator<<(QDebug&& debug, const udt::ConnectionStats::Stats& stats) {
    debug << "Connection stats:\n";
#define HIFI_LOG_EVENT(x) << "    " #x " events: " << stats.events[ConnectionStats::Stats::Event::x] << "\n"
    debug
    HIFI_LOG_EVENT(SentACK)
    HIFI_LOG_EVENT(ReceivedACK)
    HIFI_LOG_EVENT(ProcessedACK)
    ;
#undef HIFI_LOG_EVENT

    debug << "    Sent packets: " << stats.sentPackets;
    debug << "\n    Retransmitted packets: " << stats.retransmittedPackets;
    debug << "\n     Received packets: " << stats.receivedPackets;
    debug << "\n     Duplicate packets: " << stats.duplicatePackets;
    debug << "\n     Sent util bytes: " << stats.sentUtilBytes;
    debug << "\n     Sent bytes: " << stats.sentBytes;
    debug << "\n     Received bytes: " << stats.receivedBytes << "\n";
    return debug;
}
