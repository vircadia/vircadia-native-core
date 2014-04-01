//
//  DTLSServerSession.cpp
//  hifi
//
//  Created by Stephen Birarda on 2014-04-01.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "DTLSServerSession.h"

DTLSServerSession::DTLSServerSession(QUdpSocket& dtlsSocket, HifiSockAddr& destinationSocket) :
    DTLSSession(GNUTLS_SERVER, dtlsSocket, destinationSocket)
{
    
}