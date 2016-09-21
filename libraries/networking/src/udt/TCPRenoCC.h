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
    virtual void onTimeout();

protected:
    virtual void ackAction();
    virtual void duplicateACKAction();
    virtual void setInitialSendSequenceNumber(SequenceNumber seqNum);

    int _issThreshold;
    bool _slowStart;
    
    int _duplicateAckCount;
    SequenceNumber _lastACK;
};

}


#endif // hifi_TCPRenoCC_h
