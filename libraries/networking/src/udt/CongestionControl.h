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

#include <vector>

#include "SeqNum.h"

namespace udt {

class Packet;

class CongestionControl {
public:
    static const int32_t DEFAULT_SYN_INTERVAL = 10000; // 10 ms

    CongestionControl() {}
    virtual ~CongestionControl() {}

    virtual void init() {}
    virtual void close() {}
    virtual void onAck(SeqNum ackNum) {}
    virtual void onLoss(const std::vector<SeqNum>& lossList) {}
    virtual void onPacketSent(const Packet& packet) {}
    virtual void onPacketReceived(const Packet& packet) {}
    
protected:
    void setAckTimer(int syn) { _ackPeriod = (syn > _synInterval) ? _synInterval : syn; }
    void setAckInterval(int interval) { _ackInterval = interval; }
    void setRto(int rto) { _userDefinedRto = true; _rto = rto; }
    
    int32_t _synInterval = DEFAULT_SYN_INTERVAL; // UDT constant parameter, SYN
    
    double _packetSendPeriod = 1.0; // Packet sending period, in microseconds
    double _congestionWindowSize = 16.0; // Congestion window size, in packets
    
    int _bandwidth = 0; // estimated bandwidth, packets per second
    double _maxCongestionWindowSize = 0.0; // maximum cwnd size, in packets
    
    int _mss = 0; // Maximum Packet Size, including all packet headers
    SeqNum _sendCurrSeqNum; // current maximum seq num sent out
    int _recvieveRate = 0; // packet arrive rate at receiver side, packets per second
    int _rtt = 0; // current estimated RTT, microsecond
    
private:
    CongestionControl(const CongestionControl& other) = delete;
    CongestionControl& operator=(const CongestionControl& other) = delete;
    
    void setMss(int mss) { _mss = mss; }
    void setMaxCongestionWindowSize(int window) { _maxCongestionWindowSize = window; }
    void setBandwidth(int bandwidth) { _bandwidth = bandwidth; }
    void setSndCurrSeqNum(SeqNum seqNum) { _sendCurrSeqNum = seqNum; }
    void setRcvRate(int rate) { _recvieveRate = rate; }
    void setRtt(int rtt) { _rtt = rtt; }
    
    int _ackPeriod = 0; // Periodical timer to send an ACK, in milliseconds
    int _ackInterval = 0; // How many packets to send one ACK, in packets
    
    bool _userDefinedRto = false; // if the RTO value is defined by users
    int _rto = -1; // RTO value, microseconds
};
    
    
class CongestionControlVirtualFactory {
public:
    virtual ~CongestionControlVirtualFactory() {}
    
    virtual std::unique_ptr<CongestionControl> create() = 0;
};

template <class T> class CongestionControlFactory: public CongestionControlVirtualFactory
{
public:
    virtual ~CongestionControlFactory() {}
    
    virtual std::unique_ptr<CongestionControl> create() { return std::unique_ptr<T>(new T()); }
};

class UdtCC: public CongestionControl {
public:
    UdtCC() {}
    
public:
    virtual void init();
    virtual void onACK(SeqNum ackNum);
    virtual void onLoss(const std::vector<SeqNum>& lossList);
    virtual void onTimeout();
    
private:
    int _rcInterval = 0;	// UDT Rate control interval
    uint64_t _lastRCTime = 0; // last rate increase time
    bool _slowStart = true;	// if in slow start phase
    SeqNum _lastAck; // last ACKed seq num
    bool _loss = false;	// if loss happened since last rate increase
    SeqNum _lastDecSeq; // max pkt seq num sent out when last decrease happened
    double _lastDecPeriod = 1; // value of pktsndperiod when last decrease happened
    int _nakCount = 0; // NAK counter
    int _decRandom = 1; // random threshold on decrease by number of loss events
    int _avgNAKNum = 0; // average number of NAKs per congestion
    int _decCount = 0; // number of decreases in a congestion epoch
};
    
}

#endif // hifi_CongestionControl_h