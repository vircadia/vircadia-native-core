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

using namespace udt;
using namespace std::chrono;

ConnectionStats::ConnectionStats() {
    auto now = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
    _currentSample.startTime = now;
    _total.startTime = now;
}

ConnectionStats::Stats ConnectionStats::sample() {
    Stats sample = _currentSample;
    _currentSample = Stats();
    
    auto now = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
    sample.endTime = now;
    _currentSample.startTime = now;
    
    return sample;
}

void ConnectionStats::recordSentACK() {
    ++_currentSample.sentACKs;
    ++_total.sentACKs;
}

void ConnectionStats::recordReceivedACK() {
    ++_currentSample.receivedACKs;
    ++_total.receivedACKs;
}

void ConnectionStats::recordSentLightACK() {
    ++_currentSample.sentLightACKs;
    ++_total.sentLightACKs;
}

void ConnectionStats::recordReceivedLightACK() {
    ++_currentSample.receivedLightACKs;
    ++_total.receivedLightACKs;
}

void ConnectionStats::recordSentACK2() {
    ++_currentSample.sentACK2s;
    ++_total.sentACK2s;
}

void ConnectionStats::recordReceivedACK2() {
    ++_currentSample.receivedACK2s;
    ++_total.receivedACK2s;
}

void ConnectionStats::recordSentNAK() {
    ++_currentSample.sentNAKs;
    ++_total.sentNAKs;
}

void ConnectionStats::recordReceivedNAK() {
    ++_currentSample.receivedNAKs;
    ++_total.receivedNAKs;
}

void ConnectionStats::recordSentTimeoutNAK() {
    ++_currentSample.sentTimeoutNAKs;
    ++_total.sentTimeoutNAKs;
}

void ConnectionStats::recordReceivedTimeoutNAK() {
    ++_currentSample.receivedTimeoutNAKs;
    ++_total.receivedTimeoutNAKs;
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
    ++_currentSample.recievedPackets;
    ++_total.recievedPackets;
    
    _currentSample.recievedUtilBytes += payload;
    _total.recievedUtilBytes += payload;
    
    _currentSample.sentBytes += total;
    _total.recievedBytes += total;
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
    ++_currentSample.recievedUnreliablePackets;
    ++_total.recievedUnreliablePackets;
    
    _currentSample.recievedUnreliableUtilBytes += payload;
    _total.recievedUnreliableUtilBytes += payload;
    
    _currentSample.sentUnreliableBytes += total;
    _total.recievedUnreliableBytes += total;
}

void ConnectionStats::recordRetransmition() {
    ++_currentSample.retransmissions;
    ++_total.retransmissions;
}

void ConnectionStats::recordDrop() {
    ++_currentSample.drops;
    ++_total.drops;
}
