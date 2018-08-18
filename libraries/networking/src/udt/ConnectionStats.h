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

namespace udt {

class ConnectionStats {
public:
    struct Stats {
        enum Event {
            SentACK,
            ReceivedACK,
            ProcessedACK,
            Retransmission,
            Duplicate,
            
            NumEvents
        };
        
        using microseconds = std::chrono::microseconds;
        using Events = std::array<int, NumEvents>;
        
        microseconds startTime;
        microseconds endTime;
        
        // construct a vector for the events of the size of our Enum - default value is zero
        Events events;
        
        // packet counts and sizes
        int sentPackets { 0 };
        int receivedPackets { 0 };
        int sentUtilBytes { 0 };
        int receivedUtilBytes { 0 };
        int sentBytes { 0 };
        int receivedBytes { 0 };
        
        int sentUnreliablePackets { 0 };
        int receivedUnreliablePackets { 0 };
        int sentUnreliableUtilBytes { 0 };
        int receivedUnreliableUtilBytes { 0 };
        int sentUnreliableBytes { 0 };
        int receivedUnreliableBytes { 0 };
       
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
    Stats getTotalStats();
    
    void record(Stats::Event event);
    
    void recordSentPackets(int payload, int total);
    void recordReceivedPackets(int payload, int total);
    
    void recordUnreliableSentPackets(int payload, int total);
    void recordUnreliableReceivedPackets(int payload, int total);
    
    void recordSendRate(int sample);
    void recordReceiveRate(int sample);
    void recordRTT(int sample);
    void recordCongestionWindowSize(int sample);
    void recordPacketSendPeriod(int sample);
    
private:
    Stats _currentSample;
    Stats _total;
};
    
}

class QDebug;
QDebug& operator<<(QDebug&& debug, const udt::ConnectionStats::Stats& stats);

#endif // hifi_ConnectionStats_h
