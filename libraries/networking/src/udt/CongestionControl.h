//
//  CongestionControl.h
//  libraries/networking/src/udt
//
//  Created by Clement on 7/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CongestionControl_h
#define hifi_CongestionControl_h

#include <atomic>
#include <memory>
#include <vector>

#include <PortableHighResolutionClock.h>

#include "LossList.h"
#include "SequenceNumber.h"

namespace udt {
    
static const int32_t DEFAULT_SYN_INTERVAL = 10000; // 10 ms

class Connection;
class Packet;

class CongestionControl {
    friend class Connection;
public:

    CongestionControl() = default;
    virtual ~CongestionControl() = default;

    void setMaxBandwidth(int maxBandwidth);

    virtual void init() {}

    // return value specifies if connection should perform a fast re-transmit of ACK + 1 (used in TCP style congestion control)
    virtual bool onACK(SequenceNumber ackNum, p_high_resolution_clock::time_point receiveTime) { return false; }

    virtual void onTimeout() {}

    virtual void onPacketSent(int wireSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) {}
    virtual void onPacketReSent(int wireSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) {}

    virtual int estimatedTimeout() const = 0;

protected:
    void setMSS(int mss) { _mss = mss; }
    virtual void setInitialSendSequenceNumber(SequenceNumber seqNum) = 0;
    void setSendCurrentSequenceNumber(SequenceNumber seqNum) { _sendCurrSeqNum = seqNum; }
    void setPacketSendPeriod(double newSendPeriod); // call this internally to ensure send period doesn't go past max bandwidth
    
    double _packetSendPeriod { 1.0 }; // Packet sending period, in microseconds
    int _congestionWindowSize { 16 }; // Congestion window size, in packets

    std::atomic<int> _maxBandwidth { -1 }; // Maximum desired bandwidth, bits per second
    
    int _mss { 0 }; // Maximum Packet Size, including all packet headers
    SequenceNumber _sendCurrSeqNum; // current maximum seq num sent out
    
private:
    Q_DISABLE_COPY(CongestionControl);
};
    
    
class CongestionControlVirtualFactory {
public:
    virtual ~CongestionControlVirtualFactory() {}
    
    virtual std::unique_ptr<CongestionControl> create() = 0;
};

template <class T> class CongestionControlFactory: public CongestionControlVirtualFactory {
public:
    virtual ~CongestionControlFactory() {}
    virtual std::unique_ptr<CongestionControl> create() override { return std::unique_ptr<T>(new T()); }
};
    
}

#endif // hifi_CongestionControl_h
