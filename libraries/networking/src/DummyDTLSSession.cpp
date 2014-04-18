//
//  DummyDTLSSession.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-04-04.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DummyDTLSSession.h"

ssize_t DummyDTLSSession::socketPush(gnutls_transport_ptr_t ptr, const void* buffer, size_t size) {
    DummyDTLSSession* session = static_cast<DummyDTLSSession*>(ptr);
    QUdpSocket& dtlsSocket = session->_dtlsSocket;
    
#if DTLS_VERBOSE_DEBUG
    qDebug() << "Pushing a message of size" << size << "to" << session->_destinationSocket;
#endif
    
    return dtlsSocket.writeDatagram(reinterpret_cast<const char*>(buffer), size,
                                    session->_destinationSocket.getAddress(), session->_destinationSocket.getPort());
}

DummyDTLSSession::DummyDTLSSession(QUdpSocket& dtlsSocket, const HifiSockAddr& destinationSocket) :
    _dtlsSocket(dtlsSocket),
    _destinationSocket(destinationSocket)
{
    
}