//
//  DTLSClientSession.cpp
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "DTLSClientSession.h"

DTLSClientSession::DTLSClientSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    DTLSSession(GNUTLS_CLIENT, dtlsSocket, destinationSocket)
{
    // _x509 as a member variable and not global/static assumes a single DTLSClientSession per client
    gnutls_certificate_allocate_credentials(&_x509Credentials);
    
    gnutls_priority_set_direct(_gnutlsSession, "PERFORMANCE", NULL);
    gnutls_credentials_set(_gnutlsSession, GNUTLS_CRD_CERTIFICATE, _x509Credentials);
}

DTLSClientSession::~DTLSClientSession() {
    gnutls_certificate_free_credentials(_x509Credentials);
}

