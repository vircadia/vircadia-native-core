//
//  DTLSSession.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <gnutls/dtls.h>

#include "NodeList.h"
#include "DTLSSession.h"

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

gnutls_datum_t* DTLSSession::highFidelityCADatum() {
    static gnutls_datum_t hifiCADatum;
    static bool datumInitialized = false;
    
    static unsigned char HIGHFIDELITY_ROOT_CA_CERT[] =
        "-----BEGIN CERTIFICATE-----"
        "MIID6TCCA1KgAwIBAgIJANlfRkRD9A8bMA0GCSqGSIb3DQEBBQUAMIGqMQswCQYD\n"
        "VQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEWMBQGA1UEBxMNU2FuIEZyYW5j\n"
        "aXNjbzEbMBkGA1UEChMSSGlnaCBGaWRlbGl0eSwgSW5jMRMwEQYDVQQLEwpPcGVy\n"
        "YXRpb25zMRgwFgYDVQQDEw9oaWdoZmlkZWxpdHkuaW8xIjAgBgkqhkiG9w0BCQEW\n"
        "E29wc0BoaWdoZmlkZWxpdHkuaW8wHhcNMTQwMzI4MjIzMzM1WhcNMjQwMzI1MjIz\n"
        "MzM1WjCBqjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlmb3JuaWExFjAUBgNV\n"
        "BAcTDVNhbiBGcmFuY2lzY28xGzAZBgNVBAoTEkhpZ2ggRmlkZWxpdHksIEluYzET\n"
        "MBEGA1UECxMKT3BlcmF0aW9uczEYMBYGA1UEAxMPaGlnaGZpZGVsaXR5LmlvMSIw\n"
        "IAYJKoZIhvcNAQkBFhNvcHNAaGlnaGZpZGVsaXR5LmlvMIGfMA0GCSqGSIb3DQEB\n"
        "AQUAA4GNADCBiQKBgQDyo1euYiPPEdnvDZnIjWrrP230qUKMSj8SWoIkbTJF2hE8\n"
        "2eP3YOgbgSGBzZ8EJBxIOuNmj9g9Eg6691hIKFqy5W0BXO38P04Gg+pVBvpHFGBi\n"
        "wpqGbfsjaUDuYmBeJRcMO0XYkLCRQG+lAQNHoFDdItWAJfC3FwtP3OCDnz8cNwID\n"
        "AQABo4IBEzCCAQ8wHQYDVR0OBBYEFCSv2kmiGg6VFMnxXzLDNP304cPAMIHfBgNV\n"
        "HSMEgdcwgdSAFCSv2kmiGg6VFMnxXzLDNP304cPAoYGwpIGtMIGqMQswCQYDVQQG\n"
        "EwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEWMBQGA1UEBxMNU2FuIEZyYW5jaXNj\n"
        "bzEbMBkGA1UEChMSSGlnaCBGaWRlbGl0eSwgSW5jMRMwEQYDVQQLEwpPcGVyYXRp\n"
        "b25zMRgwFgYDVQQDEw9oaWdoZmlkZWxpdHkuaW8xIjAgBgkqhkiG9w0BCQEWE29w\n"
        "c0BoaWdoZmlkZWxpdHkuaW+CCQDZX0ZEQ/QPGzAMBgNVHRMEBTADAQH/MA0GCSqG\n"
        "SIb3DQEBBQUAA4GBAEkQl3p+lH5vuoCNgyfa67nL0MsBEt+5RSBOgjwCjjASjzou\n"
        "FTv5w0he2OypgMQb8i/BYtS1lJSFqjPJcSM1Salzrm3xDOK5pOXJ7h6SQLPDVEyf\n"
        "Hy2/9d/to+99+SOUlvfzfgycgjOc+s/AV7Y+GBd7uzGxUdrN4egCZW1F6/mH\n"
        "-----END CERTIFICATE-----";
    
    if (!datumInitialized) {
        hifiCADatum.data = HIGHFIDELITY_ROOT_CA_CERT;
        hifiCADatum.size = sizeof(HIGHFIDELITY_ROOT_CA_CERT);
    }
    
    return &hifiCADatum;
}

DTLSSession::DTLSSession(int end, QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    DummyDTLSSession(dtlsSocket, destinationSocket),
    _completedHandshake(false)
{
    gnutls_init(&_gnutlsSession, end | GNUTLS_DATAGRAM | GNUTLS_NONBLOCK);
    
    // see http://gnutls.org/manual/html_node/Datagram-TLS-API.html#gnutls_005fdtls_005fset_005fmtu
    const unsigned int DTLS_MAX_MTU = 1452;
    gnutls_dtls_set_mtu(_gnutlsSession, DTLS_MAX_MTU);
    
    const unsigned int DTLS_HANDSHAKE_RETRANSMISSION_TIMEOUT = DOMAIN_SERVER_CHECK_IN_MSECS;
    const unsigned int DTLS_TOTAL_CONNECTION_TIMEOUT = 2 * NODE_SILENCE_THRESHOLD_MSECS;
    gnutls_dtls_set_timeouts(_gnutlsSession, DTLS_HANDSHAKE_RETRANSMISSION_TIMEOUT, DTLS_TOTAL_CONNECTION_TIMEOUT);
    
    gnutls_transport_set_ptr(_gnutlsSession, this);
    gnutls_transport_set_push_function(_gnutlsSession, DummyDTLSSession::socketPush);
    gnutls_transport_set_pull_function(_gnutlsSession, socketPull);
    gnutls_transport_set_pull_timeout_function(_gnutlsSession, socketPullTimeout);
}

DTLSSession::~DTLSSession() {
    gnutls_deinit(_gnutlsSession);
}

void DTLSSession::setCompletedHandshake(bool completedHandshake) {
     _completedHandshake = completedHandshake;
    qDebug() << "Completed DTLS handshake with" << _destinationSocket;
}

qint64 DTLSSession::writeDatagram(const QByteArray& datagram) {
    // we don't need to put a hash in this packet, so just send it off
    return gnutls_record_send(_gnutlsSession, datagram.data(), datagram.size());
}