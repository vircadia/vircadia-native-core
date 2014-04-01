//
//  DTLSClientSession.h
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__DTLSClientSession__
#define __hifi__DTLSClientSession__

#include "DTLSSession.h"

class DTLSClientSession : public DTLSSession {
public:
    DTLSClientSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket);
};

#endif /* defined(__hifi__DTLSClientSession__) */
