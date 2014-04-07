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
        int certsProcessed = gnutls_certificate_set_x509_trust_mem(_x509CACredentials, DTLSSession::highFidelityCADatum(), GNUTLS_X509_FMT_PEM);
        qDebug() << "There were" << certsProcessed;
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