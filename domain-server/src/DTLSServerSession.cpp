//
//  DTLSServerSession.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DTLSServerSession.h"

DTLSServerSession::DTLSServerSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    DTLSSession(GNUTLS_SERVER, dtlsSocket, destinationSocket)
{
    
}