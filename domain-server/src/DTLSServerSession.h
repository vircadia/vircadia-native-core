//
//  DTLSServerSession.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DTLSServerSession_h
#define hifi_DTLSServerSession_h

#include <gnutls/dtls.h>

#include <DTLSSession.h>

class DTLSServerSession : public DTLSSession {
public:
    DTLSServerSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket);
};

#endif // hifi_DTLSServerSession_h
