//
//  DTLSSession.cpp
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <gnutls/dtls.h>

#include "DTLSSession.h"

int DTLSSession::socketPullTimeout(gnutls_transport_ptr_t ptr, unsigned int ms) {
    return 1;
}

ssize_t DTLSSession::socketPull(gnutls_transport_ptr_t ptr, void* buffer, size_t size) {
    DTLSSession* session = static_cast<DTLSSession*>(ptr);
    QUdpSocket& dtlsSocket = session->_dtlsSocket;
    
    if (dtlsSocket.hasPendingDatagrams()) {
        return dtlsSocket.read(reinterpret_cast<char*>(buffer), size);
    } else {
        gnutls_transport_set_errno(session->_gnutlsSession, GNUTLS_E_AGAIN);
        return 0;
    }
}

ssize_t DTLSSession::socketPush(gnutls_transport_ptr_t ptr, const void* buffer, size_t size) {
    DTLSSession* session = static_cast<DTLSSession*>(ptr);
    QUdpSocket& dtlsSocket = session->_dtlsSocket;
    
    return dtlsSocket.writeDatagram(reinterpret_cast<const char*>(buffer), size,
                                    session->_destinationSocket.getAddress(), session->_destinationSocket.getPort());
}

static gnutls_certificate_credentials_t* x509ClientCredentials() {
    static gnutls_certificate_credentials_t x509Credentials;
    static bool credentialsInitialized = false;
    
    if (!credentialsInitialized) {
        gnutls_certificate_allocate_credentials(&x509Credentials);
    }
    
    return &x509Credentials;
}

DTLSSession::DTLSSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    _dtlsSocket(dtlsSocket),
    _destinationSocket(destinationSocket)
{
    qDebug() << "Initializing DTLS Session.";
    
    gnutls_init(&_gnutlsSession, GNUTLS_CLIENT | GNUTLS_DATAGRAM);
    gnutls_priority_set_direct(_gnutlsSession, "NORMAL", NULL);
    
    gnutls_credentials_set(_gnutlsSession, GNUTLS_CRD_CERTIFICATE, x509ClientCredentials());
    
    // tell GnuTLS to call us for push or pull
    gnutls_transport_set_ptr(_gnutlsSession, this);
    gnutls_transport_set_push_function(_gnutlsSession, socketPush);
    gnutls_transport_set_pull_function(_gnutlsSession, socketPull);
    gnutls_transport_set_pull_timeout_function(_gnutlsSession, socketPullTimeout);
    
    // start the handshake process with domain-server now
    gnutls_handshake(_gnutlsSession);
}