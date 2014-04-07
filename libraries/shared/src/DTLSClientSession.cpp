//
//  DTLSClientSession.cpp
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "DTLSClientSession.h"

gnutls_certificate_credentials_t DTLSClientSession::_x509CACredentials;

void DTLSClientSession::globalInit() {
    static bool initialized = false;
    
    if (!initialized) {
        gnutls_global_init();
        gnutls_certificate_allocate_credentials(&_x509CACredentials);
    }
}

void DTLSClientSession::globalDeinit() {
    gnutls_certificate_free_credentials(_x509CACredentials);
    
    gnutls_global_deinit();
}

DTLSClientSession::DTLSClientSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    DTLSSession(GNUTLS_CLIENT, dtlsSocket, destinationSocket)
{
    gnutls_priority_set_direct(_gnutlsSession, "PERFORMANCE", NULL);
    gnutls_credentials_set(_gnutlsSession, GNUTLS_CRD_CERTIFICATE, _x509CACredentials);
}