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
        qDebug() << "processed" << gnutls_certificate_set_x509_trust_file(x509Credentials, "/Users/birarda/code/hifi/certs/root-ca.crt", GNUTLS_X509_FMT_PEM);
    }
    
    return &x509Credentials;
}

int verifyX509Certificate(gnutls_session_t session) {
    int certificateType = gnutls_certificate_type_get(session);
    qDebug() << "Verifying a certificate of type" << certificateType;
    
    return 0;
}

DTLSClientSession::DTLSClientSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    DTLSSession(GNUTLS_CLIENT, dtlsSocket, destinationSocket)
{
    gnutls_priority_set_direct(_gnutlsSession, "PERFORMANCE", NULL);
    gnutls_credentials_set(_gnutlsSession, GNUTLS_CRD_CERTIFICATE, x509CACredentials());
}