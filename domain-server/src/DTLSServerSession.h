//
//  DTLSServerSession.h
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__DTLSServerSession__
#define __hifi__DTLSServerSession__

#include <DTLSSession.h>

class DTLSServerSession : public DTLSSession {
public:
    DTLSServerSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket);
};

#endif /* defined(__hifi__DTLSServerSession__) */
