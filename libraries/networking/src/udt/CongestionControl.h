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
    virtual void onACK(SequenceNumber ackNum) {}
    virtual void onLoss(SequenceNumber rangeStart, SequenceNumber rangeEnd) {}
    virtual void onTimeout() {}
    virtual void onPacketSent(int packetSize, SequenceNumber seqNum) {}

    virtual bool shouldNAK() { return true; }
protected:
    void setAckInterval(int ackInterval) { _ackInterval = ackInterval; }
    void setRTO(int rto) { _userDefinedRTO = true; _rto = rto; }
    
    void setMSS(int mss) { _mss = mss; }
    void setMaxCongestionWindowSize(int window) { _maxCongestionWindowSize = window; }
    void setBandwidth(int bandwidth) { _bandwidth = bandwidth; }
    virtual void setInitialSendSequenceNumber(SequenceNumber seqNum) = 0;
    void setSendCurrentSequenceNumber(SequenceNumber seqNum) { _sendCurrSeqNum = seqNum; }
    void setReceiveRate(int rate) { _receiveRate = rate; }
    void setRTT(int rtt) { _rtt = rtt; }
    void setPacketSendPeriod(double newSendPeriod); // call this internally to ensure send period doesn't go past max bandwidth
    
    double _packetSendPeriod { 1.0 }; // Packet sending period, in microseconds
    int _congestionWindowSize { 16 }; // Congestion window size, in packets
    
    int _bandwidth { 0 }; // estimated bandwidth, packets per second
    std::atomic<int> _maxBandwidth { -1 }; // Maximum desired bandwidth, bits per second
    int _maxCongestionWindowSize { 0 }; // maximum cwnd size, in packets
    
    int _mss { 0 }; // Maximum Packet Size, including all packet headers
    SequenceNumber _sendCurrSeqNum; // current maximum seq num sent out
    int _receiveRate { 0 }; // packet arrive rate at receiver side, packets per second
    int _rtt { 0 }; // current estimated RTT, microsecond
    
private:
    CongestionControl(const CongestionControl& other) = delete;
    CongestionControl& operator=(const CongestionControl& other) = delete;
    
    int _ackInterval { 0 }; // How many packets to send one ACK, in packets
    int _lightACKInterval { 64 }; // How many packets to send one light ACK, in packets
    
    int _synInterval { DEFAULT_SYN_INTERVAL };
    
    bool _userDefinedRTO { false }; // if the RTO value is defined by users
    int _rto { -1 }; // RTO value, microseconds
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

class DefaultCC: public CongestionControl {
public:
    DefaultCC();
    
public:
    virtual void onACK(SequenceNumber ackNum) override;
    virtual void onLoss(SequenceNumber rangeStart, SequenceNumber rangeEnd) override;
    virtual void onTimeout() override;

protected:
    virtual void setInitialSendSequenceNumber(SequenceNumber seqNum) override;

private:
    void stopSlowStart(); // stops the slow start on loss or timeout
    
    p_high_resolution_clock::time_point _lastRCTime = p_high_resolution_clock::now(); // last rate increase time
    
    bool _slowStart { true };	// if in slow start phase
    SequenceNumber _lastACK; // last ACKed sequence number from previous
    bool _loss { false };	// if loss happened since last rate increase
    SequenceNumber _lastDecreaseMaxSeq; // max pkt seq num sent out when last decrease happened
    double _lastDecreasePeriod { 1 }; // value of _packetSendPeriod when last decrease happened
    int _nakCount { 0 }; // number of NAKs in congestion epoch
    int _randomDecreaseThreshold { 1 }; // random threshold on decrease by number of loss events
    int _avgNAKNum { 0 }; // average number of NAKs per congestion
    int _decreaseCount { 0 }; // number of decreases in a congestion epoch
    bool _delayedDecrease { false };
};
    
}

#endif // hifi_CongestionControl_h
