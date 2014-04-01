//
//  DTLSSession.cpp
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "DTLSSession.h"

static int socketPullTimeout(gnutls_transport_ptr_t ptr, unsigned int ms) {
    return 1;
}

static ssize_t socketPull(gnutls_transport_ptr_t ptr, void* buffer, size_t size) {
    DTLSSession* session = static_cast<DTLSSession*>(ptr);
    QUdpSocket& dtlsSocket = session->_dtlsSocket;
    
    if (dtlsSocket.hasPendingDatagrams()) {
        return dtlsSocket.read(reinterpret_cast<char*>(buffer), size);
    } else {
        gnutls_transport_set_errno(session->_gnutlsSession, GNUTLS_E_AGAIN);
        return 0;
    }
}

static ssize_t socketPush(gnutls_transport_ptr_t ptr, const void* buffer, size_t size) {
    DTLSSession* session = static_cast<DTLSSession*>(ptr);
    QUdpSocket& dtlsSocket = session->_dtlsSocket;
    
    if (dtlsSocket.state() != QAbstractSocket::ConnectedState) {
        gnutls_transport_set_errno(session->_gnutlsSession, GNUTLS_E_AGAIN);
        return -1;
    }
    
    return dtlsSocket.write(reinterpret_cast<const char*>(buffer), size);
}

DTLSSession::DTLSSession(QUdpSocket& dtlsSocket) :
    _dtlsSocket(dtlsSocket)
{
    
}