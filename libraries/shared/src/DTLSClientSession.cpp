//
//  DTLSClientSession.cpp
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "DTLSClientSession.h"

gnutls_certificate_credentials_t* DTLSClientSession::x509CACredentials() {
    static gnutls_certificate_credentials_t x509Credentials;
    static bool credentialsInitialized = false;
    
    if (!credentialsInitialized) {
        gnutls_certificate_allocate_credentials(&x509Credentials);
    }
    
    return &x509Credentials;
}

DTLSClientSession::DTLSClientSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    DTLSSession(GNUTLS_CLIENT, dtlsSocket, destinationSocket)
{
    gnutls_priority_set_direct(_gnutlsSession, "PERFORMANCE", NULL);
    gnutls_credentials_set(_gnutlsSession, GNUTLS_CRD_CERTIFICATE, *x509CACredentials());
}