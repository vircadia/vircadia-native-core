//
//  DTLSClientSession.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainHandler.h"

#include "DTLSClientSession.h"

gnutls_certificate_credentials_t DTLSClientSession::_x509CACredentials;

void DTLSClientSession::globalInit() {
    static bool initialized = false;
    
    if (!initialized) {
        gnutls_global_init();
        gnutls_certificate_allocate_credentials(&_x509CACredentials);
        
        gnutls_certificate_set_x509_trust_mem(_x509CACredentials, DTLSSession::highFidelityCADatum(), GNUTLS_X509_FMT_PEM);
        gnutls_certificate_set_verify_function(_x509CACredentials, DTLSClientSession::verifyServerCertificate);
    }
}

void DTLSClientSession::globalDeinit() {
    gnutls_certificate_free_credentials(_x509CACredentials);
    
    gnutls_global_deinit();
}

int DTLSClientSession::verifyServerCertificate(gnutls_session_t session) {
    unsigned int verifyStatus = 0;
    
    // grab the hostname from the domain handler that this session is associated with
    DomainHandler* domainHandler = reinterpret_cast<DomainHandler*>(gnutls_session_get_ptr(session));
    qDebug() << "Checking for" << domainHandler->getHostname() << "from cert.";
    
    int certReturn = gnutls_certificate_verify_peers3(session,
                                                      domainHandler->getHostname().toLocal8Bit().constData(),
                                                      &verifyStatus);
    
    if (certReturn < 0) {
        return GNUTLS_E_CERTIFICATE_ERROR;
    }
    
    gnutls_certificate_type_t typeReturn = gnutls_certificate_type_get(session);
    
    gnutls_datum_t printOut;

    
    certReturn = gnutls_certificate_verification_status_print(verifyStatus, typeReturn, &printOut, 0);
    
    if (certReturn < 0) {
        return GNUTLS_E_CERTIFICATE_ERROR;
    }
    
    qDebug() << "Gnutls certificate verification status:" << reinterpret_cast<char *>(printOut.data);
    gnutls_free(printOut.data);
    
    if (verifyStatus != 0) {
        qDebug() << "Server provided certificate for DTLS is not trusted. Can not complete handshake.";
        return GNUTLS_E_CERTIFICATE_ERROR;
    } else {
        // certificate is valid, continue handshaking as normal
        return 0;
    }
}

DTLSClientSession::DTLSClientSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    DTLSSession(GNUTLS_CLIENT, dtlsSocket, destinationSocket)
{
    gnutls_priority_set_direct(_gnutlsSession, "PERFORMANCE", NULL);
    gnutls_credentials_set(_gnutlsSession, GNUTLS_CRD_CERTIFICATE, _x509CACredentials);
}