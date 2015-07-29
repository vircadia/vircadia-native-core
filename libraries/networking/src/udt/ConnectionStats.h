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

namespace udt {

class ConnectionStats {
public:
    struct Stats {
        std::chrono::microseconds startTime;
        std::chrono::microseconds endTime;
        
        // Control Packet stat collection
        int sentACKs { 0 };
        int receivedACKs { 0 };
        int sentLightACKs { 0 };
        int receivedLightACKs { 0 };
        int sentACK2s { 0 };
        int receivedACK2s { 0 };
        int sentNAKs { 0 };
        int receivedNAKs { 0 };
        int sentTimeoutNAKs { 0 };
        int receivedTimeoutNAKs { 0 };
        
        int sentPackets { 0 };
        int recievedPackets { 0 };
    };
    
    ConnectionStats();
    
    Stats sample();
    Stats getTotalStats();
    
    void recordSentACK();
    void recordReceivedACK();
    void recordSentLightACK();
    void recordReceivedLightACK();
    void recordSentACK2();
    void recordReceivedACK2();
    void recordSentNAK();
    void recordReceivedNAK();
    void recordSentTimeoutNAK();
    void recordReceivedTimeoutNAK();
    
    void recordSentPackets();
    void recordReceivedPackets();
    
private:
    Stats _currentSample;
    Stats _total;
};
    
}

#endif // hifi_ConnectionStats_h