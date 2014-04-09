//
//  DummyDTLSSession.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-04-04.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__DummyDTLSSession__
#define __hifi__DummyDTLSSession__

#include <QtNetwork/QUdpSocket>

#include <gnutls/gnutls.h>

#include "HifiSockAddr.h"

#define DTLS_VERBOSE_DEBUG 0

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
