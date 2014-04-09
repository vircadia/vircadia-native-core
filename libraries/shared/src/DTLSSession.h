//
//  DTLSSession.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__DTLSSession__
#define __hifi__DTLSSession__

#include <QtNetwork/QUdpSocket>

#include <gnutls/gnutls.h>

#include "DummyDTLSSession.h"
#include "HifiSockAddr.h"

class DTLSSession : public DummyDTLSSession {
    Q_OBJECT
public:
    DTLSSession(int end, QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket);
    ~DTLSSession();
    
    static int socketPullTimeout(gnutls_transport_ptr_t ptr, unsigned int ms);
    static ssize_t socketPull(gnutls_transport_ptr_t ptr, void* buffer, size_t size);
    
    static gnutls_datum_t* highFidelityCADatum();
    
    qint64 writeDatagram(const QByteArray& datagram);
    
    gnutls_session_t* getGnuTLSSession() { return &_gnutlsSession; }
    
    bool completedHandshake() const { return _completedHandshake; }
    void setCompletedHandshake(bool completedHandshake);
protected:
    gnutls_session_t _gnutlsSession;
    bool _completedHandshake;
};

#endif /* defined(__hifi__DTLSSession__) */
