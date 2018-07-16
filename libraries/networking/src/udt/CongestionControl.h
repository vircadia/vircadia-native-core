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

    CongestionControl() {};
    CongestionControl(int synInterval) : _synInterval(synInterval) {}
    virtual ~CongestionControl() {}
    
    int synInterval() const { return _synInterval; }
    void setMaxBandwidth(int maxBandwidth);

    virtual void init() {}

    // return value specifies if connection should perform a fast re-transmit of ACK + 1 (used in TCP style congestion control)
    virtual bool onACK(SequenceNumber ackNum, p_high_resolution_clock::time_point receiveTime) { return false; }

    virtual void onTimeout() {}

    virtual void onPacketSent(int wireSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) {}
protected:
    void setMSS(int mss) { _mss = mss; }
    void setMaxCongestionWindowSize(int window) { _maxCongestionWindowSize = window; }
    virtual void setInitialSendSequenceNumber(SequenceNumber seqNum) = 0;
    void setSendCurrentSequenceNumber(SequenceNumber seqNum) { _sendCurrSeqNum = seqNum; }
    void setReceiveRate(int rate) { _receiveRate = rate; }
    void setRTT(int rtt) { _rtt = rtt; }
    void setPacketSendPeriod(double newSendPeriod); // call this internally to ensure send period doesn't go past max bandwidth
    
    double _packetSendPeriod { 1.0 }; // Packet sending period, in microseconds
    int _congestionWindowSize { 16 }; // Congestion window size, in packets

    std::atomic<int> _maxBandwidth { -1 }; // Maximum desired bandwidth, bits per second
    int _maxCongestionWindowSize { 0 }; // maximum cwnd size, in packets
    
    int _mss { 0 }; // Maximum Packet Size, including all packet headers
    SequenceNumber _sendCurrSeqNum; // current maximum seq num sent out
    int _receiveRate { 0 }; // packet arrive rate at receiver side, packets per second
    int _rtt { 0 }; // current estimated RTT, microsecond
    
private:
    CongestionControl(const CongestionControl& other) = delete;
    CongestionControl& operator=(const CongestionControl& other) = delete;
    
    int _synInterval { DEFAULT_SYN_INTERVAL };
};
    
    
class CongestionControlVirtualFactory {
public:
    virtual ~CongestionControlVirtualFactory() {}
    
    static int synInterval() { return DEFAULT_SYN_INTERVAL; }
    
    virtual std::unique_ptr<CongestionControl> create() = 0;
};

template <class T> class CongestionControlFactory: public CongestionControlVirtualFactory {
public:
    virtual ~CongestionControlFactory() {}
    virtual std::unique_ptr<CongestionControl> create() override { return std::unique_ptr<T>(new T()); }
};
    
}

#endif // hifi_CongestionControl_h
