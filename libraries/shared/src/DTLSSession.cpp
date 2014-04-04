//
//  DTLSSession.cpp
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <gnutls/dtls.h>

#include "NodeList.h"
#include "DTLSSession.h"

#define DTLS_VERBOSE_DEBUG 0

int DTLSSession::socketPullTimeout(gnutls_transport_ptr_t ptr, unsigned int ms) {
    DTLSSession* session = static_cast<DTLSSession*>(ptr);
    QUdpSocket& dtlsSocket = session->_dtlsSocket;
    
    if (dtlsSocket.hasPendingDatagrams()) {
        // peek the data on stack to see if it belongs to this session
        static sockaddr senderSockAddr;
        static socklen_t sockAddrSize = sizeof(senderSockAddr);
        
        QByteArray peekDatagram(dtlsSocket.pendingDatagramSize(), 0);
        
        recvfrom(dtlsSocket.socketDescriptor(), peekDatagram.data(), dtlsSocket.pendingDatagramSize(),
                 MSG_PEEK, &senderSockAddr, &sockAddrSize);
        
        if (HifiSockAddr(&senderSockAddr) == session->_destinationSocket) {
            // there is data for this session ready to be read
            return 1;
        } else {
            // the next data from the dtlsSocket is not for this session
            return 0;
        }
    } else {
        // no data available on the dtlsSocket
        return 0;
    }
}

ssize_t DTLSSession::socketPull(gnutls_transport_ptr_t ptr, void* buffer, size_t size) {
    DTLSSession* session = static_cast<DTLSSession*>(ptr);
    QUdpSocket& dtlsSocket = session->_dtlsSocket;
    
    HifiSockAddr pulledSockAddr;
    qint64 bytesReceived = dtlsSocket.readDatagram(reinterpret_cast<char*>(buffer), size,
                                                   pulledSockAddr.getAddressPointer(), pulledSockAddr.getPortPointer());
    if (bytesReceived == -1) {
        // no data to pull, return -1
#if DTLS_VERBOSE_DEBUG
        qDebug() << "Received no data on call to readDatagram";
#endif
        return bytesReceived;
    }
    
    if (pulledSockAddr == session->_destinationSocket) {
        // bytes received from the correct sender, return number of bytes received

#if DTLS_VERBOSE_DEBUG
        qDebug() << "Received" << bytesReceived << "on DTLS socket from" << pulledSockAddr;
#endif
        
        return bytesReceived;
    }
    
    // we pulled a packet not matching this session, so output that
    qDebug() << "Denied connection from" << pulledSockAddr;
    
    gnutls_transport_set_errno(session->_gnutlsSession, GNUTLS_E_AGAIN);
    return -1;
}

ssize_t DTLSSession::socketPush(gnutls_transport_ptr_t ptr, const void* buffer, size_t size) {
    DTLSSession* session = static_cast<DTLSSession*>(ptr);
    QUdpSocket& dtlsSocket = session->_dtlsSocket;
    
#if DTLS_VERBOSE_DEBUG
    qDebug() << "Pushing a message of size" << size << "to" << session->_destinationSocket;
#endif
    
    return dtlsSocket.writeDatagram(reinterpret_cast<const char*>(buffer), size,
                                    session->_destinationSocket.getAddress(), session->_destinationSocket.getPort());
}

DTLSSession::DTLSSession(int end, QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    _dtlsSocket(dtlsSocket),
    _destinationSocket(destinationSocket),
    _completedHandshake(false)
{
    gnutls_init(&_gnutlsSession, end | GNUTLS_DATAGRAM | GNUTLS_NONBLOCK);
    
    // see http://gnutls.org/manual/html_node/Datagram-TLS-API.html#gnutls_005fdtls_005fset_005fmtu
    const unsigned int DTLS_MAX_MTU = 1452;
    gnutls_dtls_set_mtu(_gnutlsSession, DTLS_MAX_MTU);
    
    const unsigned int DTLS_TOTAL_CONNECTION_TIMEOUT = 10 * DOMAIN_SERVER_CHECK_IN_MSECS;
    gnutls_dtls_set_timeouts(_gnutlsSession, 1, DTLS_TOTAL_CONNECTION_TIMEOUT);
    
    gnutls_transport_set_ptr(_gnutlsSession, this);
    gnutls_transport_set_push_function(_gnutlsSession, socketPush);
    gnutls_transport_set_pull_function(_gnutlsSession, socketPull);
    gnutls_transport_set_pull_timeout_function(_gnutlsSession, socketPullTimeout);
}

qint64 DTLSSession::writeDatagram(const QByteArray& datagram) {
    // we don't need to put a hash in this packet, so just send it off
    return gnutls_record_send(_gnutlsSession, datagram.data(), datagram.size());
}