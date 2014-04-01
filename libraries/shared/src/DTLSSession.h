//
//  DTLSSession.h
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__DTLSSession__
#define __hifi__DTLSSession__

#include <QtNetwork/QUdpSocket>

#include <gnutls/gnutls.h>

#include "HifiSockAddr.h"

class DTLSSession : public QObject {
    Q_OBJECT
public:
    DTLSSession(int end, QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket);
    
protected:
    static int socketPullTimeout(gnutls_transport_ptr_t ptr, unsigned int ms);
    static ssize_t socketPull(gnutls_transport_ptr_t ptr, void* buffer, size_t size);
    static ssize_t socketPush(gnutls_transport_ptr_t ptr, const void* buffer, size_t size);
    static gnutls_certificate_credentials_t* x509CACredentials();
    
    QUdpSocket& _dtlsSocket;
    gnutls_session_t _gnutlsSession;
    HifiSockAddr _destinationSocket;
};

#endif /* defined(__hifi__DTLSSession__) */
