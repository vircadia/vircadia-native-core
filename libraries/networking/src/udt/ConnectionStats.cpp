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
    _total.startTime = now;
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
    ++_total.events[(int) event];
}

void ConnectionStats::recordSentPackets(int payload, int total) {
    ++_currentSample.sentPackets;
    ++_total.sentPackets;
    
    _currentSample.sentUtilBytes += payload;
    _total.sentUtilBytes += payload;
    
    _currentSample.sentBytes += total;
    _total.sentBytes += total;
}

void ConnectionStats::recordReceivedPackets(int payload, int total) {
    ++_currentSample.receivedPackets;
    ++_total.receivedPackets;
    
    _currentSample.receivedUtilBytes += payload;
    _total.receivedUtilBytes += payload;
    
    _currentSample.receivedBytes += total;
    _total.receivedBytes += total;
}

void ConnectionStats::recordUnreliableSentPackets(int payload, int total) {
    ++_currentSample.sentUnreliablePackets;
    ++_total.sentUnreliablePackets;
    
    _currentSample.sentUnreliableUtilBytes += payload;
    _total.sentUnreliableUtilBytes += payload;
    
    _currentSample.sentUnreliableBytes += total;
    _total.sentUnreliableBytes += total;
}

void ConnectionStats::recordUnreliableReceivedPackets(int payload, int total) {
    ++_currentSample.receivedUnreliablePackets;
    ++_total.receivedUnreliablePackets;
    
    _currentSample.receivedUnreliableUtilBytes += payload;
    _total.receivedUnreliableUtilBytes += payload;
    
    _currentSample.sentUnreliableBytes += total;
    _total.receivedUnreliableBytes += total;
}

static const double EWMA_CURRENT_SAMPLE_WEIGHT = 0.125;
static const double EWMA_PREVIOUS_SAMPLES_WEIGHT = 1.0 - EWMA_CURRENT_SAMPLE_WEIGHT;

void ConnectionStats::recordSendRate(int sample) {
    _currentSample.sendRate = sample;
    _total.sendRate = (int)((_total.sendRate * EWMA_PREVIOUS_SAMPLES_WEIGHT) + (sample * EWMA_CURRENT_SAMPLE_WEIGHT));
}

void ConnectionStats::recordReceiveRate(int sample) {
    _currentSample.receiveRate = sample;
    _total.receiveRate = (int)((_total.receiveRate * EWMA_PREVIOUS_SAMPLES_WEIGHT) + (sample * EWMA_CURRENT_SAMPLE_WEIGHT));
}

void ConnectionStats::recordRTT(int sample) {
    _currentSample.rtt = sample;
    _total.rtt = (int)((_total.rtt * EWMA_PREVIOUS_SAMPLES_WEIGHT) + (sample * EWMA_CURRENT_SAMPLE_WEIGHT));
}

void ConnectionStats::recordCongestionWindowSize(int sample) {
    _currentSample.congestionWindowSize = sample;
    _total.congestionWindowSize = (int)((_total.congestionWindowSize * EWMA_PREVIOUS_SAMPLES_WEIGHT) + (sample * EWMA_CURRENT_SAMPLE_WEIGHT));
}

void ConnectionStats::recordPacketSendPeriod(int sample) {
    _currentSample.packetSendPeriod = sample;
    _total.packetSendPeriod = (int)((_total.packetSendPeriod * EWMA_PREVIOUS_SAMPLES_WEIGHT) + (sample * EWMA_CURRENT_SAMPLE_WEIGHT));
}

QDebug& operator<<(QDebug&& debug, const udt::ConnectionStats::Stats& stats) {
    debug << "Connection stats:\n";
#define HIFI_LOG_EVENT(x) << "    " #x " events: " << stats.events[ConnectionStats::Stats::Event::x] << "\n"
    debug
    HIFI_LOG_EVENT(SentACK)
    HIFI_LOG_EVENT(ReceivedACK)
    HIFI_LOG_EVENT(ProcessedACK)
    HIFI_LOG_EVENT(Retransmission)
    HIFI_LOG_EVENT(Duplicate)
    ;
#undef HIFI_LOG_EVENT

    debug << "    Sent packets: " << stats.sentPackets;
    debug << "\n     Received packets: " << stats.receivedPackets;
    debug << "\n     Sent util bytes: " << stats.sentUtilBytes;
    debug << "\n     Sent bytes: " << stats.sentBytes;
    debug << "\n     Received bytes: " << stats.receivedBytes << "\n";
    return debug;
}
