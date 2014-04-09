//
//  DTLSClientSession.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DTLSClientSession_h
#define hifi_DTLSClientSession_h

#include "DTLSSession.h"

class DTLSClientSession : public DTLSSession {
public:
    DTLSClientSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket);
    
    static void globalInit();
    static void globalDeinit();
    
    static int verifyServerCertificate(gnutls_session_t session);
    
    static gnutls_certificate_credentials_t _x509CACredentials;
    static bool _wasGloballyInitialized;
};

#endif // hifi_DTLSClientSession_h
