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

gnutls_certificate_credentials_t* DTLSSession::x509CACredentials() {
    static gnutls_certificate_credentials_t x509Credentials;
    static bool credentialsInitialized = false;
    
    if (!credentialsInitialized) {
        gnutls_certificate_allocate_credentials(&x509Credentials);
    }
    
    return &x509Credentials;
}

DTLSSession::DTLSSession(int end, QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    _dtlsSocket(dtlsSocket),
    _destinationSocket(destinationSocket)
{
    gnutls_init(&_gnutlsSession, end | GNUTLS_DATAGRAM);
}