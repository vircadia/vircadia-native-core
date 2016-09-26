//
//  TCPRenoCC.h
//  libraries/networking/src/udt
//
//  Created by Clement on 9/20/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TCPRenoCC_h
#define hifi_TCPRenoCC_h

#include "CongestionControl.h"

namespace udt {

class TCPRenoCC : public CongestionControl {
public:
    virtual void init();
    virtual void onACK(SequenceNumber ackNum);

protected:
    virtual void setInitialSendSequenceNumber(SequenceNumber seqNum) { _lastACK = seqNum - 1; }

    bool isInSlowStart();
    int performSlowStart(int numAcked);
    int slowStartThreshold();

    bool isCongestionWindowLimited();
    void performCongestionAvoidanceAI(int sendCongestionWindowSize, int numAcked);

    virtual void performCongestionAvoidance(SequenceNumber ack, int numAcked);


    int _sendSlowStartThreshold;
    int _sendCongestionWindowSize;
    int _sendCongestionWindowCount;
    int _sendCongestionWindowClamp = ~0;
    int _maxPacketsOut;
    bool _isCongestionWindowLimited;

    SequenceNumber _lastACK;
};

}


#endif // hifi_TCPRenoCC_h
