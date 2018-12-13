//
//  ConnectionStats.h
//  libraries/networking/src/udt
//
//  Created by Clement on 7/29/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ConnectionStats_h
#define hifi_ConnectionStats_h

#include <chrono>
#include <array>
#include <stdint.h>

namespace udt {

class ConnectionStats {
public:
    struct Stats {
        enum Event {
            SentACK,
            ReceivedACK,
            ProcessedACK,
            
            NumEvents
        };
        
        using microseconds = std::chrono::microseconds;
        using Events = std::array<int, NumEvents>;
        
        microseconds startTime;
        microseconds endTime;
        
        // construct a vector for the events of the size of our Enum - default value is zero
        Events events;
        
        // packet counts and sizes
        uint32_t sentPackets { 0 };
        uint32_t receivedPackets { 0 };
        uint32_t retransmittedPackets { 0 };
        uint32_t duplicatePackets { 0 };

        uint64_t sentUtilBytes { 0 };
        uint64_t receivedUtilBytes { 0 };
        uint64_t retransmittedUtilBytes { 0 };
        uint64_t duplicateUtilBytes { 0 };

        uint64_t sentBytes { 0 };
        uint64_t receivedBytes { 0 };
        uint64_t retransmittedBytes { 0 };
        uint64_t duplicateBytes { 0 };
        
        uint32_t sentUnreliablePackets { 0 };
        uint32_t receivedUnreliablePackets { 0 };
        uint64_t sentUnreliableUtilBytes { 0 };
        uint64_t receivedUnreliableUtilBytes { 0 };
        uint64_t sentUnreliableBytes { 0 };
        uint64_t receivedUnreliableBytes { 0 };
       
        // the following stats are trailing averages in the result, not totals
        int sendRate { 0 };
        int receiveRate { 0 };
        int estimatedBandwith { 0 };
        int rtt { 0 };
        int congestionWindowSize { 0 };
        int packetSendPeriod { 0 };
        
        // TODO: Remove once Win build supports brace initialization: `Events events {{ 0 }};`
        Stats() { events.fill(0); }
    };
    
    ConnectionStats();
    
    Stats sample();
    
    void record(Stats::Event event);

    void recordSentACK(int size);
    void recordReceivedACK(int size);

    void recordSentPackets(int payload, int total);
    void recordReceivedPackets(int payload, int total);

    void recordRetransmittedPackets(int payload, int total);
    void recordDuplicatePackets(int payload, int total);
    
    void recordUnreliableSentPackets(int payload, int total);
    void recordUnreliableReceivedPackets(int payload, int total);

    void recordCongestionWindowSize(int sample);
    void recordPacketSendPeriod(int sample);
    
private:
    Stats _currentSample;
};
    
}

class QDebug;
QDebug& operator<<(QDebug&& debug, const udt::ConnectionStats::Stats& stats);

#endif // hifi_ConnectionStats_h
