//
//  DummyDTLSSession.h
//  hifi
//
//  Created by Stephen Birarda on 2014-04-04.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__DummyDTLSSession__
#define __hifi__DummyDTLSSession__

#include <QtNetwork/QUdpSocket>

#include <gnutls/gnutls.h>

#include "HifiSockAddr.h"

#define DTLS_VERBOSE_DEBUG 1

class DummyDTLSSession : public QObject {
    Q_OBJECT
public:
    DummyDTLSSession(QUdpSocket& dtlsSocket, const HifiSockAddr& destinationSocket);
    
    static ssize_t socketPush(gnutls_transport_ptr_t ptr, const void* buffer, size_t size);
protected:
    QUdpSocket& _dtlsSocket;
    HifiSockAddr _destinationSocket;
};

#endif /* defined(__hifi__DummyDTLSSession__) */
